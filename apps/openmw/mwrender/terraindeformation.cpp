#include "terraindeformation.hpp"

#include <osg/Geometry>
#include <osg/Texture2D>
#include <osgUtil/CullVisitor>

#include <components/debug/debuglog.hpp>
#include <components/resource/imagemanager.hpp>
#include <components/resource/resourcesystem.hpp>
#include <components/resource/scenemanager.hpp>
#include <components/sceneutil/color.hpp>
#include <components/sceneutil/depth.hpp>
#include <components/sceneutil/glextensions.hpp>
#include <components/shader/shadermanager.hpp>

#include "../mwworld/ptr.hpp"

#include "../mwmechanics/actorutil.hpp"

#include "vismask.hpp"

namespace MWRender
{
    TerrainDeformationSurface::TerrainDeformationSurface(Resource::ResourceSystem* resourceSystem)
        : osg::Geometry()
        , mResourceSystem(resourceSystem)
    {
        setUseDisplayList(false);
        setUseVertexBufferObjects(true);

        // Create fullscreen triangle
        osg::ref_ptr<osg::Vec3Array> verts = new osg::Vec3Array;
        verts->push_back(osg::Vec3f(-1, -1, 0));
        verts->push_back(osg::Vec3f(-1, 3, 0));
        verts->push_back(osg::Vec3f(3, -1, 0));

        setVertexArray(verts);

        setCullingActive(false);

        addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::TRIANGLES, 0, 3));

#ifdef __APPLE__
        // Apple compatibility: disable compute shaders
        mUseCompute = false;
#else
        constexpr float minimumGLVersionRequiredForCompute = 4.4;
        osg::GLExtensions& exts = SceneUtil::getGLExtensions();
        mUseCompute = exts.glVersion >= minimumGLVersionRequiredForCompute
            && exts.glslLanguageVersion >= minimumGLVersionRequiredForCompute;
#endif

        // Initialize state sets for double buffering
        for (size_t i = 0; i < mState.size(); ++i)
        {
            osg::ref_ptr<osg::StateSet> stateset = new osg::StateSet;

            if (!mUseCompute)
                stateset->addUniform(new osg::Uniform("imageIn", 0));

            stateset->addUniform(new osg::Uniform("offset", osg::Vec2f()));
            stateset->addUniform(new osg::Uniform("positionCount", 0));
            stateset->addUniform(new osg::Uniform(osg::Uniform::Type::FLOAT_VEC4, "positions", 100));
            stateset->addUniform(new osg::Uniform(osg::Uniform::Type::INT, "materialTypes", 100));
            stateset->setAttributeAndModes(
                new osg::Viewport(0, 0, TerrainDeformationSurface::sRTTSize, TerrainDeformationSurface::sRTTSize));
            mState[i].mStateset = stateset;
        }

        // Initialize textures for ping-pong rendering
        for (size_t i = 0; i < mTextures.size(); ++i)
        {
            osg::ref_ptr<osg::Texture2D> texture = new osg::Texture2D;
            texture->setSourceFormat(GL_RED);
            texture->setInternalFormat(GL_R16F); // Single channel float for displacement
            texture->setFilter(osg::Texture2D::MIN_FILTER, osg::Texture::LINEAR);
            texture->setFilter(osg::Texture2D::MAG_FILTER, osg::Texture::LINEAR);
            texture->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_BORDER);
            texture->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_BORDER);
            texture->setBorderColor(osg::Vec4(0, 0, 0, 0));
            texture->setTextureSize(sRTTSize, sRTTSize);

            mTextures[i] = texture;

            mFBOs[i] = new osg::FrameBufferObject;
            mFBOs[i]->setAttachment(osg::Camera::COLOR_BUFFER0, osg::FrameBufferAttachment(mTextures[i]));
        }

        if (mUseCompute)
            setupComputePipeline();
        else
            setupFragmentPipeline();

        if (mProgramStamp != nullptr)
        {
            [[maybe_unused]] static const bool pipelineLogged = [&] {
                if (mUseCompute)
                    Log(Debug::Info) << "Initialized compute shader pipeline for terrain deformation";
                else
                    Log(Debug::Info) << "Initialized fallback fragment shader pipeline for terrain deformation";
                return true;
            }();
        }

        setCullCallback(new osg::NodeCallback);
        setUpdateCallback(new osg::NodeCallback);
    }

    void TerrainDeformationSurface::setupFragmentPipeline()
    {
        auto& shaderManager = mResourceSystem->getSceneManager()->getShaderManager();

        Shader::ShaderManager::DefineMap defineMap
            = { { "deformMapSize", std::to_string(sRTTSize) + ".0" } };

        osg::ref_ptr<osg::Shader> vertex = shaderManager.getShader("fullscreen_tri.vert", {}, osg::Shader::VERTEX);
        osg::ref_ptr<osg::Shader> stamp
            = shaderManager.getShader("terrain_deform_stamp.frag", defineMap, osg::Shader::FRAGMENT);
        osg::ref_ptr<osg::Shader> decay
            = shaderManager.getShader("terrain_deform_decay.frag", defineMap, osg::Shader::FRAGMENT);

        if (vertex == nullptr || stamp == nullptr || decay == nullptr)
        {
            Log(Debug::Error) << "Failed to load shaders required for fragment shader terrain deformation pipeline";
            return;
        }

        mProgramStamp = shaderManager.getProgram(vertex, std::move(stamp));
        mProgramDecay = shaderManager.getProgram(std::move(vertex), std::move(decay));
    }

    void TerrainDeformationSurface::setupComputePipeline()
    {
        auto& shaderManager = mResourceSystem->getSceneManager()->getShaderManager();

        osg::ref_ptr<osg::Shader> stamp
            = shaderManager.getShader("core/terrain_deform_stamp.comp", {}, osg::Shader::COMPUTE);
        osg::ref_ptr<osg::Shader> decay
            = shaderManager.getShader("core/terrain_deform_decay.comp", {}, osg::Shader::COMPUTE);

        if (stamp == nullptr || decay == nullptr)
        {
            Log(Debug::Error) << "Failed to load shaders required for compute shader terrain deformation pipeline";
            return;
        }

        mProgramStamp = shaderManager.getProgram(nullptr, std::move(stamp));
        mProgramDecay = shaderManager.getProgram(nullptr, std::move(decay));
    }

    void TerrainDeformationSurface::updateState(const osg::FrameStamp& frameStamp, State& state)
    {
        state.mPaused = mPaused;

        if (mPaused)
            return;

        constexpr double updateFrequency = 30.0; // Update at 30 Hz
        constexpr double updatePeriod = 1.0 / updateFrequency;

        const double simulationTime = frameStamp.getSimulationTime();
        const double frameDuration = simulationTime - mLastSimulationTime;
        mLastSimulationTime = simulationTime;

        mRemainingDecayTime += frameDuration;
        const double ticks = std::floor(mRemainingDecayTime * updateFrequency);
        mRemainingDecayTime -= ticks * updatePeriod;

        if (ticks == 0 && mPositionCount == 0)
        {
            state.mPaused = true;
            return;
        }

        const MWWorld::Ptr player = MWMechanics::getPlayer();
        const ESM::Position& playerPos = player.getRefData().getPosition();

        mCurrentPlayerPos = osg::Vec2f(
            std::floor(playerPos.pos[0] / sWorldScaleFactor), std::floor(playerPos.pos[1] / sWorldScaleFactor));
        const osg::Vec2f offset = mCurrentPlayerPos - mLastPlayerPos;
        mLastPlayerPos = mCurrentPlayerPos;

        state.mStateset->getUniform("positionCount")->set(static_cast<int>(mPositionCount));
        state.mStateset->getUniform("offset")->set(offset);

        osg::Uniform* const positions = state.mStateset->getUniform("positions");
        osg::Uniform* const materialTypes = state.mStateset->getUniform("materialTypes");

        for (std::size_t i = 0; i < mPositionCount; ++i)
        {
            osg::Vec3f pos = mDeformationPoints[i].position
                - osg::Vec3f(mCurrentPlayerPos.x() * sWorldScaleFactor, mCurrentPlayerPos.y() * sWorldScaleFactor, 0.0)
                + osg::Vec3f(sRTTSize * sWorldScaleFactor / 2, sRTTSize * sWorldScaleFactor / 2, 0.0);
            pos /= sWorldScaleFactor;

            // Pack material type into w component
            osg::Vec4f posWithMaterial(pos.x(), pos.y(), pos.z(), static_cast<float>(mDeformationPoints[i].materialType));
            positions->setElement(i, posWithMaterial);
            materialTypes->setElement(i, static_cast<int>(mDeformationPoints[i].materialType));
        }
        positions->dirty();
        materialTypes->dirty();

        mPositionCount = 0;
    }

    void TerrainDeformationSurface::traverse(osg::NodeVisitor& nv)
    {
        const osg::FrameStamp* const frameStamp = nv.getFrameStamp();

        if (frameStamp == nullptr)
            return;

        if (nv.getVisitorType() == osg::NodeVisitor::CULL_VISITOR)
            updateState(*frameStamp, mState[frameStamp->getFrameNumber() % 2]);

        osg::Geometry::traverse(nv);
    }

    void TerrainDeformationSurface::drawImplementation(osg::RenderInfo& renderInfo) const
    {
        if (mProgramStamp == nullptr || mProgramDecay == nullptr)
            return;

        osg::State& state = *renderInfo.getState();
        const std::size_t currentFrame = state.getFrameStamp()->getFrameNumber() % 2;
        const State& frameState = mState[currentFrame];

        if (frameState.mPaused)
            return;

        osg::GLExtensions& ext = *state.get<osg::GLExtensions>();
        const std::size_t contextID = state.getContextID();

        const auto bindImage = [&](osg::Texture2D* texture, GLuint index, GLenum access) {
            osg::Texture::TextureObject* to = texture->getTextureObject(contextID);
            if (!to || texture->isDirty(contextID))
            {
                state.applyTextureAttribute(index, texture);
                to = texture->getTextureObject(contextID);
            }
            ext.glBindImageTexture(index, to->id(), 0, GL_FALSE, 0, access, GL_R16F);
        };

        // PASS 1: Stamp footprints
        state.pushStateSet(frameState.mStateset);
        state.apply();
        state.applyAttribute(mProgramStamp);

        for (const auto& [name, stack] : state.getUniformMap())
        {
            if (!stack.uniformVec.empty())
                state.getLastAppliedProgramObject()->apply(*(stack.uniformVec.back().first));
        }

        if (mUseCompute)
        {
            bindImage(mTextures[1], 0, GL_WRITE_ONLY_ARB);
            bindImage(mTextures[0], 1, GL_READ_ONLY_ARB);

            ext.glDispatchCompute(sRTTSize / 16, sRTTSize / 16, 1);
            ext.glMemoryBarrier(GL_ALL_BARRIER_BITS);
        }
        else
        {
            mFBOs[1]->apply(state, osg::FrameBufferObject::DRAW_FRAMEBUFFER);
            state.applyTextureAttribute(0, mTextures[0]);
            osg::Geometry::drawImplementation(renderInfo);
        }

        // PASS 2: Decay over time
        state.applyAttribute(mProgramDecay);

        for (const auto& [name, stack] : state.getUniformMap())
        {
            if (!stack.uniformVec.empty())
                state.getLastAppliedProgramObject()->apply(*(stack.uniformVec.back().first));
        }

        if (mUseCompute)
        {
            bindImage(mTextures[0], 0, GL_WRITE_ONLY_ARB);
            bindImage(mTextures[1], 1, GL_READ_ONLY_ARB);

            ext.glDispatchCompute(sRTTSize / 16, sRTTSize / 16, 1);
            ext.glMemoryBarrier(GL_ALL_BARRIER_BITS);
        }
        else
        {
            mFBOs[0]->apply(state, osg::FrameBufferObject::DRAW_FRAMEBUFFER);
            state.applyTextureAttribute(0, mTextures[1]);
            osg::Geometry::drawImplementation(renderInfo);
        }

        state.popStateSet();
    }

    osg::Texture* TerrainDeformationSurface::getColorTexture() const
    {
        return mTextures[0];
    }

    void TerrainDeformationSurface::emit(const osg::Vec3f pos, float sizeInCellUnits, TerrainMaterialType materialType)
    {
        // Emitted positions are reset every frame, don't bother wrapping around when out of buffer space
        if (mPositionCount >= mDeformationPoints.size())
            return;

        mDeformationPoints[mPositionCount].position = pos;
        mDeformationPoints[mPositionCount].materialType = materialType;
        mDeformationPoints[mPositionCount].position.z() = sizeInCellUnits; // Store size in z component

        mPositionCount++;
    }

    void TerrainDeformationSurface::releaseGLObjects(osg::State* state) const
    {
        for (const auto& tex : mTextures)
            tex->releaseGLObjects(state);
        for (const auto& fbo : mFBOs)
            fbo->releaseGLObjects(state);

        if (mProgramStamp)
            mProgramStamp->releaseGLObjects(state);
        if (mProgramDecay)
            mProgramDecay->releaseGLObjects(state);
    }

    TerrainDeformation::TerrainDeformation(Resource::ResourceSystem* resourceSystem)
        : osg::Camera()
        , mDeformationSurface(new TerrainDeformationSurface(resourceSystem))
    {
        getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
        getOrCreateStateSet()->setMode(GL_DEPTH_TEST, osg::StateAttribute::OFF);
        setRenderOrder(osg::Camera::PRE_RENDER);
        setReferenceFrame(osg::Camera::ABSOLUTE_RF);
        setNodeMask(Mask_RenderToTexture);
        setClearMask(GL_NONE);
        setViewport(
            0, 0, TerrainDeformationSurface::sRTTSize, TerrainDeformationSurface::sRTTSize);
        addChild(mDeformationSurface);
        setCullingActive(false);
        setImplicitBufferAttachmentMask(0, 0);
    }

    osg::Texture* TerrainDeformation::getColorTexture() const
    {
        return mDeformationSurface->getColorTexture();
    }

    void TerrainDeformation::emit(const osg::Vec3f pos, float sizeInCellUnits, TerrainMaterialType materialType)
    {
        mDeformationSurface->emit(pos, sizeInCellUnits, materialType);
    }

    void TerrainDeformation::setPaused(bool paused)
    {
        mDeformationSurface->setPaused(paused);
    }
}

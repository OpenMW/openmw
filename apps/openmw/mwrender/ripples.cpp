#include "ripples.hpp"

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
    RipplesSurface::RipplesSurface(Resource::ResourceSystem* resourceSystem)
        : osg::Geometry()
        , mResourceSystem(resourceSystem)
    {
        setUseDisplayList(false);
        setUseVertexBufferObjects(true);

        osg::ref_ptr<osg::Vec3Array> verts = new osg::Vec3Array;
        verts->push_back(osg::Vec3f(-1, -1, 0));
        verts->push_back(osg::Vec3f(-1, 3, 0));
        verts->push_back(osg::Vec3f(3, -1, 0));

        setVertexArray(verts);

        setCullingActive(false);

        addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::TRIANGLES, 0, 3));

#ifdef __APPLE__
        // we can not trust Apple :)
        mUseCompute = false;
#else
        constexpr float minimumGLVersionRequiredForCompute = 4.4f;
        osg::GLExtensions& exts = SceneUtil::getGLExtensions();
        mUseCompute = exts.glVersion >= minimumGLVersionRequiredForCompute
            && exts.glslLanguageVersion >= minimumGLVersionRequiredForCompute;
#endif

        for (size_t i = 0; i < mState.size(); ++i)
        {
            osg::ref_ptr<osg::StateSet> stateset = new osg::StateSet;
            // bindings are set in the compute shader
            if (!mUseCompute)
                stateset->addUniform(new osg::Uniform("imageIn", 0));

            stateset->addUniform(new osg::Uniform("offset", osg::Vec2f()));
            stateset->addUniform(new osg::Uniform("positionCount", 0));
            stateset->addUniform(new osg::Uniform(osg::Uniform::Type::FLOAT_VEC3, "positions", 100));
            stateset->setAttributeAndModes(new osg::Viewport(0, 0, RipplesSurface::sRTTSize, RipplesSurface::sRTTSize));
            mState[i].mStateset = stateset;
        }

        for (size_t i = 0; i < mTextures.size(); ++i)
        {
            osg::ref_ptr<osg::Texture2D> texture = new osg::Texture2D;
            texture->setSourceFormat(GL_RGBA);
            texture->setInternalFormat(GL_RGBA16F);
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

        if (mProgramBlobber != nullptr)
        {
            [[maybe_unused]] static const bool pipelineLogged = [&] {
                if (mUseCompute)
                    Log(Debug::Info) << "Initialized compute shader pipeline for water ripples";
                else
                    Log(Debug::Info) << "Initialized fallback fragment shader pipeline for water ripples";
                return true;
            }();
        }

        setCullCallback(new osg::NodeCallback);
        setUpdateCallback(new osg::NodeCallback);
    }

    void RipplesSurface::setupFragmentPipeline()
    {
        auto& shaderManager = mResourceSystem->getSceneManager()->getShaderManager();

        Shader::ShaderManager::DefineMap defineMap = { { "rippleMapSize", std::to_string(sRTTSize) + ".0" } };

        osg::ref_ptr<osg::Shader> vertex = shaderManager.getShader("fullscreen_tri.vert", {}, osg::Shader::VERTEX);
        osg::ref_ptr<osg::Shader> blobber
            = shaderManager.getShader("ripples_blobber.frag", defineMap, osg::Shader::FRAGMENT);
        osg::ref_ptr<osg::Shader> simulate
            = shaderManager.getShader("ripples_simulate.frag", defineMap, osg::Shader::FRAGMENT);
        if (vertex == nullptr || blobber == nullptr || simulate == nullptr)
        {
            Log(Debug::Error) << "Failed to load shaders required for fragment shader ripple pipeline";
            return;
        }

        mProgramBlobber = shaderManager.getProgram(vertex, std::move(blobber));
        mProgramSimulation = shaderManager.getProgram(std::move(vertex), std::move(simulate));
    }

    void RipplesSurface::setupComputePipeline()
    {
        auto& shaderManager = mResourceSystem->getSceneManager()->getShaderManager();

        osg::ref_ptr<osg::Shader> blobber
            = shaderManager.getShader("core/ripples_blobber.comp", {}, osg::Shader::COMPUTE);
        osg::ref_ptr<osg::Shader> simulate
            = shaderManager.getShader("core/ripples_simulate.comp", {}, osg::Shader::COMPUTE);
        if (blobber == nullptr || simulate == nullptr)
        {
            Log(Debug::Error) << "Failed to load shaders required for compute shader ripple pipeline";
            return;
        }

        mProgramBlobber = shaderManager.getProgram(nullptr, std::move(blobber));
        mProgramSimulation = shaderManager.getProgram(nullptr, std::move(simulate));
    }

    void RipplesSurface::updateState(const osg::FrameStamp& frameStamp, State& state)
    {
        state.mPaused = mPaused;

        if (mPaused)
            return;

        constexpr double updateFrequency = 60.0;
        constexpr double updatePeriod = 1.0 / updateFrequency;

        const double simulationTime = frameStamp.getSimulationTime();
        const double frameDuration = simulationTime - mLastSimulationTime;
        mLastSimulationTime = simulationTime;

        mRemainingWaveTime += frameDuration;
        const double ticks = std::floor(mRemainingWaveTime * updateFrequency);
        mRemainingWaveTime -= ticks * updatePeriod;

        if (ticks == 0)
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

        for (std::size_t i = 0; i < mPositionCount; ++i)
        {
            osg::Vec3f pos = mPositions[i]
                - osg::Vec3f(mCurrentPlayerPos.x() * sWorldScaleFactor, mCurrentPlayerPos.y() * sWorldScaleFactor, 0.0)
                + osg::Vec3f(sRTTSize * sWorldScaleFactor / 2, sRTTSize * sWorldScaleFactor / 2, 0.0);
            pos /= sWorldScaleFactor;
            positions->setElement(static_cast<unsigned>(i), pos);
        }
        positions->dirty();

        mPositionCount = 0;
    }

    void RipplesSurface::traverse(osg::NodeVisitor& nv)
    {
        const osg::FrameStamp* const frameStamp = nv.getFrameStamp();

        if (frameStamp == nullptr)
            return;

        if (nv.getVisitorType() == osg::NodeVisitor::CULL_VISITOR)
            updateState(*frameStamp, mState[frameStamp->getFrameNumber() % 2]);

        osg::Geometry::traverse(nv);
    }

    void RipplesSurface::drawImplementation(osg::RenderInfo& renderInfo) const
    {
        if (mProgramBlobber == nullptr || mProgramSimulation == nullptr)
            return;

        osg::State& state = *renderInfo.getState();
        const std::size_t currentFrame = state.getFrameStamp()->getFrameNumber() % 2;
        const State& frameState = mState[currentFrame];
        if (frameState.mPaused)
        {
            return;
        }

        osg::GLExtensions& ext = *state.get<osg::GLExtensions>();
        const unsigned contextID = state.getContextID();

        const auto bindImage = [&](osg::Texture2D* texture, GLuint index, GLenum access) {
            osg::Texture::TextureObject* to = texture->getTextureObject(contextID);
            if (!to || texture->isDirty(contextID))
            {
                state.applyTextureAttribute(index, texture);
                to = texture->getTextureObject(contextID);
            }
            ext.glBindImageTexture(index, to->id(), 0, GL_FALSE, 0, access, GL_RGBA16F);
        };

        // PASS: Blot in all ripple spawners
        state.pushStateSet(frameState.mStateset);
        state.apply();
        state.applyAttribute(mProgramBlobber);
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

        // PASS: Wave simulation
        state.applyAttribute(mProgramSimulation);
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

    osg::Texture* RipplesSurface::getColorTexture() const
    {
        return mTextures[0];
    }

    void RipplesSurface::emit(const osg::Vec3f pos, float sizeInCellUnits)
    {
        // Emitted positions are reset every frame, don't bother wrapping around when out of buffer space
        if (mPositionCount >= mPositions.size())
        {
            return;
        }

        mPositions[mPositionCount] = osg::Vec3f(pos.x(), pos.y(), sizeInCellUnits);

        mPositionCount++;
    }

    void RipplesSurface::releaseGLObjects(osg::State* state) const
    {
        for (const auto& tex : mTextures)
            tex->releaseGLObjects(state);
        for (const auto& fbo : mFBOs)
            fbo->releaseGLObjects(state);

        if (mProgramBlobber)
            mProgramBlobber->releaseGLObjects(state);
        if (mProgramSimulation)
            mProgramSimulation->releaseGLObjects(state);
    }

    Ripples::Ripples(Resource::ResourceSystem* resourceSystem)
        : osg::Camera()
        , mRipples(new RipplesSurface(resourceSystem))
    {
        getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
        getOrCreateStateSet()->setMode(GL_DEPTH_TEST, osg::StateAttribute::OFF);
        setRenderOrder(osg::Camera::PRE_RENDER);
        setReferenceFrame(osg::Camera::ABSOLUTE_RF);
        setNodeMask(Mask_RenderToTexture);
        setClearMask(GL_NONE);
        setViewport(0, 0, RipplesSurface::sRTTSize, RipplesSurface::sRTTSize);
        addChild(mRipples);
        setCullingActive(false);
        setImplicitBufferAttachmentMask(0, 0);
    }

    osg::Texture* Ripples::getColorTexture() const
    {
        return mRipples->getColorTexture();
    }

    void Ripples::emit(const osg::Vec3f pos, float sizeInCellUnits)
    {
        mRipples->emit(pos, sizeInCellUnits);
    }

    void Ripples::setPaused(bool paused)
    {
        mRipples->setPaused(paused);
    }
}

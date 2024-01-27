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
        constexpr float minimumGLVersionRequiredForCompute = 4.4;
        osg::GLExtensions* exts = osg::GLExtensions::Get(0, false);
        mUseCompute = exts->glVersion >= minimumGLVersionRequiredForCompute
            && exts->glslLanguageVersion >= minimumGLVersionRequiredForCompute;
#endif

        if (mUseCompute)
            Log(Debug::Info) << "Initialized compute shader pipeline for water ripples";
        else
            Log(Debug::Info) << "Initialized fallback fragment shader pipeline for water ripples";

        for (size_t i = 0; i < mState.size(); ++i)
        {
            osg::ref_ptr<osg::StateSet> stateset = new osg::StateSet;
            // bindings are set in the compute shader
            if (!mUseCompute)
                stateset->addUniform(new osg::Uniform("imageIn", 0));

            stateset->addUniform(new osg::Uniform("offset", osg::Vec2f()));
            stateset->addUniform(new osg::Uniform("positionCount", 0));
            stateset->addUniform(new osg::Uniform(osg::Uniform::Type::FLOAT_VEC3, "positions", 100));
            stateset->setAttributeAndModes(new osg::Viewport(0, 0, RipplesSurface::mRTTSize, RipplesSurface::mRTTSize));
            mState[i].mStateset = stateset;
        }

        for (size_t i = 0; i < mTextures.size(); ++i)
        {
            osg::ref_ptr<osg::Texture2D> texture = new osg::Texture2D;
            texture->setSourceFormat(GL_RGBA);
            texture->setSourceType(GL_HALF_FLOAT);
            texture->setInternalFormat(GL_RGBA16F);
            texture->setFilter(osg::Texture2D::MIN_FILTER, osg::Texture::LINEAR);
            texture->setFilter(osg::Texture2D::MAG_FILTER, osg::Texture::LINEAR);
            texture->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_BORDER);
            texture->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_BORDER);
            texture->setBorderColor(osg::Vec4(0, 0, 0, 0));
            texture->setTextureSize(mRTTSize, mRTTSize);

            mTextures[i] = texture;

            mFBOs[i] = new osg::FrameBufferObject;
            mFBOs[i]->setAttachment(osg::Camera::COLOR_BUFFER0, osg::FrameBufferAttachment(mTextures[i]));
        }

        if (mUseCompute)
            setupComputePipeline();
        else
            setupFragmentPipeline();

        setCullCallback(new osg::NodeCallback);
        setUpdateCallback(new osg::NodeCallback);
    }

    void RipplesSurface::setupFragmentPipeline()
    {
        auto& shaderManager = mResourceSystem->getSceneManager()->getShaderManager();

        Shader::ShaderManager::DefineMap defineMap = { { "ripple_map_size", std::to_string(mRTTSize) + ".0" } };

        osg::ref_ptr<osg::Shader> vertex = shaderManager.getShader("fullscreen_tri.vert", {}, osg::Shader::VERTEX);

        mProgramBlobber = shaderManager.getProgram(
            vertex, shaderManager.getShader("ripples_blobber.frag", defineMap, osg::Shader::FRAGMENT));
        mProgramSimulation = shaderManager.getProgram(
            std::move(vertex), shaderManager.getShader("ripples_simulate.frag", defineMap, osg::Shader::FRAGMENT));
    }

    void RipplesSurface::setupComputePipeline()
    {
        auto& shaderManager = mResourceSystem->getSceneManager()->getShaderManager();

        mProgramBlobber = shaderManager.getProgram(
            nullptr, shaderManager.getShader("core/ripples_blobber.comp", {}, osg::Shader::COMPUTE));
        mProgramSimulation = shaderManager.getProgram(
            nullptr, shaderManager.getShader("core/ripples_simulate.comp", {}, osg::Shader::COMPUTE));
    }

    void RipplesSurface::traverse(osg::NodeVisitor& nv)
    {
        if (!nv.getFrameStamp())
            return;

        if (nv.getVisitorType() == osg::NodeVisitor::CULL_VISITOR)
        {
            size_t frameId = nv.getFrameStamp()->getFrameNumber() % 2;

            const auto& player = MWMechanics::getPlayer();
            const ESM::Position& playerPos = player.getRefData().getPosition();

            mCurrentPlayerPos = osg::Vec2f(
                std::floor(playerPos.pos[0] / mWorldScaleFactor), std::floor(playerPos.pos[1] / mWorldScaleFactor));
            osg::Vec2f offset = mCurrentPlayerPos - mLastPlayerPos;
            mLastPlayerPos = mCurrentPlayerPos;
            mState[frameId].mPaused = mPaused;
            mState[frameId].mOffset = offset;
            mState[frameId].mStateset->getUniform("positionCount")->set(static_cast<int>(mPositionCount));
            mState[frameId].mStateset->getUniform("offset")->set(offset);

            auto* positions = mState[frameId].mStateset->getUniform("positions");

            for (size_t i = 0; i < mPositionCount; ++i)
            {
                osg::Vec3f pos = mPositions[i]
                    - osg::Vec3f(
                        mCurrentPlayerPos.x() * mWorldScaleFactor, mCurrentPlayerPos.y() * mWorldScaleFactor, 0.0)
                    + osg::Vec3f(mRTTSize * mWorldScaleFactor / 2, mRTTSize * mWorldScaleFactor / 2, 0.0);
                pos /= mWorldScaleFactor;
                positions->setElement(i, pos);
            }
            positions->dirty();

            mPositionCount = 0;
        }
        osg::Geometry::traverse(nv);
    }

    void RipplesSurface::drawImplementation(osg::RenderInfo& renderInfo) const
    {
        osg::State& state = *renderInfo.getState();
        osg::GLExtensions& ext = *state.get<osg::GLExtensions>();
        size_t contextID = state.getContextID();

        size_t currentFrame = state.getFrameStamp()->getFrameNumber() % 2;
        const State& frameState = mState[currentFrame];
        if (frameState.mPaused)
        {
            return;
        }

        auto bindImage = [contextID, &state, &ext](osg::Texture2D* texture, GLuint index, GLenum access) {
            osg::Texture::TextureObject* to = texture->getTextureObject(contextID);
            if (!to || texture->isDirty(contextID))
            {
                state.applyTextureAttribute(index, texture);
                to = texture->getTextureObject(contextID);
            }
            ext.glBindImageTexture(index, to->id(), 0, GL_FALSE, 0, access, GL_RGBA16F);
        };

        // Run simulation at a fixed rate independent on current FPS
        // FIXME: when we skip frames we need to preserve positions. this doesn't work now
        size_t ticks = 1;

        // PASS: Blot in all ripple spawners
        mProgramBlobber->apply(state);
        state.apply(frameState.mStateset);

        for (size_t i = 0; i < ticks; i++)
        {
            if (mUseCompute)
            {
                bindImage(mTextures[1], 0, GL_WRITE_ONLY_ARB);
                bindImage(mTextures[0], 1, GL_READ_ONLY_ARB);

                ext.glDispatchCompute(mRTTSize / 16, mRTTSize / 16, 1);
                ext.glMemoryBarrier(GL_ALL_BARRIER_BITS);
            }
            else
            {
                mFBOs[1]->apply(state, osg::FrameBufferObject::DRAW_FRAMEBUFFER);
                state.applyTextureAttribute(0, mTextures[0]);
                osg::Geometry::drawImplementation(renderInfo);
            }
        }

        // PASS: Wave simulation
        mProgramSimulation->apply(state);
        state.apply(frameState.mStateset);

        for (size_t i = 0; i < ticks; i++)
        {
            if (mUseCompute)
            {
                bindImage(mTextures[0], 0, GL_WRITE_ONLY_ARB);
                bindImage(mTextures[1], 1, GL_READ_ONLY_ARB);

                ext.glDispatchCompute(mRTTSize / 16, mRTTSize / 16, 1);
                ext.glMemoryBarrier(GL_ALL_BARRIER_BITS);
            }
            else
            {
                mFBOs[0]->apply(state, osg::FrameBufferObject::DRAW_FRAMEBUFFER);
                state.applyTextureAttribute(0, mTextures[1]);
                osg::Geometry::drawImplementation(renderInfo);
            }
        }
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
        setViewport(0, 0, RipplesSurface::mRTTSize, RipplesSurface::mRTTSize);
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

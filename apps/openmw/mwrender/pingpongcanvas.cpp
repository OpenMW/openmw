#include "pingpongcanvas.hpp"

#include <cassert>

#include <components/shader/shadermanager.hpp>
#include <components/stereo/multiview.hpp>
#include <components/stereo/stereomanager.hpp>

#include <osg/Texture2DArray>

#include "postprocessor.hpp"

namespace MWRender
{
    PingPongCanvas::PingPongCanvas(
        Shader::ShaderManager& shaderManager, const std::shared_ptr<LuminanceCalculator>& luminanceCalculator)
        : mFallbackStateSet(new osg::StateSet)
        , mMultiviewResolveStateSet(new osg::StateSet)
        , mLuminanceCalculator(luminanceCalculator)
    {
        setUseDisplayList(false);
        setUseVertexBufferObjects(true);

        osg::ref_ptr<osg::Vec3Array> verts = new osg::Vec3Array;
        verts->push_back(osg::Vec3f(-1, -1, 0));
        verts->push_back(osg::Vec3f(-1, 3, 0));
        verts->push_back(osg::Vec3f(3, -1, 0));

        setVertexArray(verts);

        addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::TRIANGLES, 0, 3));

        mLuminanceCalculator->disable();

        Shader::ShaderManager::DefineMap defines;
        Stereo::shaderStereoDefines(defines);

        mFallbackProgram = shaderManager.getProgram("fullscreen_tri");

        mFallbackStateSet->setAttributeAndModes(mFallbackProgram);
        mFallbackStateSet->addUniform(new osg::Uniform("lastShader", 0));
        mFallbackStateSet->addUniform(new osg::Uniform("scaling", osg::Vec2f(1, 1)));

        mMultiviewResolveProgram = shaderManager.getProgram("multiview_resolve");
        mMultiviewResolveStateSet->setAttributeAndModes(mMultiviewResolveProgram);
        mMultiviewResolveStateSet->addUniform(new osg::Uniform("lastShader", 0));
    }

    void PingPongCanvas::setPasses(fx::DispatchArray&& passes)
    {
        mPasses = std::move(passes);
    }

    void PingPongCanvas::setMask(bool underwater, bool exterior)
    {
        mMask = 0;
        mMask |= underwater ? fx::Technique::Flag_Disable_Underwater : fx::Technique::Flag_Disable_Abovewater;
        mMask |= exterior ? fx::Technique::Flag_Disable_Exteriors : fx::Technique::Flag_Disable_Interiors;
    }

    void PingPongCanvas::drawGeometry(osg::RenderInfo& renderInfo) const
    {
        osg::Geometry::drawImplementation(renderInfo);
    }

    static void attachCloneOfTemplate(
        osg::FrameBufferObject* fbo, osg::Camera::BufferComponent component, osg::Texture* tex)
    {
        osg::ref_ptr<osg::Texture> clone = static_cast<osg::Texture*>(tex->clone(osg::CopyOp::SHALLOW_COPY));
        fbo->setAttachment(component, Stereo::createMultiviewCompatibleAttachment(clone));
    }

    void PingPongCanvas::drawImplementation(osg::RenderInfo& renderInfo) const
    {
        osg::State& state = *renderInfo.getState();
        osg::GLExtensions* ext = state.get<osg::GLExtensions>();

        size_t frameId = state.getFrameStamp()->getFrameNumber() % 2;

        std::vector<size_t> filtered;

        filtered.reserve(mPasses.size());

        for (size_t i = 0; i < mPasses.size(); ++i)
        {
            const auto& node = mPasses[i];

            if (mMask & node.mFlags)
                continue;

            filtered.push_back(i);
        }

        auto* resolveViewport = state.getCurrentViewport();

        if (filtered.empty() || !mPostprocessing)
        {
            state.pushStateSet(mFallbackStateSet);
            state.apply();

            if (Stereo::getMultiview())
            {
                state.pushStateSet(mMultiviewResolveStateSet);
                state.apply();
            }

            state.applyTextureAttribute(0, mTextureScene);
            resolveViewport->apply(state);

            drawGeometry(renderInfo);
            state.popStateSet();

            if (Stereo::getMultiview())
            {
                state.popStateSet();
            }

            return;
        }

        const unsigned int handle = mFbos[0] ? mFbos[0]->getHandle(state.getContextID()) : 0;

        if (handle == 0 || mDirty)
        {
            for (auto& fbo : mFbos)
            {
                fbo = new osg::FrameBufferObject;
                attachCloneOfTemplate(fbo, osg::FrameBufferObject::BufferComponent::COLOR_BUFFER0, mTextureScene);
                fbo->apply(state);
                glClearColor(0.5, 0.5, 0.5, 1);
                glClear(GL_COLOR_BUFFER_BIT);
            }

            if (Stereo::getMultiview())
            {
                mMultiviewResolveFramebuffer = new osg::FrameBufferObject();
                attachCloneOfTemplate(mMultiviewResolveFramebuffer,
                    osg::FrameBufferObject::BufferComponent::COLOR_BUFFER0, mTextureScene);
                mMultiviewResolveFramebuffer->apply(state);
                glClearColor(0.5, 0.5, 0.5, 1);
                glClear(GL_COLOR_BUFFER_BIT);

                mMultiviewResolveStateSet->setTextureAttribute(PostProcessor::Unit_LastShader,
                    (osg::Texture*)mMultiviewResolveFramebuffer->getAttachment(osg::Camera::COLOR_BUFFER0)
                        .getTexture());
            }

            mLuminanceCalculator->dirty(mTextureScene->getTextureWidth(), mTextureScene->getTextureHeight());

            if (Stereo::getStereo())
                mRenderViewport
                    = new osg::Viewport(0, 0, mTextureScene->getTextureWidth(), mTextureScene->getTextureHeight());
            else
                mRenderViewport = nullptr;

            mDirty = false;
        }

        constexpr std::array<std::array<int, 2>, 3> buffers
            = { { { GL_COLOR_ATTACHMENT1_EXT, GL_COLOR_ATTACHMENT2_EXT },
                { GL_COLOR_ATTACHMENT0_EXT, GL_COLOR_ATTACHMENT2_EXT },
                { GL_COLOR_ATTACHMENT0_EXT, GL_COLOR_ATTACHMENT1_EXT } } };

        (mAvgLum) ? mLuminanceCalculator->enable() : mLuminanceCalculator->disable();

        // A histogram based approach is superior way to calculate scene luminance. Using mipmaps is more broadly
        // supported, so that's what we use for now.
        mLuminanceCalculator->draw(*this, renderInfo, state, ext, frameId);

        auto buffer = buffers[0];

        int lastDraw = 0;
        int lastShader = 0;

        unsigned int lastApplied = handle;

        const unsigned int cid = state.getContextID();

        const osg::ref_ptr<osg::FrameBufferObject>& destinationFbo = mDestinationFBO ? mDestinationFBO : nullptr;
        unsigned int destinationHandle = destinationFbo ? destinationFbo->getHandle(cid) : 0;

        auto bindDestinationFbo = [&]() {
            if (destinationFbo)
            {
                destinationFbo->apply(state, osg::FrameBufferObject::DRAW_FRAMEBUFFER);
                lastApplied = destinationHandle;
            }
            else if (Stereo::getMultiview())
            {
                mMultiviewResolveFramebuffer->apply(state, osg::FrameBufferObject::DRAW_FRAMEBUFFER);
                lastApplied = mMultiviewResolveFramebuffer->getHandle(cid);
            }
            else
            {
                ext->glBindFramebuffer(GL_DRAW_FRAMEBUFFER_EXT, 0);

                lastApplied = 0;
            }
        };

        // When textures are created (or resized) we need to either dirty them and/or clear them.
        // Otherwise, there will be undefined behavior when reading from a texture that has yet to be written to in a
        // later pass.
        for (const auto& attachment : mDirtyAttachments)
        {
            const auto [w, h]
                = attachment.mSize.get(mTextureScene->getTextureWidth(), mTextureScene->getTextureHeight());

            attachment.mTarget->setTextureSize(w, h);
            if (attachment.mMipMap)
                attachment.mTarget->setNumMipmapLevels(osg::Image::computeNumberOfMipmapLevels(w, h));
            attachment.mTarget->dirtyTextureObject();

            osg::ref_ptr<osg::FrameBufferObject> fbo = new osg::FrameBufferObject;

            fbo->setAttachment(
                osg::FrameBufferObject::BufferComponent::COLOR_BUFFER0, osg::FrameBufferAttachment(attachment.mTarget));
            fbo->apply(state, osg::FrameBufferObject::DRAW_FRAMEBUFFER);

            glViewport(0, 0, attachment.mTarget->getTextureWidth(), attachment.mTarget->getTextureHeight());
            state.haveAppliedAttribute(osg::StateAttribute::VIEWPORT);
            glClearColor(attachment.mClearColor.r(), attachment.mClearColor.g(), attachment.mClearColor.b(),
                attachment.mClearColor.a());
            glClear(GL_COLOR_BUFFER_BIT);

            if (attachment.mTarget->getNumMipmapLevels() > 0)
            {
                state.setActiveTextureUnit(0);
                state.applyTextureAttribute(0, attachment.mTarget);
                ext->glGenerateMipmap(GL_TEXTURE_2D);
            }
        }

        for (const size_t& index : filtered)
        {
            const auto& node = mPasses[index];

            node.mRootStateSet->setTextureAttribute(PostProcessor::Unit_Depth, mTextureDepth);

            if (mAvgLum)
                node.mRootStateSet->setTextureAttribute(PostProcessor::TextureUnits::Unit_EyeAdaptation,
                    mLuminanceCalculator->getLuminanceTexture(frameId));

            if (mTextureNormals)
                node.mRootStateSet->setTextureAttribute(PostProcessor::TextureUnits::Unit_Normals, mTextureNormals);

            if (mTextureDistortion)
                node.mRootStateSet->setTextureAttribute(
                    PostProcessor::TextureUnits::Unit_Distortion, mTextureDistortion);

            state.pushStateSet(node.mRootStateSet);
            state.apply();

            for (size_t passIndex = 0; passIndex < node.mPasses.size(); ++passIndex)
            {
                if (mRenderViewport)
                    mRenderViewport->apply(state);
                const auto& pass = node.mPasses[passIndex];

                bool lastPass = passIndex == node.mPasses.size() - 1;

                // VR-TODO: This won't actually work for tex2darrays
                if (lastShader == 0)
                    pass.mStateSet->setTextureAttribute(PostProcessor::Unit_LastShader, mTextureScene);
                else
                    pass.mStateSet->setTextureAttribute(PostProcessor::Unit_LastShader,
                        (osg::Texture*)mFbos[lastShader - GL_COLOR_ATTACHMENT0_EXT]
                            ->getAttachment(osg::Camera::COLOR_BUFFER0)
                            .getTexture());

                if (lastDraw == 0)
                    pass.mStateSet->setTextureAttribute(PostProcessor::Unit_LastPass, mTextureScene);
                else
                    pass.mStateSet->setTextureAttribute(PostProcessor::Unit_LastPass,
                        (osg::Texture*)mFbos[lastDraw - GL_COLOR_ATTACHMENT0_EXT]
                            ->getAttachment(osg::Camera::COLOR_BUFFER0)
                            .getTexture());

                if (pass.mRenderTarget)
                {
                    pass.mRenderTarget->apply(state, osg::FrameBufferObject::DRAW_FRAMEBUFFER);

                    lastApplied = pass.mRenderTarget->getHandle(state.getContextID());
                }
                else if (pass.mResolve && index == filtered.back())
                {
                    bindDestinationFbo();
                    if (!destinationFbo && !Stereo::getMultiview())
                    {
                        resolveViewport->apply(state);
                    }
                }
                else if (lastPass)
                {
                    lastDraw = buffer[0];
                    lastShader = buffer[0];
                    mFbos[buffer[0] - GL_COLOR_ATTACHMENT0_EXT]->apply(state, osg::FrameBufferObject::DRAW_FRAMEBUFFER);
                    buffer = buffers[lastShader - GL_COLOR_ATTACHMENT0_EXT];

                    lastApplied = mFbos[buffer[0] - GL_COLOR_ATTACHMENT0_EXT]->getHandle(cid);
                }
                else
                {
                    mFbos[buffer[0] - GL_COLOR_ATTACHMENT0_EXT]->apply(state, osg::FrameBufferObject::DRAW_FRAMEBUFFER);
                    lastDraw = buffer[0];
                    std::swap(buffer[0], buffer[1]);

                    lastApplied = mFbos[buffer[0] - GL_COLOR_ATTACHMENT0_EXT]->getHandle(cid);
                }

                state.pushStateSet(pass.mStateSet);
                state.apply();

                if (!state.getLastAppliedProgramObject())
                    mFallbackProgram->apply(state);

                drawGeometry(renderInfo);

                if (pass.mRenderTarget && pass.mRenderTexture->getNumMipmapLevels() > 0)
                {
                    state.setActiveTextureUnit(0);
                    state.applyTextureAttribute(0,
                        pass.mRenderTarget->getAttachment(osg::FrameBufferObject::BufferComponent::COLOR_BUFFER0)
                            .getTexture());
                    ext->glGenerateMipmap(GL_TEXTURE_2D);
                }

                state.popStateSet();
                state.apply();
            }

            state.popStateSet();
        }

        if (Stereo::getMultiview())
        {
            ext->glBindFramebuffer(GL_DRAW_FRAMEBUFFER_EXT, 0);
            lastApplied = 0;

            resolveViewport->apply(state);
            state.pushStateSet(mMultiviewResolveStateSet);
            state.apply();

            drawGeometry(renderInfo);

            state.popStateSet();
            state.apply();
        }

        if (lastApplied != destinationHandle)
        {
            bindDestinationFbo();
        }

        mDirtyAttachments.clear();
    }
}

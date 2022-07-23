#include "pingpongcanvas.hpp"

#include <components/shader/shadermanager.hpp>
#include <components/debug/debuglog.hpp>
#include <components/stereo/stereomanager.hpp>
#include <components/stereo/multiview.hpp>

#include <osg/Texture2DArray>

#include "postprocessor.hpp"

namespace MWRender
{
    PingPongCanvas::PingPongCanvas(Shader::ShaderManager& shaderManager)
        : mFallbackStateSet(new osg::StateSet)
        , mMultiviewResolveStateSet(new osg::StateSet)
    {
        setUseDisplayList(false);
        setUseVertexBufferObjects(true);

        osg::ref_ptr<osg::Vec3Array> verts = new osg::Vec3Array;
        verts->push_back(osg::Vec3f(-1, -1, 0));
        verts->push_back(osg::Vec3f(-1, 3, 0));
        verts->push_back(osg::Vec3f(3, -1, 0));

        setVertexArray(verts);

        addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::TRIANGLES, 0, 3));

        mLuminanceCalculator = LuminanceCalculator(shaderManager);
        mLuminanceCalculator.disable();

        Shader::ShaderManager::DefineMap defines;
        Stereo::Manager::instance().shaderStereoDefines(defines);

        auto fallbackVertex = shaderManager.getShader("fullscreen_tri_vertex.glsl", defines, osg::Shader::VERTEX);
        auto fallbackFragment = shaderManager.getShader("fullscreen_tri_fragment.glsl", defines, osg::Shader::FRAGMENT);
        mFallbackProgram = shaderManager.getProgram(fallbackVertex, fallbackFragment);

        mFallbackStateSet->setAttributeAndModes(mFallbackProgram);
        mFallbackStateSet->addUniform(new osg::Uniform("omw_SamplerLastShader", 0));

        auto multiviewResolveVertex = shaderManager.getShader("multiview_resolve_vertex.glsl", {}, osg::Shader::VERTEX);
        auto multiviewResolveFragment = shaderManager.getShader("multiview_resolve_fragment.glsl", {}, osg::Shader::FRAGMENT);
        mMultiviewResolveProgram = shaderManager.getProgram(multiviewResolveVertex, multiviewResolveFragment);
        mMultiviewResolveStateSet->setAttributeAndModes(mMultiviewResolveProgram);
        mMultiviewResolveStateSet->addUniform(new osg::Uniform("omw_SamplerLastShader", 0));
    }

    void PingPongCanvas::setCurrentFrameData(size_t frameId, fx::DispatchArray&& data)
    {
        mBufferData[frameId].data = std::move(data);
    }

    void PingPongCanvas::setMask(size_t frameId, bool underwater, bool exterior)
    {
        mBufferData[frameId].mask = 0;

        mBufferData[frameId].mask |= underwater ? fx::Technique::Flag_Disable_Underwater : fx::Technique::Flag_Disable_Abovewater;
        mBufferData[frameId].mask |= exterior ? fx::Technique::Flag_Disable_Exteriors : fx::Technique::Flag_Disable_Interiors;
    }

    void PingPongCanvas::drawGeometry(osg::RenderInfo& renderInfo) const
    {
        osg::Geometry::drawImplementation(renderInfo);
    }

    static void attachCloneOfTemplate(osg::FrameBufferObject* fbo, osg::Camera::BufferComponent component, osg::Texture* tex)
    {
        osg::ref_ptr<osg::Texture> clone = static_cast<osg::Texture*>(tex->clone(osg::CopyOp::SHALLOW_COPY));
        fbo->setAttachment(component, Stereo::createMultiviewCompatibleAttachment(clone));
    }

    void PingPongCanvas::drawImplementation(osg::RenderInfo& renderInfo) const
    {
        osg::State& state = *renderInfo.getState();
        osg::GLExtensions* ext = state.get<osg::GLExtensions>();

        size_t frameId = state.getFrameStamp()->getFrameNumber() % 2;

        auto& bufferData = mBufferData[frameId];

        const auto& data = bufferData.data;

        std::vector<size_t> filtered;

        filtered.reserve(data.size());

        for (size_t i = 0; i < data.size(); ++i)
        {
            const auto& node = data[i];

            if (bufferData.mask & node.mFlags)
                continue;

            filtered.push_back(i);
        }

        auto* resolveViewport = state.getCurrentViewport();

        if (filtered.empty() || !bufferData.postprocessing)
        {
            if (bufferData.postprocessing)
            {
                if (!mLoggedLastError)
                {
                    Log(Debug::Error) << "Critical error, postprocess shaders failed to compile. Using default shader.";
                    mLoggedLastError = true;
                }
            }
            else
                mLoggedLastError = false;

            state.pushStateSet(mFallbackStateSet);
            state.apply();

            if (Stereo::getMultiview() && mMultiviewResolveProgram)
            {
                state.pushStateSet(mMultiviewResolveStateSet);
                state.apply();
            }

            state.applyTextureAttribute(0, bufferData.sceneTex);
            resolveViewport->apply(state);

            drawGeometry(renderInfo);
            state.popStateSet();

            if (Stereo::getMultiview() && mMultiviewResolveProgram)
            {
                state.popStateSet();
            }

            return;
        }

        const unsigned int handle = mFbos[0] ? mFbos[0]->getHandle(state.getContextID()) : 0;

        if (handle == 0 || bufferData.dirty)
        {
            for (auto& fbo : mFbos)
            {
                fbo = new osg::FrameBufferObject;
                attachCloneOfTemplate(fbo, osg::FrameBufferObject::BufferComponent::COLOR_BUFFER0, bufferData.sceneTexLDR);
                fbo->apply(state);
                glClearColor(0.5, 0.5, 0.5, 1);
                glClear(GL_COLOR_BUFFER_BIT);
            }

            if (Stereo::getMultiview())
            {
                mMultiviewResolveFramebuffer = new osg::FrameBufferObject();
                attachCloneOfTemplate(mMultiviewResolveFramebuffer, osg::FrameBufferObject::BufferComponent::COLOR_BUFFER0, bufferData.sceneTexLDR);
                mMultiviewResolveFramebuffer->apply(state);
                glClearColor(0.5, 0.5, 0.5, 1);
                glClear(GL_COLOR_BUFFER_BIT);

                mMultiviewResolveStateSet->setTextureAttribute(PostProcessor::Unit_LastShader, (osg::Texture*)mMultiviewResolveFramebuffer->getAttachment(osg::Camera::COLOR_BUFFER0).getTexture());
            }

            mLuminanceCalculator.dirty(bufferData.sceneTex->getTextureWidth(), bufferData.sceneTex->getTextureHeight());

            if (Stereo::getStereo())
                mRenderViewport = new osg::Viewport(0, 0, bufferData.sceneTex->getTextureWidth(), bufferData.sceneTex->getTextureHeight());
            else
                mRenderViewport = nullptr;

            bufferData.dirty = false;
        }

        constexpr std::array<std::array<int, 2>, 3> buffers = {{
            {GL_COLOR_ATTACHMENT1_EXT, GL_COLOR_ATTACHMENT2_EXT},
            {GL_COLOR_ATTACHMENT0_EXT, GL_COLOR_ATTACHMENT2_EXT},
            {GL_COLOR_ATTACHMENT0_EXT, GL_COLOR_ATTACHMENT1_EXT}
        }};

        (bufferData.hdr) ? mLuminanceCalculator.enable() : mLuminanceCalculator.disable();

        // A histogram based approach is superior way to calculate scene luminance. Using mipmaps is more broadly supported, so that's what we use for now.
        mLuminanceCalculator.draw(*this, renderInfo, state, ext, frameId);

        auto buffer = buffers[0];

        int lastDraw = 0;
        int lastShader = 0;

        unsigned int lastApplied = handle;

        const unsigned int cid = state.getContextID();

        const osg::ref_ptr<osg::FrameBufferObject>& destinationFbo = bufferData.destination ? bufferData.destination : nullptr;
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

        for (const size_t& index : filtered)
        {
            const auto& node = data[index];

            node.mRootStateSet->setTextureAttribute(PostProcessor::Unit_Depth, bufferData.depthTex);

            if (bufferData.hdr)
                node.mRootStateSet->setTextureAttribute(PostProcessor::TextureUnits::Unit_EyeAdaptation, mLuminanceCalculator.getLuminanceTexture(frameId));

            if (bufferData.normalsTex)
                node.mRootStateSet->setTextureAttribute(PostProcessor::TextureUnits::Unit_Normals, bufferData.normalsTex);

            state.pushStateSet(node.mRootStateSet);
            state.apply();

            for (size_t passIndex = 0; passIndex < node.mPasses.size(); ++passIndex)
            {
                if (mRenderViewport)
                    mRenderViewport->apply(state);
                const auto& pass = node.mPasses[passIndex];

                bool lastPass = passIndex == node.mPasses.size() - 1;

                //VR-TODO: This won't actually work for tex2darrays
                if (lastShader == 0)
                    pass.mStateSet->setTextureAttribute(PostProcessor::Unit_LastShader, bufferData.sceneTex);
                else
                    pass.mStateSet->setTextureAttribute(PostProcessor::Unit_LastShader, (osg::Texture*)mFbos[lastShader - GL_COLOR_ATTACHMENT0_EXT]->getAttachment(osg::Camera::COLOR_BUFFER0).getTexture());

                if (lastDraw == 0)
                    pass.mStateSet->setTextureAttribute(PostProcessor::Unit_LastPass, bufferData.sceneTex);
                else
                    pass.mStateSet->setTextureAttribute(PostProcessor::Unit_LastPass, (osg::Texture*)mFbos[lastDraw - GL_COLOR_ATTACHMENT0_EXT]->getAttachment(osg::Camera::COLOR_BUFFER0).getTexture());

                if (pass.mRenderTarget)
                {
                    pass.mRenderTarget->apply(state, osg::FrameBufferObject::DRAW_FRAMEBUFFER);

                    if (pass.mRenderTexture->getNumMipmapLevels() > 0)
                    {
                        state.setActiveTextureUnit(0);
                        state.applyTextureAttribute(0, pass.mRenderTarget->getAttachment(osg::FrameBufferObject::BufferComponent::COLOR_BUFFER0).getTexture());
                        ext->glGenerateMipmap(GL_TEXTURE_2D);
                    }

                    lastApplied = pass.mRenderTarget->getHandle(state.getContextID());;
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

                state.popStateSet();
                state.apply();
            }

            state.popStateSet();
        }

        if (Stereo::getMultiview() && mMultiviewResolveProgram)
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
    }
}

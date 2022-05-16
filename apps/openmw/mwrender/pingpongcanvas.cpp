#include "pingpongcanvas.hpp"

#include <components/shader/shadermanager.hpp>
#include <components/debug/debuglog.hpp>

#include "postprocessor.hpp"

namespace MWRender
{
    PingPongCanvas::PingPongCanvas(Shader::ShaderManager& shaderManager)
        : mFallbackStateSet(new osg::StateSet)
        , mQueuedDispatchArray(std::nullopt)
        , mQueuedDispatchFrameId(0)
    {
        setUseDisplayList(false);
        setUseVertexBufferObjects(true);

        osg::ref_ptr<osg::Vec3Array> verts = new osg::Vec3Array;
        verts->push_back(osg::Vec3f(-1, -1, 0));
        verts->push_back(osg::Vec3f(-1, 3, 0));
        verts->push_back(osg::Vec3f(3, -1, 0));

        setVertexArray(verts);

        addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::TRIANGLES, 0, 3));

        mHDRDriver = HDRDriver(shaderManager);
        mHDRDriver.disable();

        auto fallbackVertex = shaderManager.getShader("fullscreen_tri_vertex.glsl", {}, osg::Shader::VERTEX);
        auto fallbackFragment = shaderManager.getShader("fullscreen_tri_fragment.glsl", {}, osg::Shader::FRAGMENT);
        mFallbackProgram = shaderManager.getProgram(fallbackVertex, fallbackFragment);

        mFallbackStateSet->setAttributeAndModes(mFallbackProgram);
        mFallbackStateSet->addUniform(new osg::Uniform("omw_SamplerLastShader", 0));
    }

    void PingPongCanvas::setCurrentFrameData(size_t frameId, fx::DispatchArray&& data)
    {
        mQueuedDispatchArray = fx::DispatchArray(data);
        mQueuedDispatchFrameId = !frameId;

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

    void PingPongCanvas::drawImplementation(osg::RenderInfo& renderInfo) const
    {
        osg::State& state = *renderInfo.getState();
        osg::GLExtensions* ext = state.get<osg::GLExtensions>();

        size_t frameId = state.getFrameStamp()->getFrameNumber() % 2;

        auto& bufferData = mBufferData[frameId];

        if (mQueuedDispatchArray && mQueuedDispatchFrameId == frameId)
        {
            mBufferData[frameId].data = std::move(mQueuedDispatchArray.value());
            mQueuedDispatchArray = std::nullopt;
        }

        const auto& data = bufferData.data;

        std::vector<size_t> filtered;

        filtered.reserve(data.size());

        const fx::DispatchNode::SubPass* resolvePass = nullptr;

        for (size_t i = 0; i < data.size(); ++i)
        {
            const auto& node = data[i];

            if (bufferData.mask & node.mFlags)
                continue;

            for (auto it = node.mPasses.crbegin(); it != node.mPasses.crend(); ++it)
            {
                if (!(*it).mRenderTarget)
                {
                    resolvePass = &(*it);
                    break;
                }
            }

            filtered.push_back(i);
        }

        auto* viewport = state.getCurrentViewport();

        if (filtered.empty() || !bufferData.postprocessing)
        {
            if (bufferData.postprocessing)
                Log(Debug::Error) << "Critical error, postprocess shaders failed to compile. Using default shader.";

            mFallbackStateSet->setTextureAttributeAndModes(0, bufferData.sceneTex);

            state.pushStateSet(mFallbackStateSet);
            state.apply();
            viewport->apply(state);

            drawGeometry(renderInfo);
            state.popStateSet();
            return;
        }

        const unsigned int handle = mFbos[0] ? mFbos[0]->getHandle(state.getContextID()) : 0;

        if (handle == 0 || bufferData.dirty)
        {
            for (auto& fbo : mFbos)
            {
                fbo = new osg::FrameBufferObject;
                fbo->setAttachment(osg::FrameBufferObject::BufferComponent::COLOR_BUFFER0, osg::FrameBufferAttachment(new osg::Texture2D(*bufferData.sceneTexLDR)));
                fbo->apply(state);
                glClearColor(0.5, 0.5, 0.5, 1);
                glClear(GL_COLOR_BUFFER_BIT);
            }

            mHDRDriver.dirty(bufferData.sceneTex->getTextureWidth(), bufferData.sceneTex->getTextureHeight());

            bufferData.dirty = false;
        }

        constexpr std::array<std::array<int, 2>, 3> buffers = {{
            {GL_COLOR_ATTACHMENT1_EXT, GL_COLOR_ATTACHMENT2_EXT},
            {GL_COLOR_ATTACHMENT0_EXT, GL_COLOR_ATTACHMENT2_EXT},
            {GL_COLOR_ATTACHMENT0_EXT, GL_COLOR_ATTACHMENT1_EXT}
        }};

        (bufferData.hdr) ? mHDRDriver.enable() : mHDRDriver.disable();

        // A histogram based approach is superior way to calculate scene luminance. Using mipmaps is more broadly supported, so that's what we use for now.
        mHDRDriver.draw(*this, renderInfo, state, ext, frameId);

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
                node.mRootStateSet->setTextureAttribute(PostProcessor::TextureUnits::Unit_EyeAdaptation, mHDRDriver.getLuminanceTexture(frameId));

            if (bufferData.normalsTex)
                node.mRootStateSet->setTextureAttribute(PostProcessor::TextureUnits::Unit_Normals, bufferData.normalsTex);

            state.pushStateSet(node.mRootStateSet);
            state.apply();

            for (size_t passIndex = 0; passIndex < node.mPasses.size(); ++passIndex)
            {
                const auto& pass = node.mPasses[passIndex];

                bool lastPass = passIndex == node.mPasses.size() - 1;

                if (lastShader == 0)
                    pass.mStateSet->setTextureAttribute(PostProcessor::Unit_LastShader, bufferData.sceneTex);
                else
                    pass.mStateSet->setTextureAttribute(PostProcessor::Unit_LastShader, (osg::Texture2D*)mFbos[lastShader - GL_COLOR_ATTACHMENT0_EXT]->getAttachment(osg::Camera::COLOR_BUFFER0).getTexture());

                if (lastDraw == 0)
                    pass.mStateSet->setTextureAttribute(PostProcessor::Unit_LastPass, bufferData.sceneTex);
                else
                    pass.mStateSet->setTextureAttribute(PostProcessor::Unit_LastPass, (osg::Texture2D*)mFbos[lastDraw - GL_COLOR_ATTACHMENT0_EXT]->getAttachment(osg::Camera::COLOR_BUFFER0).getTexture());

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
                else if (&pass == resolvePass)
                {
                    bindDestinationFbo();
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

        if (lastApplied != destinationHandle)
        {
            bindDestinationFbo();
        }
    }
}

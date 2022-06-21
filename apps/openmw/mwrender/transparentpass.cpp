#include "transparentpass.hpp"

#include <osg/BlendFunc>
#include <osg/Texture2D>
#include <osg/Texture2DArray>

#ifdef OSG_HAS_MULTIVIEW
#include <osg/Texture2DMultisampleArray>
#endif

#include <osgUtil/RenderStage>

#include <components/shader/shadermanager.hpp>
#include <components/stereo/multiview.hpp>
#include <components/stereo/stereomanager.hpp>

namespace MWRender
{
        TransparentDepthBinCallback::TransparentDepthBinCallback(Shader::ShaderManager& shaderManager, bool postPass)
            : mStateSet(new osg::StateSet)
            , mPostPass(postPass)
        {
            osg::ref_ptr<osg::Image> image = new osg::Image;
            image->allocateImage(1, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE);
            image->setColor(osg::Vec4(1,1,1,1), 0, 0);

            osg::ref_ptr<osg::Texture2D> dummyTexture = new osg::Texture2D(image);

            constexpr osg::StateAttribute::OverrideValue modeOff = osg::StateAttribute::OFF | osg::StateAttribute::OVERRIDE;
            constexpr osg::StateAttribute::OverrideValue modeOn = osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE;

            mStateSet->setTextureAttributeAndModes(0, dummyTexture);

            Shader::ShaderManager::DefineMap defines;
            Stereo::Manager::instance().shaderStereoDefines(defines);
            osg::ref_ptr<osg::Shader> vertex = shaderManager.getShader("blended_depth_postpass_vertex.glsl", defines, osg::Shader::VERTEX);
            osg::ref_ptr<osg::Shader> fragment = shaderManager.getShader("blended_depth_postpass_fragment.glsl", defines, osg::Shader::FRAGMENT);

            mStateSet->setAttributeAndModes(new osg::BlendFunc, modeOff);
            mStateSet->setAttributeAndModes(shaderManager.getProgram(vertex, fragment), modeOn);

            for (unsigned int unit = 1; unit < 8; ++unit)
                mStateSet->setTextureMode(unit, GL_TEXTURE_2D, modeOff);
        }

        void TransparentDepthBinCallback::drawImplementation(osgUtil::RenderBin* bin, osg::RenderInfo& renderInfo, osgUtil::RenderLeaf*& previous)
        {
            osg::State& state = *renderInfo.getState();
            osg::GLExtensions* ext = state.get<osg::GLExtensions>();

            bool validFbo = false;
            unsigned int frameId = state.getFrameStamp()->getFrameNumber() % 2;

            const auto& fbo = mFbo[frameId];
            const auto& msaaFbo = mMsaaFbo[frameId];
            const auto& opaqueFbo = mOpaqueFbo[frameId];

            if (bin->getStage()->getMultisampleResolveFramebufferObject() && bin->getStage()->getMultisampleResolveFramebufferObject() == fbo)
                validFbo = true;
            else if (bin->getStage()->getFrameBufferObject() && (bin->getStage()->getFrameBufferObject() == fbo || bin->getStage()->getFrameBufferObject() == msaaFbo))
                validFbo = true;

            if (!validFbo)
            {
                bin->drawImplementation(renderInfo, previous);
                return;
            }

            const osg::Texture* tex = opaqueFbo->getAttachment(osg::FrameBufferObject::BufferComponent::PACKED_DEPTH_STENCIL_BUFFER).getTexture();

            if (Stereo::getMultiview())
            {
                if (!mMultiviewDepthResolveLeftSource[frameId])
                    setupMultiviewDepthResolveBuffers(frameId);
                mMultiviewDepthResolveLeftTarget[frameId]->apply(state, osg::FrameBufferObject::BindTarget::DRAW_FRAMEBUFFER);
                mMultiviewDepthResolveLeftSource[frameId]->apply(state, osg::FrameBufferObject::BindTarget::READ_FRAMEBUFFER);
                ext->glBlitFramebuffer(0, 0, tex->getTextureWidth(), tex->getTextureHeight(), 0, 0, tex->getTextureWidth(), tex->getTextureHeight(), GL_DEPTH_BUFFER_BIT, GL_NEAREST);
                mMultiviewDepthResolveRightTarget[frameId]->apply(state, osg::FrameBufferObject::BindTarget::DRAW_FRAMEBUFFER);
                mMultiviewDepthResolveRightSource[frameId]->apply(state, osg::FrameBufferObject::BindTarget::READ_FRAMEBUFFER);
                ext->glBlitFramebuffer(0, 0, tex->getTextureWidth(), tex->getTextureHeight(), 0, 0, tex->getTextureWidth(), tex->getTextureHeight(), GL_DEPTH_BUFFER_BIT, GL_NEAREST);
            }
            else
            {
                opaqueFbo->apply(state, osg::FrameBufferObject::DRAW_FRAMEBUFFER);
                ext->glBlitFramebuffer(0, 0, tex->getTextureWidth(), tex->getTextureHeight(), 0, 0, tex->getTextureWidth(), tex->getTextureHeight(), GL_DEPTH_BUFFER_BIT, GL_NEAREST);
            }

            msaaFbo ? msaaFbo->apply(state, osg::FrameBufferObject::DRAW_FRAMEBUFFER) : fbo->apply(state, osg::FrameBufferObject::DRAW_FRAMEBUFFER);

            // draws scene into primary attachments
            bin->drawImplementation(renderInfo, previous);

            if (!mPostPass)
                return;

            opaqueFbo->apply(state, osg::FrameBufferObject::DRAW_FRAMEBUFFER);

            osg::ref_ptr<osg::StateSet> restore = bin->getStateSet();
            bin->setStateSet(mStateSet);
            // draws transparent post-pass to populate a postprocess friendly depth texture with alpha-clipped geometry
            bin->drawImplementation(renderInfo, previous);
            bin->setStateSet(restore);

            msaaFbo ? msaaFbo->apply(state, osg::FrameBufferObject::DRAW_FRAMEBUFFER) : fbo->apply(state, osg::FrameBufferObject::DRAW_FRAMEBUFFER);
            state.checkGLErrors("after TransparentDepthBinCallback::drawImplementation");
        }

        void TransparentDepthBinCallback::dirtyFrame(int frameId)
        {
            mMultiviewDepthResolveLeftSource[frameId] = mMultiviewDepthResolveRightSource[frameId] = nullptr;
            mMultiviewDepthResolveLeftTarget[frameId] = mMultiviewDepthResolveRightTarget[frameId] = nullptr;
        }

        osg::FrameBufferAttachment makeSingleLayerAttachmentFromMultilayerAttachment(osg::FrameBufferAttachment attachment, int layer)
        {
            osg::Texture* tex = attachment.getTexture();

            if (tex->getTextureTarget() == GL_TEXTURE_2D_ARRAY)
                return osg::FrameBufferAttachment(static_cast<osg::Texture2DArray*>(tex), layer, 0);

#ifdef OSG_HAS_MULTIVIEW
            if (tex->getTextureTarget() == GL_TEXTURE_2D_MULTISAMPLE_ARRAY)
                return osg::FrameBufferAttachment(static_cast<osg::Texture2DMultisampleArray*>(tex), layer, 0);
#endif

            Log(Debug::Error) << "Attempted to extract a layer from an unlayered texture";

            return osg::FrameBufferAttachment();
        }

        void TransparentDepthBinCallback::setupMultiviewDepthResolveBuffers(int frameId)
        {
            const osg::FrameBufferObject::BufferComponent component = osg::FrameBufferObject::BufferComponent::PACKED_DEPTH_STENCIL_BUFFER;
            const auto& sourceFbo = mMsaaFbo[frameId] ? mMsaaFbo[frameId] : mFbo[frameId];
            const auto& sourceAttachment = sourceFbo->getAttachment(component);
            mMultiviewDepthResolveLeftSource[frameId] = new osg::FrameBufferObject;
            mMultiviewDepthResolveLeftSource[frameId]->setAttachment(component, makeSingleLayerAttachmentFromMultilayerAttachment(sourceAttachment, 0));
            mMultiviewDepthResolveRightSource[frameId] = new osg::FrameBufferObject;
            mMultiviewDepthResolveRightSource[frameId]->setAttachment(component, makeSingleLayerAttachmentFromMultilayerAttachment(sourceAttachment, 1));
            const auto& targetFbo = mOpaqueFbo[frameId];
            const auto& targetAttachment = targetFbo->getAttachment(component);
            mMultiviewDepthResolveLeftTarget[frameId] = new osg::FrameBufferObject;
            mMultiviewDepthResolveLeftTarget[frameId]->setAttachment(component, makeSingleLayerAttachmentFromMultilayerAttachment(targetAttachment, 0));
            mMultiviewDepthResolveRightTarget[frameId] = new osg::FrameBufferObject;
            mMultiviewDepthResolveRightTarget[frameId]->setAttachment(component, makeSingleLayerAttachmentFromMultilayerAttachment(targetAttachment, 1));
        }

}

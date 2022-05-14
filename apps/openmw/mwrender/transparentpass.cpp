#include "transparentpass.hpp"

#include <osg/BlendFunc>
#include <osg/Texture2D>

#include <osgUtil/RenderStage>

#include <components/shader/shadermanager.hpp>

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

            osg::ref_ptr<osg::Shader> vertex = shaderManager.getShader("blended_depth_postpass_vertex.glsl", {}, osg::Shader::VERTEX);
            osg::ref_ptr<osg::Shader> fragment = shaderManager.getShader("blended_depth_postpass_fragment.glsl", {}, osg::Shader::FRAGMENT);

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

            opaqueFbo->apply(state, osg::FrameBufferObject::DRAW_FRAMEBUFFER);

            ext->glBlitFramebuffer(0, 0, tex->getTextureWidth(), tex->getTextureHeight(), 0, 0, tex->getTextureWidth(), tex->getTextureHeight(), GL_DEPTH_BUFFER_BIT, GL_NEAREST);

            if (msaaFbo)
                msaaFbo->apply(state, osg::FrameBufferObject::DRAW_FRAMEBUFFER);
            else
                fbo->apply(state, osg::FrameBufferObject::DRAW_FRAMEBUFFER);

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

            if (!msaaFbo)
                fbo->apply(state, osg::FrameBufferObject::DRAW_FRAMEBUFFER);
        }

}
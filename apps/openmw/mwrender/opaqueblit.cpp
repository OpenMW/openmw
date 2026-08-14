#include "opaqueblit.hpp"

#include <osgUtil/RenderStage>

#include <components/stereo/multiview.hpp>
#include <components/stereo/stereomanager.hpp>

namespace MWRender
{
    void OpaqueColorBinCallback::drawImplementation(
        osgUtil::RenderBin* bin, osg::RenderInfo& renderInfo, osgUtil::RenderLeaf*& previous)
    {
        resolve(bin, renderInfo);
        bin->drawImplementation(renderInfo, previous);
    }

    void OpaqueColorBinCallback::resolve(osgUtil::RenderBin* bin, osg::RenderInfo& renderInfo)
    {
        osg::State& state = *renderInfo.getState();
        osg::GLExtensions* ext = state.get<osg::GLExtensions>();
        const unsigned int frameId = state.getFrameStamp()->getFrameNumber() % 2;
        const auto& fbo = mFbo[frameId];
        const auto& msaaFbo = mMsaaFbo[frameId];
        const auto& opaqueFbo = mOpaqueFbo[frameId];

        if (!bin->getStage()->getFrameBufferObject()
            || (bin->getStage()->getFrameBufferObject() != fbo && bin->getStage()->getFrameBufferObject() != msaaFbo))
            return;

        const osg::Texture* colorTex
            = opaqueFbo->getAttachment(osg::FrameBufferObject::BufferComponent::COLOR_BUFFER0).getTexture();
        if (Stereo::getMultiview())
        {
            if (!mMultiviewResolve[frameId])
                mMultiviewResolve[frameId] = std::make_unique<Stereo::MultiviewFramebufferResolve>(
                    msaaFbo ? msaaFbo : fbo, opaqueFbo, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            else
            {
                mMultiviewResolve[frameId]->setResolveFbo(opaqueFbo);
                mMultiviewResolve[frameId]->setMsaaFbo(msaaFbo ? msaaFbo : fbo);
            }
            mMultiviewResolve[frameId]->resolveImplementation(state);
        }
        else
        {
            opaqueFbo->apply(state, osg::FrameBufferObject::DRAW_FRAMEBUFFER);
            ext->glBlitFramebuffer(0, 0, colorTex->getTextureWidth(), colorTex->getTextureHeight(), 0, 0,
                colorTex->getTextureWidth(), colorTex->getTextureHeight(), GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT,
                GL_NEAREST);
        }

        (msaaFbo ? msaaFbo : fbo)->apply(state, osg::FrameBufferObject::DRAW_FRAMEBUFFER);
    }
}

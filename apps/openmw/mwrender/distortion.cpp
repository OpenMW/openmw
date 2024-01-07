#include "distortion.hpp"

#include <osg/FrameBufferObject>

namespace MWRender
{
    void DistortionCallback::drawImplementation(
        osgUtil::RenderBin* bin, osg::RenderInfo& renderInfo, osgUtil::RenderLeaf*& previous)
    {
        osg::State* state = renderInfo.getState();
        size_t frameId = state->getFrameStamp()->getFrameNumber() % 2;

        mFBO[frameId]->apply(*state);

        const osg::Texture* tex
            = mFBO[frameId]->getAttachment(osg::FrameBufferObject::BufferComponent::COLOR_BUFFER0).getTexture();

        glViewport(0, 0, tex->getTextureWidth(), tex->getTextureHeight());
        glClearColor(0.0, 0.0, 0.0, 1.0);
        glClear(GL_COLOR_BUFFER_BIT);

        bin->drawImplementation(renderInfo, previous);

        tex = mOriginalFBO[frameId]->getAttachment(osg::FrameBufferObject::BufferComponent::COLOR_BUFFER0).getTexture();
        glViewport(0, 0, tex->getTextureWidth(), tex->getTextureHeight());
        mOriginalFBO[frameId]->apply(*state);
    }
}

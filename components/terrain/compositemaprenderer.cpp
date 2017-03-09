#include "compositemaprenderer.hpp"

#include <OpenThreads/ScopedLock>

#include <osg/FrameBufferObject>
#include <osg/Texture2D>
#include <osg/RenderInfo>

namespace Terrain
{

CompositeMapRenderer::CompositeMapRenderer()
    : mNumCompilePerFrame(1)
{
    setSupportsDisplayList(false);
    setCullingActive(false);

    mFBO = new osg::FrameBufferObject;

    getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
}

void CompositeMapRenderer::drawImplementation(osg::RenderInfo &renderInfo) const
{
    mCompiled.clear();

    unsigned int numCompiled = 0;

    OpenThreads::ScopedLock<OpenThreads::Mutex> lock(mMutex);

    if (mImmediateCompileSet.empty() && mCompileSet.empty())
        return;

    while (!mImmediateCompileSet.empty())
    {
        CompositeMap* node = *mImmediateCompileSet.begin();
        mCompiled.insert(node);

        compile(*node, renderInfo);

        mImmediateCompileSet.erase(mImmediateCompileSet.begin());

        ++numCompiled;
    }

    while (!mCompileSet.empty() && numCompiled <= mNumCompilePerFrame)
    {
        CompositeMap* node = *mCompileSet.begin();

        compile(*node, renderInfo);

        ++numCompiled;

        mCompiled.insert(node);
        mCompileSet.erase(mCompileSet.begin());
    }
}

void CompositeMapRenderer::compile(CompositeMap &compositeMap, osg::RenderInfo &renderInfo) const
{
    osg::State& state = *renderInfo.getState();
    osg::GLExtensions* ext = state.get<osg::GLExtensions>();

    if (!mFBO)
        return;

    if (!ext->isFrameBufferObjectSupported)
        return;

    osg::FrameBufferAttachment attach (compositeMap.mTexture);
    mFBO->setAttachment(osg::Camera::COLOR_BUFFER, attach);
    mFBO->apply(state, osg::FrameBufferObject::DRAW_FRAMEBUFFER);

    GLenum status = ext->glCheckFramebufferStatus(GL_FRAMEBUFFER_EXT);

    if (status != GL_FRAMEBUFFER_COMPLETE_EXT)
    {
        GLuint fboId = state.getGraphicsContext() ? state.getGraphicsContext()->getDefaultFboId() : 0;
        ext->glBindFramebuffer(GL_FRAMEBUFFER_EXT, fboId);
        OSG_ALWAYS << "Error attaching FBO" << std::endl;
        return;
    }

    for (unsigned int i=0; i<compositeMap.mDrawables.size(); ++i)
    {
        osg::Drawable* drw = compositeMap.mDrawables[i];
        osg::StateSet* stateset = drw->getStateSet();

        if (stateset)
            renderInfo.getState()->pushStateSet(stateset);

        renderInfo.getState()->apply();

        glViewport(0,0,compositeMap.mTexture->getTextureWidth(), compositeMap.mTexture->getTextureHeight());
        drw->drawImplementation(renderInfo);

        if (stateset)
            renderInfo.getState()->popStateSet();
    }

    state.haveAppliedAttribute(osg::StateAttribute::VIEWPORT);

    GLuint fboId = state.getGraphicsContext() ? state.getGraphicsContext()->getDefaultFboId() : 0;
    ext->glBindFramebuffer(GL_FRAMEBUFFER_EXT, fboId);
}

void CompositeMapRenderer::setNumCompilePerFrame(int num)
{
    mNumCompilePerFrame = num;
}

void CompositeMapRenderer::addCompositeMap(CompositeMap* compositeMap, bool immediate)
{
    OpenThreads::ScopedLock<OpenThreads::Mutex> lock(mMutex);
    if (immediate)
        mImmediateCompileSet.insert(compositeMap);
    else
        mCompileSet.insert(compositeMap);
}

void CompositeMapRenderer::setImmediate(CompositeMap* compositeMap)
{
    OpenThreads::ScopedLock<OpenThreads::Mutex> lock(mMutex);
    mImmediateCompileSet.insert(compositeMap);
    mCompileSet.erase(compositeMap);
}

CompositeMap::~CompositeMap()
{

}



}

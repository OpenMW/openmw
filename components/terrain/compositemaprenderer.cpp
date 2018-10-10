#include "compositemaprenderer.hpp"

#include <OpenThreads/ScopedLock>

#include <osg/FrameBufferObject>
#include <osg/Texture2D>
#include <osg/RenderInfo>

#include <algorithm>

namespace Terrain
{

CompositeMapRenderer::CompositeMapRenderer()
    : mTargetFrameRate(120)
    , mMinimumTimeAvailable(0.0025)
{
    setSupportsDisplayList(false);
    setCullingActive(false);

    mFBO = new osg::FrameBufferObject;

    getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
}

void CompositeMapRenderer::drawImplementation(osg::RenderInfo &renderInfo) const
{
    double dt = mTimer.time_s();
    dt = std::min(dt, 0.2);
    mTimer.setStartTick();
    double targetFrameTime = 1.0/static_cast<double>(mTargetFrameRate);
    double conservativeTimeRatio(0.75);
    double availableTime = std::max((targetFrameTime - dt)*conservativeTimeRatio,
                                    mMinimumTimeAvailable);

    mCompiled.clear();

    OpenThreads::ScopedLock<OpenThreads::Mutex> lock(mMutex);

    if (mImmediateCompileSet.empty() && mCompileSet.empty())
        return;

    while (!mImmediateCompileSet.empty())
    {
        CompositeMap* node = *mImmediateCompileSet.begin();
        mCompiled.insert(node);

        compile(*node, renderInfo, nullptr);

        mImmediateCompileSet.erase(mImmediateCompileSet.begin());
    }

    double timeLeft = availableTime;

    while (!mCompileSet.empty() && timeLeft > 0)
    {
        CompositeMap* node = *mCompileSet.begin();

        compile(*node, renderInfo, &timeLeft);

        if (node->mCompiled >= node->mDrawables.size())
        {
            mCompiled.insert(node);
            mCompileSet.erase(mCompileSet.begin());
        }
    }
    mTimer.setStartTick();
}

void CompositeMapRenderer::compile(CompositeMap &compositeMap, osg::RenderInfo &renderInfo, double* timeLeft) const
{
    // if there are no more external references we can assume the texture is no longer required
    if (compositeMap.mTexture->referenceCount() <= 1)
    {
        compositeMap.mCompiled = compositeMap.mDrawables.size();
        return;
    }

    osg::Timer timer;
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

    // inform State that Texture attribute has changed due to compiling of FBO texture
    // should OSG be doing this on its own?
    state.haveAppliedTextureAttribute(state.getActiveTextureUnit(), osg::StateAttribute::TEXTURE);

    for (unsigned int i=compositeMap.mCompiled; i<compositeMap.mDrawables.size(); ++i)
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

        ++compositeMap.mCompiled;

        if (timeLeft)
        {
            *timeLeft -= timer.time_s();
            timer.setStartTick();

            if (*timeLeft <= 0)
                break;
        }
    }

    state.haveAppliedAttribute(osg::StateAttribute::VIEWPORT);

    GLuint fboId = state.getGraphicsContext() ? state.getGraphicsContext()->getDefaultFboId() : 0;
    ext->glBindFramebuffer(GL_FRAMEBUFFER_EXT, fboId);
}

void CompositeMapRenderer::setMinimumTimeAvailableForCompile(double time)
{
    mMinimumTimeAvailable = time;
}

void CompositeMapRenderer::setTargetFrameRate(float framerate)
{
    mTargetFrameRate = framerate;
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
    CompileSet::iterator found = mCompileSet.find(compositeMap);
    if (found == mCompileSet.end())
        return;
    else
    {
        mImmediateCompileSet.insert(compositeMap);
        mCompileSet.erase(found);
    }
}

unsigned int CompositeMapRenderer::getCompileSetSize() const
{
    OpenThreads::ScopedLock<OpenThreads::Mutex> lock(mMutex);
    return mCompileSet.size();
}

CompositeMap::CompositeMap()
    : mCompiled(0)
{
}

CompositeMap::~CompositeMap()
{

}



}

#include "compositemaprenderer.hpp"

#include <OpenThreads/ScopedLock>

#include <osg/NodeVisitor>

namespace Terrain
{

CompositeMapRenderer::CompositeMapRenderer()
    : mNumCompilePerFrame(1)
    , mLastFrame(0)
{
    setCullingActive(false);
}

void CompositeMapRenderer::traverse(osg::NodeVisitor &nv)
{
    if (nv.getVisitorType() != osg::NodeVisitor::CULL_VISITOR)
        return;

    if (mLastFrame == nv.getTraversalNumber())
        return;
    mLastFrame = nv.getTraversalNumber();

    mCompiled.clear();

    unsigned int numCompiled = 0;

    OpenThreads::ScopedLock<OpenThreads::Mutex> lock(mMutex);

    while (!mImmediateCompileSet.empty())
    {
        osg::Node* node = *mImmediateCompileSet.begin();
        mCompiled.insert(node);

        node->accept(nv);
        mImmediateCompileSet.erase(mImmediateCompileSet.begin());

        ++numCompiled;
    }

    while (!mCompileSet.empty() && numCompiled <= mNumCompilePerFrame)
    {
        osg::Node* node = *mCompileSet.begin();
        mCompiled.insert(node);

        node->accept(nv);
        mCompileSet.erase(mCompileSet.begin());

        ++numCompiled;
    }
}

void CompositeMapRenderer::setNumCompilePerFrame(int num)
{
    mNumCompilePerFrame = num;
}

void CompositeMapRenderer::addCompositeMap(osg::Node *node, bool immediate)
{
    OpenThreads::ScopedLock<OpenThreads::Mutex> lock(mMutex);
    if (immediate)
        mImmediateCompileSet.insert(node);
    else
        mCompileSet.insert(node);
}

void CompositeMapRenderer::setImmediate(osg::Node *node)
{
    OpenThreads::ScopedLock<OpenThreads::Mutex> lock(mMutex);
    mImmediateCompileSet.insert(node);
    mCompileSet.erase(node);
}



}

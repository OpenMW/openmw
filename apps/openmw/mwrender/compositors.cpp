#include "compositors.hpp"

#include <OgreViewport.h>
#include <OgreCompositorManager.h>
#include <OgreCompositorChain.h>
#include <OgreCompositionTargetPass.h>

using namespace MWRender;

Compositors::Compositors(Ogre::Viewport* vp) :
      mViewport(vp)
    , mEnabled(true)
{
}

Compositors::~Compositors()
{
    Ogre::CompositorManager::getSingleton().removeCompositorChain(mViewport);
}

void Compositors::setEnabled (const bool enabled)
{
    for (CompositorMap::iterator it=mCompositors.begin();
        it != mCompositors.end(); ++it)
    {
        Ogre::CompositorManager::getSingleton().setCompositorEnabled(mViewport, it->first, enabled && it->second.first);
    }
    mEnabled = enabled;
}

void Compositors::recreate()
{
    Ogre::CompositorManager::getSingleton().removeCompositorChain(mViewport);

    CompositorMap temp = mCompositors;
    mCompositors.clear();

    for (CompositorMap::iterator it=temp.begin();
        it != temp.end(); ++it)
    {
        addCompositor(it->first, it->second.second);
        setCompositorEnabled(it->first, mEnabled && it->second.first);
    }
}

void Compositors::addCompositor (const std::string& name, const int priority)
{
    int id = 0;

    for (CompositorMap::iterator it=mCompositors.begin();
        it != mCompositors.end(); ++it)
    {
        if (it->second.second > priority)
            break;
        ++id;
    }
    Ogre::CompositorManager::getSingleton().addCompositor (mViewport, name, id);

    mCompositors[name] = std::make_pair(false, priority);
}

void Compositors::setCompositorEnabled (const std::string& name, const bool enabled)
{
    mCompositors[name].first = enabled;
    Ogre::CompositorManager::getSingleton().setCompositorEnabled (mViewport, name, enabled && mEnabled);
}

void Compositors::removeAll()
{
    Ogre::CompositorManager::getSingleton().removeCompositorChain(mViewport);

    mCompositors.clear();
}

bool Compositors::anyCompositorEnabled()
{
    for (CompositorMap::iterator it=mCompositors.begin();
        it != mCompositors.end(); ++it)
    {
        if (it->second.first && mEnabled)
            return true;
    }
    return false;
}

void Compositors::countTrianglesBatches(unsigned int &triangles, unsigned int &batches)
{
    triangles = 0;
    batches = 0;

    Ogre::CompositorInstance* c = NULL;
    Ogre::CompositorChain* chain = Ogre::CompositorManager::getSingleton().getCompositorChain (mViewport);
    // accumulate tris & batches from all compositors with all their render targets
    for (unsigned int i=0; i < chain->getNumCompositors(); ++i)
    {
        if (chain->getCompositor(i)->getEnabled())
        {
            c = chain->getCompositor(i);
            for (unsigned int j = 0; j < c->getTechnique()->getNumTargetPasses(); ++j)
            {
                std::string textureName = c->getTechnique()->getTargetPass(j)->getOutputName();
                Ogre::RenderTarget* rt = c->getRenderTarget(textureName);
                triangles += rt->getTriangleCount();
                batches += rt->getBatchCount();
            }
        }
    }
}

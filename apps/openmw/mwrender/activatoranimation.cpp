#include "activatoranimation.hpp"

#include <OgreSceneNode.h>
#include <OgreParticleSystem.h>

#include <components/esm/loadacti.hpp>

#include "renderconst.hpp"

namespace MWRender
{

ActivatorAnimation::~ActivatorAnimation()
{
}

ActivatorAnimation::ActivatorAnimation(const MWWorld::Ptr &ptr, const std::string& model)
  : Animation(ptr, ptr.getRefData().getBaseNode())
{
    if(!model.empty())
    {
        setObjectRoot(model, false);
        setRenderProperties(mObjectRoot, RV_Misc, RQG_Main, RQG_Alpha);

        addAnimSource(model);
    }
    else
    {
        // No model given. Create an object root anyway, so that lights can be added to it if needed.
        mObjectRoot = NifOgre::ObjectScenePtr (new NifOgre::ObjectScene(mInsert->getCreator()));
    }
}

void ActivatorAnimation::addLight(const ESM::Light *light)
{
    addExtraLight(mInsert->getCreator(), mObjectRoot, light);
}

void ActivatorAnimation::removeParticles()
{
    for (unsigned int i=0; i<mObjectRoot->mParticles.size(); ++i)
    {
        // Don't destroyParticleSystem, the ParticleSystemController is still holding a pointer to it.
        // Don't setVisible, this could conflict with a VisController.
        // The following will remove all spawned particles, then set the speed factor to zero so that no new ones will be spawned.
        mObjectRoot->mParticles[i]->setSpeedFactor(0.f);
        mObjectRoot->mParticles[i]->clear();
    }
}

}

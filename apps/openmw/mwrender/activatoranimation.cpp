#include "activatoranimation.hpp"

#include <OgreEntity.h>
#include <OgreParticleSystem.h>
#include <OgreSceneManager.h>
#include <OgreSubEntity.h>

#include "renderconst.hpp"

#include "../mwbase/world.hpp"

namespace MWRender
{

ActivatorAnimation::~ActivatorAnimation()
{
}

ActivatorAnimation::ActivatorAnimation(const MWWorld::Ptr &ptr)
  : Animation(ptr)
{
    MWWorld::LiveCellRef<ESM::Activator> *ref = mPtr.get<ESM::Activator>();

    assert (ref->mBase != NULL);
    if(!ref->mBase->mModel.empty())
    {
        std::string mesh = "meshes\\" + ref->mBase->mModel;

        createObjectList(mPtr.getRefData().getBaseNode(), mesh);
        for(size_t i = 0;i < mObjectLists[0].mEntities.size();i++)
        {
            Ogre::Entity *ent = mObjectLists[0].mEntities[i];
            ent->setVisibilityFlags(RV_Misc);

            for(unsigned int j=0; j < ent->getNumSubEntities(); ++j)
            {
                Ogre::SubEntity* subEnt = ent->getSubEntity(j);
                subEnt->setRenderQueueGroup(subEnt->getMaterial()->isTransparent() ? RQG_Alpha : RQG_Main);
            }
        }
        for(size_t i = 0;i < mObjectLists[0].mParticles.size();i++)
        {
            Ogre::ParticleSystem *part = mObjectLists[0].mParticles[i];
            part->setVisibilityFlags(RV_Misc);
            part->setRenderQueueGroup(RQG_Alpha);
        }
    }
}

}

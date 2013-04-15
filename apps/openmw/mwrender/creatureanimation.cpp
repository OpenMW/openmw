#include "creatureanimation.hpp"

#include <OgreEntity.h>
#include <OgreParticleSystem.h>
#include <OgreSceneManager.h>
#include <OgreSubEntity.h>

#include "renderconst.hpp"

#include "../mwbase/world.hpp"

namespace MWRender
{

CreatureAnimation::~CreatureAnimation()
{
}

CreatureAnimation::CreatureAnimation(const MWWorld::Ptr &ptr)
  : Animation(ptr)
{
    MWWorld::LiveCellRef<ESM::Creature> *ref = mPtr.get<ESM::Creature>();

    assert (ref->mBase != NULL);
    if(!ref->mBase->mModel.empty())
    {
        std::string model = "meshes\\"+ref->mBase->mModel;

        std::vector<std::string> names;
        if((ref->mBase->mFlags&ESM::Creature::Biped))
            names.push_back("meshes\\base_anim.nif");
        names.push_back(model);

        createObjectList(mPtr.getRefData().getBaseNode(), model/*names*/);
        for(size_t i = 0;i < mObjectLists[0].mEntities.size();i++)
        {
            Ogre::Entity *ent = mObjectLists[0].mEntities[i];
            ent->setVisibilityFlags(RV_Actors);

            for(unsigned int j=0; j < ent->getNumSubEntities(); ++j)
            {
                Ogre::SubEntity* subEnt = ent->getSubEntity(j);
                subEnt->setRenderQueueGroup(subEnt->getMaterial()->isTransparent() ? RQG_Alpha : RQG_Main);
            }
        }
        for(size_t i = 0;i < mObjectLists[0].mParticles.size();i++)
        {
            Ogre::ParticleSystem *part = mObjectLists[0].mParticles[i];
            part->setVisibilityFlags(RV_Actors);

            part->setRenderQueueGroup(RQG_Alpha);
        }
    }
}

}

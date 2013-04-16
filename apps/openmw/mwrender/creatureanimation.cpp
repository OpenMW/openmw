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

        if((ref->mBase->mFlags&ESM::Creature::Biped))
            addObjectList(mPtr.getRefData().getBaseNode(), "meshes\\base_anim.nif", true);

        addObjectList(mPtr.getRefData().getBaseNode(), model, false);

        const NifOgre::ObjectList &objlist = mObjectLists.back();
        for(size_t i = 0;i < objlist.mEntities.size();i++)
        {
            Ogre::Entity *ent = objlist.mEntities[i];
            ent->setVisibilityFlags(RV_Actors);

            for(unsigned int j=0; j < ent->getNumSubEntities(); ++j)
            {
                Ogre::SubEntity* subEnt = ent->getSubEntity(j);
                subEnt->setRenderQueueGroup(subEnt->getMaterial()->isTransparent() ? RQG_Alpha : RQG_Main);
            }
        }
        for(size_t i = 0;i < objlist.mParticles.size();i++)
        {
            Ogre::ParticleSystem *part = objlist.mParticles[i];
            part->setVisibilityFlags(RV_Actors);
            part->setRenderQueueGroup(RQG_Alpha);
        }
    }
}

}

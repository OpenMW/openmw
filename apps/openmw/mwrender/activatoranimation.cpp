#include "activatoranimation.hpp"

#include <OgreEntity.h>
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

        createEntityList(mPtr.getRefData().getBaseNode(), mesh);
        for(size_t i = 0;i < mEntityList.mEntities.size();i++)
        {
            Ogre::Entity *ent = mEntityList.mEntities[i];

            for(unsigned int j=0; j < ent->getNumSubEntities(); ++j)
            {
                Ogre::SubEntity* subEnt = ent->getSubEntity(j);
                subEnt->setRenderQueueGroup(subEnt->getMaterial()->isTransparent() ? RQG_Alpha : RQG_Main);
            }

            ent->setVisibilityFlags(RV_Misc);
        }
        setAnimationSource(mesh);
    }
}

}

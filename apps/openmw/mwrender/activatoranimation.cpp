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

            bool transparent = false;
            for (unsigned int j=0;j < ent->getNumSubEntities() && !transparent; ++j)
            {
                Ogre::MaterialPtr mat = ent->getSubEntity(j)->getMaterial();
                Ogre::Material::TechniqueIterator techIt = mat->getTechniqueIterator();
                while (techIt.hasMoreElements() && !transparent)
                {
                    Ogre::Technique* tech = techIt.getNext();
                    Ogre::Technique::PassIterator passIt = tech->getPassIterator();
                    while (passIt.hasMoreElements() && !transparent)
                    {
                        Ogre::Pass* pass = passIt.getNext();

                        if (pass->getDepthWriteEnabled() == false)
                            transparent = true;
                    }
                }
            }
            ent->setVisibilityFlags(RV_Misc);
            ent->setRenderQueueGroup(transparent ? RQG_Alpha : RQG_Main);
        }
        setAnimationSource(mesh);
    }
}

}

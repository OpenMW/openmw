#include "creatureanimation.hpp"

#include <OgreEntity.h>
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

        createEntityList(mPtr.getRefData().getBaseNode(), model);
        for(size_t i = 0;i < mEntityList.mEntities.size();i++)
        {
            Ogre::Entity *ent = mEntityList.mEntities[i];
            ent->setVisibilityFlags(RV_Actors);

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
                        if(pass->getSourceBlendFactor() != Ogre::SBF_ONE || pass->getDestBlendFactor() != Ogre::SBF_ZERO)
                            transparent = true;
                    }
                }
            }
            ent->setRenderQueueGroup(transparent ? RQG_Alpha : RQG_Main);
        }

        std::vector<std::string> names;
        if((ref->mBase->mFlags&ESM::Creature::Biped))
            names.push_back("meshes\\base_anim.nif");
        names.push_back(model);
        setAnimationSources(names);
    }
}

}

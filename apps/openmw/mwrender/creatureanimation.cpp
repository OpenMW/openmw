#include "creatureanimation.hpp"

#include <OgreEntity.h>
#include <OgreSceneManager.h>
#include <OgreSubEntity.h>

#include "renderconst.hpp"

#include "../mwbase/world.hpp"

using namespace Ogre;
using namespace NifOgre;
namespace MWRender{

CreatureAnimation::~CreatureAnimation()
{
}

CreatureAnimation::CreatureAnimation(const MWWorld::Ptr& ptr, OEngine::Render::OgreRenderer& _rend): Animation(_rend)
{
    mInsert = ptr.getRefData().getBaseNode();
    MWWorld::LiveCellRef<ESM::Creature> *ref = ptr.get<ESM::Creature>();

    assert (ref->base != NULL);
    if(!ref->base->model.empty())
    {
        std::string mesh = "meshes\\" + ref->base->model;

        mEntityList = NifOgre::NIFLoader::createEntities(mInsert, &mTextKeys, mesh);
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

                        if (pass->getDepthWriteEnabled() == false)
                            transparent = true;
                    }
                }
            }
            ent->setRenderQueueGroup(transparent ? RQG_Alpha : RQG_Main);
        }

        if(mEntityList.mSkelBase)
        {
            Ogre::AnimationStateSet *aset = mEntityList.mSkelBase->getAllAnimationStates();
            Ogre::AnimationStateIterator as = aset->getAnimationStateIterator();
            while(as.hasMoreElements())
            {
                Ogre::AnimationState *state = as.getNext();
                state->setEnabled(true);
                state->setLoop(false);
            }
        }
    }
}

void CreatureAnimation::runAnimation(float timepassed)
{
    // Placeholder

    Animation::runAnimation(timepassed);
}

}

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
    insert = ptr.getRefData().getBaseNode();
    MWWorld::LiveCellRef<ESM::Creature> *ref = ptr.get<ESM::Creature>();

    assert (ref->base != NULL);
    if(!ref->base->model.empty())
    {
        std::string mesh = "meshes\\" + ref->base->model;

        NifOgre::NIFLoader::load(mesh);
        base = mRend.getScene()->createEntity(mesh);
        base->setVisibilityFlags(RV_Actors);

        bool transparent = false;
        for (unsigned int i=0; i<base->getNumSubEntities(); ++i)
        {
            Ogre::MaterialPtr mat = base->getSubEntity(i)->getMaterial();
            Ogre::Material::TechniqueIterator techIt = mat->getTechniqueIterator();
            while (techIt.hasMoreElements())
            {
                Ogre::Technique* tech = techIt.getNext();
                Ogre::Technique::PassIterator passIt = tech->getPassIterator();
                while (passIt.hasMoreElements())
                {
                    Ogre::Pass* pass = passIt.getNext();

                    if (pass->getDepthWriteEnabled() == false)
                        transparent = true;
                }
            }
        }
        base->setRenderQueueGroup(transparent ? RQG_Alpha : RQG_Main);

        insert->attachObject(base);
    }
}

void CreatureAnimation::runAnimation(float timepassed)
{
    if(animate > 0)
    {
        //Add the amount of time passed to time

        //Handle the animation transforms dependent on time

        //Handle the shapes dependent on animation transforms
        time += timepassed;
        if(time >= stopTime)
        {
            animate--;
            //std::cout << "Stopping the animation\n";
            if(animate == 0)
                time = stopTime;
            else
                time = startTime + (time - stopTime);
        }

        handleAnimationTransforms();
    }
}

}

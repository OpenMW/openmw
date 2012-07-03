#include "creatureanimation.hpp"
#include "renderconst.hpp"

#include "../mwbase/world.hpp"

using namespace Ogre;
using namespace NifOgre;
namespace MWRender{

CreatureAnimation::~CreatureAnimation(){

}
CreatureAnimation::CreatureAnimation(const MWWorld::Ptr& ptr, OEngine::Render::OgreRenderer& _rend): Animation(_rend){
    insert = ptr.getRefData().getBaseNode();
    MWWorld::LiveCellRef<ESM::Creature> *ref =
            ptr.get<ESM::Creature>();

    assert (ref->base != NULL);
    if(!ref->base->model.empty()){
        const std::string &mesh = "meshes\\" + ref->base->model;
        std::string meshNumbered = mesh + getUniqueID(mesh) + ">|";
        NifOgre::NIFLoader::load(meshNumbered);
        base = mRend.getScene()->createEntity(meshNumbered);
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

        std::string meshZero = mesh + "0000>|";

        if((transformations = (NIFLoader::getSingletonPtr())->getAnim(meshZero))){

        for(std::size_t init = 0; init < transformations->size(); init++){
				rindexI.push_back(0);
				tindexI.push_back(0);
			}
        stopTime = transformations->begin()->getStopTime();
		startTime = transformations->begin()->getStartTime();
		shapes = (NIFLoader::getSingletonPtr())->getShapes(meshZero);
        }
        textmappings = NIFLoader::getSingletonPtr()->getTextIndices(meshZero);
        insert->attachObject(base);
    }
}

void CreatureAnimation::runAnimation(float timepassed){
    vecRotPos.clear();
	if(animate > 0){
		//Add the amount of time passed to time

		//Handle the animation transforms dependent on time

		//Handle the shapes dependent on animation transforms
        time += timepassed;
        if(time >= stopTime){
            animate--;
            //std::cout << "Stopping the animation\n";
            if(animate == 0)
                time = stopTime;
            else
                time = startTime + (time - stopTime);
        }

        handleAnimationTransforms();
        handleShapes(shapes, base, base->getSkeleton());

	}
}
}

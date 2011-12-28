#include "creatureanimation.hpp"

#include "../mwworld/world.hpp"

using namespace Ogre;
using namespace NifOgre;
namespace MWRender{

CreatureAnimation::~CreatureAnimation(){

}
CreatureAnimation::CreatureAnimation(const MWWorld::Ptr& ptr, MWWorld::Environment& _env,OEngine::Render::OgreRenderer& _rend): Animation(_env,_rend){
    Ogre::SceneNode* insert = ptr.getRefData().getBaseNode();
    assert(insert);
    ESMS::LiveCellRef<ESM::Creature, MWWorld::RefData> *ref =
            ptr.get<ESM::Creature>();

   assert (ref->base != NULL);
    if(!ref->base->model.empty()){
        const std::string &mesh = "meshes\\" + ref->base->model;
        std::string meshNumbered = mesh + getUniqueID(mesh) + ">|";
        NifOgre::NIFLoader::load(meshNumbered);
        base = mRend.getScene()->createEntity(meshNumbered);
        std::string meshZero = mesh + "0000>|";

        if(transformations = (NIFLoader::getSingletonPtr())->getAnim(meshZero)){

        for(int init = 0; init < transformations->size(); init++){
				rindexI.push_back(0);
				//a.rindexJ.push_back(0);
				tindexI.push_back(0);
				//a.tindexJ.push_back(0);
			}
        loop = false;
        stopTime = transformations->begin()->getStopTime();
			//a.startTime = NIFLoader::getSingletonPtr()->getTime(item.smodel, "IdleSneak: Start");
				startTime = transformations->begin()->getStartTime();
		shapes = (NIFLoader::getSingletonPtr())->getShapes(meshZero);
    }
        insert->attachObject(base);
    }
}

void CreatureAnimation::runAnimation(float timepassed){
	if(animate){
		//Add the amount of time passed to time

		//Handle the animation transforms dependent on time

		//Handle the shapes dependent on animation transforms
        time += timepassed;
        if(time > transformations->begin()->getStopTime()){
            animate = false;
            std::cout << "Stopping the animation\n";
            return;
        }
            
        handleAnimationTransforms();
        handleShapes(shapes, base, base->getSkeleton());

	}
}
}
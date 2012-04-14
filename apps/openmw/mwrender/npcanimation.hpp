#ifndef _GAME_RENDER_NPCANIMATION_H
#define _GAME_RENDER_NPCANIMATION_H
#include "animation.hpp"
#include <components/nif/data.hpp>
#include <components/nif/node.hpp>
#include <components/nif/property.hpp>
#include <components/nif/controller.hpp>
#include <components/nif/extra.hpp>
#include <utility>

#include "../mwworld/refdata.hpp"
#include "../mwworld/ptr.hpp"
#include "../mwworld/environment.hpp"
#include "components/nifogre/ogre_nif_loader.hpp"
#include "../mwworld/inventorystore.hpp"
#include "../mwclass/npc.hpp"
#include "../mwworld/containerstore.hpp"
#include "components/esm/loadarmo.hpp"

namespace MWRender{

class NpcAnimation: public Animation{
private:
	MWWorld::InventoryStore& inv;
	int mStateID;
	//Free Parts
	   std::pair<Ogre::Entity*, std::vector<Nif::NiTriShapeCopy>*> chest;
	   std::pair<Ogre::Entity*, std::vector<Nif::NiTriShapeCopy>*> skirt;
	   std::pair<Ogre::Entity*, std::vector<Nif::NiTriShapeCopy>*> lhand;
	   std::pair<Ogre::Entity*, std::vector<Nif::NiTriShapeCopy>*> rhand;
	   std::pair<Ogre::Entity*, std::vector<Nif::NiTriShapeCopy>*> tail;
	   std::pair<Ogre::Entity*, std::vector<Nif::NiTriShapeCopy>*> lBeastFoot;
	   std::pair<Ogre::Entity*, std::vector<Nif::NiTriShapeCopy>*> rBeastFoot;

       int partslots[27];  //Each part slot is taken by clothing, armor, or is empty
       int partpriorities[27];
       std::pair<Ogre::Entity*, std::vector<Nif::NiTriShapeCopy>*> zero;

	
	
	//Bounded Parts
	Ogre::Entity* lclavicle;
	Ogre::Entity* rclavicle;
	Ogre::Entity* rupperArm;
	Ogre::Entity* lupperArm;
	Ogre::Entity* rUpperLeg;
	Ogre::Entity* lUpperLeg;
	Ogre::Entity* lForearm;
	Ogre::Entity* rForearm;
	Ogre::Entity* lWrist;
	Ogre::Entity* rWrist;
	Ogre::Entity* rKnee;
	Ogre::Entity* lKnee;
	Ogre::Entity* neck;
	Ogre::Entity* rAnkle;
	Ogre::Entity* lAnkle;
	Ogre::Entity* groin;
	Ogre::Entity* lfoot;
	Ogre::Entity* rfoot;
	Ogre::Entity* hair;
	Ogre::Entity* head;
    
	Ogre::SceneNode* insert;
    bool isBeast;
	std::string headID;
	std::string hairID;
	std::string npcName;
	std::string bodyRaceID;
	float timeToChange;
	MWWorld::ContainerStoreIterator robe;
    MWWorld::ContainerStoreIterator helmet;
    
    public:
     NpcAnimation(const MWWorld::Ptr& ptr, MWWorld::Environment& _env, OEngine::Render::OgreRenderer& _rend, MWWorld::InventoryStore& _inv);
     virtual ~NpcAnimation();
    Ogre::Entity* insertBoundedPart(const std::string &mesh, std::string bonename);
     std::pair<Ogre::Entity*, std::vector<Nif::NiTriShapeCopy>*> insertFreePart(const std::string &mesh, const std::string suffix);
	virtual void runAnimation(float timepassed);
	void updateParts();
    void removeIndividualPart(int type);
    void removePartGroup(int group);
    bool addOrReplaceIndividualPart(int type, int group, int priority, const std::string &mesh);
	
};
}
#endif
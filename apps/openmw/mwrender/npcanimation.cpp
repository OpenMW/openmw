#include "npcanimation.hpp"
#include "../mwworld/world.hpp"



using namespace Ogre;
using namespace NifOgre;
namespace MWRender{
NpcAnimation::~NpcAnimation(){

}


NpcAnimation::NpcAnimation(const MWWorld::Ptr& ptr, MWWorld::Environment& _env,OEngine::Render::OgreRenderer& _rend, MWWorld::InventoryStore& _inv): Animation(_env,_rend), mStateID(-1), inv(_inv), robe(inv.getSlot(MWWorld::InventoryStore::Slot_Robe)){
     ESMS::LiveCellRef<ESM::NPC, MWWorld::RefData> *ref =
            ptr.get<ESM::NPC>();
	 Ogre::Entity* blank = 0;
	  std::vector<Nif::NiTriShapeCopy>* blankshape = 0;
	 chest = std::make_pair(blank, blankshape);
	 tail = std::make_pair(blank, blankshape);
	 lBeastFoot = std::make_pair(blank, blankshape);
	 rBeastFoot = std::make_pair(blank, blankshape);
	 rhand = std::make_pair(blank, blankshape);
	 lhand = std::make_pair(blank, blankshape);
	 skirt = std::make_pair(blank, blankshape);
		//Part selection on last character of the file string
		//  " Tri Chest
		//  * Tri Tail
		//  : Tri Left Foot
		//  < Tri Right Foot
		//  > Tri Left Hand
		//  ? Tri Right Hand
		//  | Normal

		//Mirroring Parts on second to last character
		//suffix == '*'
		//	vector = Ogre::Vector3(-1,1,1);
		//  suffix == '?'
		//	vector = Ogre::Vector3(1,-1,1);
		//  suffix == '<'
		//	vector = Ogre::Vector3(1,1,-1);


		hairID = ref->base->hair;
        headID = ref->base->head;
		npcName = ref->base->name;
        //ESMStore::Races r =
        const ESM::Race* race = mEnvironment.mWorld->getStore().races.find(ref->base->race);


         bodyRaceID = headID.substr(0, headID.find_last_of("head_") - 4);
		char secondtolast = bodyRaceID.at(bodyRaceID.length() - 2);
		bool female = tolower(secondtolast) == 'f';
		std::transform(bodyRaceID.begin(), bodyRaceID.end(), bodyRaceID.begin(), ::tolower);
		isBeast = bodyRaceID == "b_n_khajiit_m_" || bodyRaceID == "b_n_khajiit_f_" || bodyRaceID == "b_n_argonian_m_" || bodyRaceID == "b_n_argonian_f_";

        /*std::cout << "Race: " << ref->base->race ;
        if(female){
           std::cout << " Sex: Female" << " Height: " << race->data.height.female << "\n";
        }
        else{
             std::cout << " Sex: Male" << " Height: " << race->data.height.male << "\n";
        }*/



        std::string smodel = "meshes\\base_anim.nif";
		if(isBeast)
			smodel = "meshes\\base_animkna.nif";

         insert = ptr.getRefData().getBaseNode();
         assert(insert);

         NifOgre::NIFLoader::load(smodel);

    base = mRend.getScene()->createEntity(smodel);
	
    base->setSkipAnimationStateUpdate(true);   //Magical line of code, this makes the bones
                                               //stay in the same place when we skipanim, or open a gui window



    if((transformations = (NIFLoader::getSingletonPtr())->getAnim(smodel))){

        for(unsigned int init = 0; init < transformations->size(); init++){
				rindexI.push_back(0);
				tindexI.push_back(0);
			}

        stopTime = transformations->begin()->getStopTime();
		startTime = transformations->begin()->getStartTime();
    }
    textmappings = NIFLoader::getSingletonPtr()->getTextIndices(smodel);
    insert->attachObject(base);
	

        if(female)
            insert->scale(race->data.height.female, race->data.height.female, race->data.height.female);
        else
            insert->scale(race->data.height.male, race->data.height.male, race->data.height.male);
        updateParts();
		
}

void NpcAnimation::updateParts(){
	std::string headModel = "meshes\\" +
            mEnvironment.mWorld->getStore().bodyParts.find(headID)->model;

		std::string hairModel = "meshes\\" +
            mEnvironment.mWorld->getStore().bodyParts.find(hairID)->model;

		//inv.getSlot(MWWorld::InventoryStore::Slot_Robe);
		
		robe = inv.getSlot(MWWorld::InventoryStore::Slot_Cuirass);
		if(robe == inv.end())
			std::cout << "No part\n";
		else
			std::cout << "yes part\n";
		
		
        const ESM::BodyPart *chestPart = mEnvironment.mWorld->getStore().bodyParts.search (bodyRaceID + "chest");
        const ESM::BodyPart *upperleg = mEnvironment.mWorld->getStore().bodyParts.search (bodyRaceID + "upper leg");
		const ESM::BodyPart *groinpart = mEnvironment.mWorld->getStore().bodyParts.search (bodyRaceID + "groin");
		const ESM::BodyPart *arml = mEnvironment.mWorld->getStore().bodyParts.search (bodyRaceID + "upper arm");  //We need two
		const ESM::BodyPart *neckpart = mEnvironment.mWorld->getStore().bodyParts.search (bodyRaceID + "neck");
		const ESM::BodyPart *knee = mEnvironment.mWorld->getStore().bodyParts.search (bodyRaceID + "knee");
		const ESM::BodyPart *ankle = mEnvironment.mWorld->getStore().bodyParts.search (bodyRaceID + "ankle");
		const ESM::BodyPart *foot = mEnvironment.mWorld->getStore().bodyParts.search (bodyRaceID + "foot");
		const ESM::BodyPart *feetpart = mEnvironment.mWorld->getStore().bodyParts.search (bodyRaceID + "feet");
		const ESM::BodyPart *tailpart = mEnvironment.mWorld->getStore().bodyParts.search (bodyRaceID + "tail");
		const ESM::BodyPart *wristlpart = mEnvironment.mWorld->getStore().bodyParts.search (bodyRaceID + "wrist");  //We need two
		const ESM::BodyPart *forearmlpart = mEnvironment.mWorld->getStore().bodyParts.search (bodyRaceID + "forearm");  //We need two
		const ESM::BodyPart *handlpart = mEnvironment.mWorld->getStore().bodyParts.search (bodyRaceID + "hand");   //We need two
		const ESM::BodyPart *hairpart = mEnvironment.mWorld->getStore().bodyParts.search(hairID);
		const ESM::BodyPart *headpart = mEnvironment.mWorld->getStore().bodyParts.search(headID);
        if(bodyRaceID == "b_n_argonian_f_")
            forearmlpart = mEnvironment.mWorld->getStore().bodyParts.search ("b_n_argonian_m_forearm");  //We need two
		if(!handlpart)
			handlpart = mEnvironment.mWorld->getStore().bodyParts.search (bodyRaceID + "hands");
		//const ESM::BodyPart* claviclel = environment.mWorld->getStore().bodyParts.search (bodyRaceID + "clavicle");
		//const ESM::BodyPart* clavicler = claviclel;
		const ESM::BodyPart* handrpart = handlpart;
		const ESM::BodyPart* forearmr = forearmlpart;
		const ESM::BodyPart* wristrpart = wristlpart;
		const ESM::BodyPart* armr = arml;


        if(upperleg){
			lUpperLeg = insertBoundedPart("meshes\\" + upperleg->model + "*|", "Left Upper Leg");
			rUpperLeg = insertBoundedPart("meshes\\" + upperleg->model, "Right Upper Leg");

		}
		
        if(foot){
			if(bodyRaceID.compare("b_n_khajiit_m_") == 0)
			{
				feetpart = foot;
			}
			else
			{
				rfoot = insertBoundedPart("meshes\\" + foot->model, "Right Foot");
				lfoot = insertBoundedPart("meshes\\" + foot->model + "*|", "Left Foot");
			}
		}
        if(groinpart){
			groin = insertBoundedPart("meshes\\" + groinpart->model, "Groin");
		}
        if(knee)
		{
			lKnee = insertBoundedPart("meshes\\" + knee->model + "*|", "Left Knee");  //e
			rKnee = insertBoundedPart("meshes\\" + knee->model, "Right Knee");   //e

		}
		if(ankle){

			lAnkle = insertBoundedPart("meshes\\" + ankle->model + "*|", "Left Ankle"); //Ogre::Quaternion(Ogre::Radian(3.14 / 4), Ogre::Vector3(1, 0, 0)),blank); //1,0,0, blank);
			rAnkle = insertBoundedPart("meshes\\" + ankle->model, "Right Ankle");
		}
        if (armr){
			rupperArm = insertBoundedPart("meshes\\" + armr->model, "Right Upper Arm");
		}
		if(arml){
			lupperArm = insertBoundedPart("meshes\\" + arml->model + "*|", "Left Upper Arm");
		}

		if (forearmr)
		{
				rForearm = insertBoundedPart("meshes\\" + forearmr->model, "Right Forearm");
		}
		if(forearmlpart)
			lForearm = insertBoundedPart("meshes\\" + forearmlpart->model + "*|", "Left Forearm");

		if (wristrpart)
		{
			rWrist = insertBoundedPart("meshes\\" + wristrpart->model, "Right Wrist");
		}

		if(wristlpart)
				lWrist = insertBoundedPart("meshes\\" + wristlpart->model + "*|", "Left Wrist");





		/*if(claviclel)
			insertBoundedPart("meshes\\" + claviclel->model + "*|", "Left Clavicle", base);
		if(clavicler)
			insertBoundedPart("meshes\\" + clavicler->model , "Right Clavicle", base);*/

		if(neckpart)
		{
			neck = insertBoundedPart("meshes\\" + neckpart->model, "Neck");
		}
		if(headpart)
			head = insertBoundedPart("meshes\\" + headpart->model, "Head");
		if(hairpart)
			hair = insertBoundedPart("meshes\\" + hairpart->model, "Head");

        if (chestPart){
				chest = insertFreePart("meshes\\" + chestPart->model, ":\"");


		}
        if (handrpart){
				rhand = insertFreePart("meshes\\" + handrpart->model , ":?");

		}
        if (handlpart){
				lhand = insertFreePart("meshes\\" + handlpart->model, ":>");

		}
        if(tailpart){
                tail = insertFreePart("meshes\\" + tailpart->model, ":*");
        }
        if(feetpart){
                
                lBeastFoot = insertFreePart("meshes\\" + feetpart->model,"::");
                rBeastFoot = insertFreePart("meshes\\" + feetpart->model,":<");
        }
        //originalpos = insert->_getWorl
}

Ogre::Entity* NpcAnimation::insertBoundedPart(const std::string &mesh, std::string bonename){
   
    NIFLoader::load(mesh);
    Ogre::Entity* part = mRend.getScene()->createEntity(mesh);

    base->attachObjectToBone(bonename, part);
    return part;
}
std::pair<Ogre::Entity*, std::vector<Nif::NiTriShapeCopy>*> NpcAnimation::insertFreePart(const std::string &mesh, const std::string suffix){
	
    std::string meshNumbered = mesh + getUniqueID(mesh + suffix) + suffix;
    NIFLoader::load(meshNumbered);

    Ogre::Entity* part = mRend.getScene()->createEntity(meshNumbered);

     


    insert->attachObject(part);
   
    std::vector<Nif::NiTriShapeCopy>* shape = ((NIFLoader::getSingletonPtr())->getShapes(mesh + "0000" + suffix));
    if(shape){
        
        handleShapes(shape, part, base->getSkeleton());
    }
	 std::pair<Ogre::Entity*, std::vector<Nif::NiTriShapeCopy>*> pair = std::make_pair(part, shape);
	 return pair;
}


void NpcAnimation::runAnimation(float timepassed){
	if(mStateID != inv.getStateId()){
		mStateID = inv.getStateId();
		updateParts();
	}
	
   
    //1. Add the amount of time passed to time

	//2. Handle the animation transforms dependent on time

	//3. Handle the shapes dependent on animation transforms
	if(animate > 0){
        time += timepassed;

        if(time > stopTime){
            animate--;

            if(animate == 0)
                time = stopTime;
            else
                time = startTime + (time - stopTime);
        }

       handleAnimationTransforms();

       
            vecRotPos.clear();
          
			
			if(lBeastFoot.first)
				handleShapes(lBeastFoot.second, lBeastFoot.first, base->getSkeleton());
			if(rBeastFoot.first)
				handleShapes(rBeastFoot.second, rBeastFoot.first, base->getSkeleton());
			if(chest.first)
				handleShapes(chest.second, chest.first, base->getSkeleton());
			if(tail.first)
				handleShapes(tail.second, tail.first, base->getSkeleton());
			if(skirt.first)
				handleShapes(skirt.second, skirt.first, base->getSkeleton());
			if(lhand.first)
				handleShapes(lhand.second, lhand.first, base->getSkeleton());
			if(rhand.first)
				handleShapes(rhand.second, rhand.first, base->getSkeleton());
			
}
}
}
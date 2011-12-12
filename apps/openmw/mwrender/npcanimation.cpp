#include "npcanimation.hpp"
#include "../mwworld/world.hpp"

using namespace Ogre;
using namespace NifOgre;
namespace MWRender{

NpcAnimation::NpcAnimation(const MWWorld::Ptr& ptr, MWWorld::Environment& _env,OEngine::Render::OgreRenderer& _rend): Animation(_env,_rend){
     ESMS::LiveCellRef<ESM::NPC, MWWorld::RefData> *ref =
            ptr.get<ESM::NPC>();
        //assert (ref->base != NULL);
		
        //insertBegin(ptr, true, true);
		
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



		std::string hairID = ref->base->hair;
        std::string headID = ref->base->head;
		std::string npcName = ref->base->name;

         std::string bodyRaceID = headID.substr(0, headID.find_last_of("head_") - 4);
		char secondtolast = bodyRaceID.at(bodyRaceID.length() - 2);
		bool female = tolower(secondtolast) == 'f';
		bool beast = bodyRaceID == "b_n_khajiit_m_" || bodyRaceID == "b_n_khajiit_f_" || bodyRaceID == "b_n_argonian_m_" || bodyRaceID == "b_n_argonian_f_";


        std::string smodel = "meshes\\base_anim.nif";
		if(beast)
			smodel = "meshes\\base_animkna.nif";

         Ogre::SceneNode* insert = ptr.getRefData().getBaseNode();
         assert(insert);

         NifOgre::NIFLoader::load(smodel);
         
    base = mRend.getScene()->createEntity(smodel);
    insert->attachObject(base);
        std::cout << "Nifloader\n";
        
        std::string headModel = "meshes\\" +
            mEnvironment.mWorld->getStore().bodyParts.find(headID)->model;

		std::string hairModel = "meshes\\" +
            mEnvironment.mWorld->getStore().bodyParts.find(hairID)->model;

        const ESM::BodyPart *upperleg = mEnvironment.mWorld->getStore().bodyParts.search (bodyRaceID + "upper leg");
		const ESM::BodyPart *groin = mEnvironment.mWorld->getStore().bodyParts.search (bodyRaceID + "groin");
		const ESM::BodyPart *arml = mEnvironment.mWorld->getStore().bodyParts.search (bodyRaceID + "upper arm");  //We need two
		const ESM::BodyPart *neck = mEnvironment.mWorld->getStore().bodyParts.search (bodyRaceID + "neck");
		const ESM::BodyPart *knee = mEnvironment.mWorld->getStore().bodyParts.search (bodyRaceID + "knee");
		const ESM::BodyPart *ankle = mEnvironment.mWorld->getStore().bodyParts.search (bodyRaceID + "ankle");
		const ESM::BodyPart *foot = mEnvironment.mWorld->getStore().bodyParts.search (bodyRaceID + "foot");
		const ESM::BodyPart *feet = mEnvironment.mWorld->getStore().bodyParts.search (bodyRaceID + "feet");
		const ESM::BodyPart *tail = mEnvironment.mWorld->getStore().bodyParts.search (bodyRaceID + "tail");
		const ESM::BodyPart *wristl = mEnvironment.mWorld->getStore().bodyParts.search (bodyRaceID + "wrist");  //We need two
		const ESM::BodyPart *forearml = mEnvironment.mWorld->getStore().bodyParts.search (bodyRaceID + "forearm");  //We need two
		const ESM::BodyPart *handl = mEnvironment.mWorld->getStore().bodyParts.search (bodyRaceID + "hand");   //We need two
		const ESM::BodyPart *hair = mEnvironment.mWorld->getStore().bodyParts.search(hairID);
		const ESM::BodyPart *head = mEnvironment.mWorld->getStore().bodyParts.search(headID);
		if(!handl)
			handl = mEnvironment.mWorld->getStore().bodyParts.search (bodyRaceID + "hands");
		//const ESM::BodyPart* claviclel = environment.mWorld->getStore().bodyParts.search (bodyRaceID + "clavicle");
		//const ESM::BodyPart* clavicler = claviclel;
		const ESM::BodyPart* handr = handl;
		const ESM::BodyPart* forearmr = forearml;
		const ESM::BodyPart* wristr = wristl;
		const ESM::BodyPart* armr = arml;
        std::cout << "upperleg";
        if(upperleg){
			insertBoundedPart("meshes\\" + upperleg->model + "*|", "Left Upper Leg");
			insertBoundedPart("meshes\\" + upperleg->model, "Right Upper Leg");
			
		}
        if(foot){
			if(bodyRaceID.compare("b_n_khajiit_m_") == 0)
			{
				feet = foot;
			}
			else
			{
				insertBoundedPart("meshes\\" + foot->model, "Right Foot");
				insertBoundedPart("meshes\\" + foot->model + "*|", "Left Foot");
			}
		}
        if(groin){
			insertBoundedPart("meshes\\" + groin->model, "Groin");
		}
        if(knee)
		{
			insertBoundedPart("meshes\\" + knee->model + "*|", "Left Knee");  //e
			insertBoundedPart("meshes\\" + knee->model, "Right Knee");   //e
			
		}
		if(ankle){
			
			insertBoundedPart("meshes\\" + ankle->model + "*|", "Left Ankle"); //Ogre::Quaternion(Ogre::Radian(3.14 / 4), Ogre::Vector3(1, 0, 0)),blank); //1,0,0, blank);
			insertBoundedPart("meshes\\" + ankle->model, "Right Ankle");
		}
        if (armr){
			insertBoundedPart("meshes\\" + armr->model, "Right Upper Arm");
		}
		if(arml){
			insertBoundedPart("meshes\\" + arml->model + "*|", "Left Upper Arm");
		}

		if (forearmr)
		{
				insertBoundedPart("meshes\\" + forearmr->model, "Right Forearm");
		}
		if(forearml)
			insertBoundedPart("meshes\\" + forearml->model + "*|", "Left Forearm");

		if (wristr)
		{
			insertBoundedPart("meshes\\" + wristr->model, "Right Wrist");
		}

		if(wristl)
				insertBoundedPart("meshes\\" + wristl->model + "*|", "Left Wrist");
		

	
		

		/*if(claviclel)
			insertBoundedPart("meshes\\" + claviclel->model + "*|", "Left Clavicle", base);
		if(clavicler)
			insertBoundedPart("meshes\\" + clavicler->model , "Right Clavicle", base);*/
	
	
		if(neck)
		{
			insertBoundedPart("meshes\\" + neck->model, "Neck");
		}
		if(head)
			insertBoundedPart("meshes\\" + head->model, "Head");
		if(hair)
			insertBoundedPart("meshes\\" + hair->model, "Head");
}

Ogre::Entity* NpcAnimation::insertBoundedPart(const std::string &mesh, std::string bonename){
    NIFLoader::load(mesh);
    Entity* ent = mRend.getScene()->createEntity(mesh);
	 
    base->attachObjectToBone(bonename, ent); 
    return ent;
}
}
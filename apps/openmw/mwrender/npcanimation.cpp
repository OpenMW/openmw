#include "npcanimation.hpp"
#include "../mwworld/world.hpp"
#include "renderconst.hpp"



using namespace Ogre;
using namespace NifOgre;
namespace MWRender{
NpcAnimation::~NpcAnimation(){

}


NpcAnimation::NpcAnimation(const MWWorld::Ptr& ptr, MWWorld::Environment& _env,OEngine::Render::OgreRenderer& _rend, MWWorld::InventoryStore& _inv): Animation(_env,_rend), mStateID(-1), inv(_inv), timeToChange(0), 
    robe(inv.getSlot(MWWorld::InventoryStore::Slot_Robe)), helmet(inv.getSlot(MWWorld::InventoryStore::Slot_Helmet)), shirt(inv.getSlot(MWWorld::InventoryStore::Slot_Shirt)),
    cuirass(inv.getSlot(MWWorld::InventoryStore::Slot_Cuirass)), greaves(inv.getSlot(MWWorld::InventoryStore::Slot_Greaves)),
    lclavicle(0),
	rclavicle(0),
	rupperArm(0),
	lupperArm(0),
	rUpperLeg(0),
	lUpperLeg(0),
	lForearm(0),
	rForearm(0),
	lWrist(0),
	rWrist(0),
	rKnee(0),
	lKnee(0),
	neck(0),
	rAnkle(0),
	lAnkle(0),
	groin(0),
	lfoot(0),
	rfoot(0)
    {
     ESMS::LiveCellRef<ESM::NPC, MWWorld::RefData> *ref =
            ptr.get<ESM::NPC>();
	 Ogre::Entity* blank = 0;
	  std::vector<Nif::NiTriShapeCopy>* blankshape = 0;
      zero = std::make_pair(blank, blankshape);
	 chest = std::make_pair(blank, blankshape);
	 tail = std::make_pair(blank, blankshape);
	 lBeastFoot = std::make_pair(blank, blankshape);
	 rBeastFoot = std::make_pair(blank, blankshape);
	 rhand = std::make_pair(blank, blankshape);
	 lhand = std::make_pair(blank, blankshape);
	 skirt = std::make_pair(blank, blankshape);
     for (int init = 0; init < 27; init++){
         partslots[init] = -1;  //each slot is empty
         partpriorities[init] = 0;
     }


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
		isFemale = tolower(secondtolast) == 'f';
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
	

        if(isFemale)
            insert->scale(race->data.height.female, race->data.height.female, race->data.height.female);
        else
            insert->scale(race->data.height.male, race->data.height.male, race->data.height.male);
		std::cout << "Inv" << inv.getStateId() << "\n";
        updateParts();
		
}

void NpcAnimation::updateParts(){
	std::string headModel = "meshes\\" +
            mEnvironment.mWorld->getStore().bodyParts.find(headID)->model;

		std::string hairModel = "meshes\\" +
            mEnvironment.mWorld->getStore().bodyParts.find(hairID)->model;
        bool apparelChanged = false;
        

		//inv.getSlot(MWWorld::InventoryStore::Slot_Robe);
		if(robe != inv.getSlot(MWWorld::InventoryStore::Slot_Robe)){
            //A robe was added or removed
            removePartGroup(MWWorld::InventoryStore::Slot_Robe);
            robe = inv.getSlot(MWWorld::InventoryStore::Slot_Robe);
            apparelChanged = true;
           
		
		}
        if(helmet != inv.getSlot(MWWorld::InventoryStore::Slot_Helmet)){
            apparelChanged = true;
            helmet = inv.getSlot(MWWorld::InventoryStore::Slot_Helmet);
            removePartGroup(MWWorld::InventoryStore::Slot_Helmet);

        }
        if(cuirass != inv.getSlot(MWWorld::InventoryStore::Slot_Cuirass)){
            cuirass = inv.getSlot(MWWorld::InventoryStore::Slot_Cuirass);
            removePartGroup(MWWorld::InventoryStore::Slot_Cuirass);
            apparelChanged = true;
            
        }
        if(greaves != inv.getSlot(MWWorld::InventoryStore::Slot_Greaves)){
            cuirass = inv.getSlot(MWWorld::InventoryStore::Slot_Greaves);
            removePartGroup(MWWorld::InventoryStore::Slot_Greaves);
            apparelChanged = true;
            
        }
        if(shirt != inv.getSlot(MWWorld::InventoryStore::Slot_Shirt)){
            shirt = inv.getSlot(MWWorld::InventoryStore::Slot_Shirt);
            removePartGroup(MWWorld::InventoryStore::Slot_Shirt);
            apparelChanged = true;
            
        }

        if(apparelChanged){
             if(robe != inv.end())
            {
                MWWorld::Ptr ptr = *robe;
                
                const ESM::Clothing *clothes = (ptr.get<ESM::Clothing>())->base;
                std::vector<ESM::PartReference> parts = clothes->parts.parts;
                addPartGroup(MWWorld::InventoryStore::Slot_Robe, 5, parts);
            }
            
             if(helmet != inv.end()){
                removeIndividualPart(ESM::PRT_Hair);
                const ESM::Armor *armor = (helmet->get<ESM::Armor>())->base;
                std::vector<ESM::PartReference> parts = armor->parts.parts;
                addPartGroup(MWWorld::InventoryStore::Slot_Helmet, 3, parts);
                
            }
             if(cuirass != inv.end()){
                const ESM::Armor *armor = (cuirass->get<ESM::Armor>())->base;
                std::vector<ESM::PartReference> parts = armor->parts.parts;
                addPartGroup(MWWorld::InventoryStore::Slot_Cuirass, 3, parts);
                
            }
             if(greaves != inv.end()){
                const ESM::Armor *armor = (greaves->get<ESM::Armor>())->base;
                std::vector<ESM::PartReference> parts = armor->parts.parts;
                addPartGroup(MWWorld::InventoryStore::Slot_Greaves, 3, parts);
                
            }
             if(shirt != inv.end()){
                const ESM::Clothing *clothes = (shirt->get<ESM::Clothing>())->base;
                std::vector<ESM::PartReference> parts = clothes->parts.parts;
                addPartGroup(MWWorld::InventoryStore::Slot_Shirt, 2, parts);
            }
        }

                if(partpriorities[ESM::PRT_Cuirass] < 1){
                    const ESM::BodyPart *chestPart = mEnvironment.mWorld->getStore().bodyParts.search (bodyRaceID + "chest");
                    if(chestPart)
                        addOrReplaceIndividualPart(ESM::PRT_Cuirass, -1,1,"meshes\\" + chestPart->model);
                }
                if(partpriorities[ESM::PRT_Head] < 1){
                        addOrReplaceIndividualPart(ESM::PRT_Head, -1,1,headModel);
                }
                if(partpriorities[ESM::PRT_Hair] < 1 && partpriorities[ESM::PRT_Head] <= 1){
                        addOrReplaceIndividualPart(ESM::PRT_Hair, -1,1,hairModel);
                }
           
		
		
		
		
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
	
	if(timeToChange > .2){
		
		timeToChange = 0;
		
		updateParts();
	}
	
	timeToChange += timepassed;
   
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

void NpcAnimation::removeIndividualPart(int type){
    partpriorities[type] = 0;
    partslots[type] = -1;
    
        if(type == ESM::PRT_Head && head){   //0
            base->detachObjectFromBone(head);
            head = 0;
        }
        else if(type == ESM::PRT_Hair && hair){//1
            base->detachObjectFromBone(hair);
            hair = 0;
        }
        else if(type == ESM::PRT_Neck && neck){//2
            base->detachObjectFromBone(neck);
            neck = 0;
        }
        else if(type == ESM::PRT_Cuirass && chest.first){//3
            insert->detachObject(chest.first);
            chest = zero;
        }
        else if(type == ESM::PRT_Groin && groin){//4
            base->detachObjectFromBone(groin);
            groin = 0;
        }
        else if(type == ESM::PRT_Skirt && skirt.first){//5
            insert->detachObject(skirt.first);
            skirt = zero;
        }
        else if(type == ESM::PRT_RHand && rhand.first){//6
            insert->detachObject(rhand.first);
            rhand = zero;
        }
        else if(type == ESM::PRT_LHand && lhand.first){//7
            insert->detachObject(lhand.first);
            lhand = zero;
        }
        else if(type == ESM::PRT_RWrist && rWrist){//8
            base->detachObjectFromBone(rWrist);
            rWrist = 0;
        }
        else if(type == ESM::PRT_LWrist && lWrist){//9
            base->detachObjectFromBone(lWrist);
            lWrist = 0;
        }
        else if(type == ESM::PRT_Shield){//10
            
        }
        else if(type == ESM::PRT_RForearm && rForearm){//11
            base->detachObjectFromBone(rForearm);
            rForearm = 0;
        }
        else if(type == ESM::PRT_LForearm && lForearm){//12
            base->detachObjectFromBone(lForearm);
            lForearm = 0;
        }
        else if(type == ESM::PRT_RUpperarm && rupperArm){//13
            base->detachObjectFromBone(rupperArm);
            rupperArm = 0;
        }
        else if(type == ESM::PRT_LUpperarm && lupperArm){//14
            base->detachObjectFromBone(lupperArm);
            lupperArm = 0;
        }
        else if(type == ESM::PRT_RFoot){                 //15
            if(rfoot){
                base->detachObjectFromBone(rfoot);
                rfoot = 0;
            }
            else if(rBeastFoot.first){
                insert->detachObject(rBeastFoot.first);
                rBeastFoot = zero;
            }
        }
        else if(type == ESM::PRT_LFoot){                //16
            if(lfoot){
                base->detachObjectFromBone(lfoot);
                lfoot = 0;
            }
            else if(lBeastFoot.first){
                insert->detachObject(lBeastFoot.first);
                lBeastFoot = zero;
            }
        }
        else if(type == ESM::PRT_RAnkle && rAnkle){    //17
            base->detachObjectFromBone(rAnkle);
            rAnkle = 0;
        }
        else if(type == ESM::PRT_LAnkle && lAnkle){    //18
            base->detachObjectFromBone(lAnkle);
            lAnkle = 0;
        }
         else if(type == ESM::PRT_RKnee && rKnee){    //19
            base->detachObjectFromBone(rKnee);
            rKnee = 0;
        }
        else if(type == ESM::PRT_LKnee && lKnee){    //20
            base->detachObjectFromBone(lKnee);
            lKnee = 0;
        }
         else if(type == ESM::PRT_RLeg && rUpperLeg){    //21
            base->detachObjectFromBone(rUpperLeg);
            rUpperLeg = 0;
        }
        else if(type == ESM::PRT_LLeg && lUpperLeg){    //22
            base->detachObjectFromBone(lUpperLeg);
            lUpperLeg = 0;
        }
          else if(type == ESM::PRT_RPauldron && rclavicle){    //23
            base->detachObjectFromBone(rclavicle);
            rclavicle = 0;
        }
        else if(type == ESM::PRT_LPauldron && lclavicle){    //24
            base->detachObjectFromBone(lclavicle);
            lclavicle = 0;
        }
        else if(type == ESM::PRT_Weapon){                  //25

        }
        else if(type == ESM::PRT_Tail && tail.first){     //26
            insert->detachObject(tail.first);
            tail = zero;
        }
        
        
        

    }

    void NpcAnimation::removePartGroup(int group){
        for(int i = 0; i < 27; i++){
            if(partslots[i] == group){
                 removeIndividualPart(i);
            }
        }
    }
    bool NpcAnimation::addOrReplaceIndividualPart(int type, int group, int priority, const std::string &mesh){
        if(priority > partpriorities[type]){
            removeIndividualPart(type);
            partslots[type] = group;
            partpriorities[type] = priority;
            switch(type){
                case ESM::PRT_Head:                           //0
                    head = insertBoundedPart(mesh, "Head");
                    break;
                case ESM::PRT_Hair:                          //1
                    hair = insertBoundedPart(mesh, "Head");
                    break;
                case ESM::PRT_Neck:                          //2
                    neck = insertBoundedPart(mesh, "Neck");
                    break;
                case ESM::PRT_Cuirass:                          //3
                    chest = insertFreePart(mesh, ":\"");
                    break;
                case ESM::PRT_Groin:                          //4
                    neck = insertBoundedPart(mesh, "Groin");
                    break;
                case ESM::PRT_Skirt:                          //5
                    skirt = insertFreePart(mesh, ":|");
                    break;
                case ESM::PRT_RHand:                         //6
                    rhand = insertFreePart(mesh, ":?");
                    break;
                case ESM::PRT_LHand:                         //7
                    lhand = insertFreePart(mesh,  ":>");
                    break;
                case ESM::PRT_RWrist:                          //8
                    rWrist = insertBoundedPart(mesh, "Right Wrist");
                    break;
                case ESM::PRT_LWrist:                          //9
                    lWrist = insertBoundedPart(mesh + "*|", "Left Wrist");
                    break;
                case ESM::PRT_Shield:                         //10
                    break;
                case ESM::PRT_RForearm:                          //11
                    rForearm = insertBoundedPart(mesh, "Right Forearm");
                    break;
                case ESM::PRT_LForearm:                          //12
                    lForearm = insertBoundedPart(mesh + "*|", "Left Forearm");
                    break;
                case ESM::PRT_RUpperarm:                          //13
                    rupperArm = insertBoundedPart(mesh, "Right Upper Arm");
                    break;
                case ESM::PRT_LUpperarm:                          //14
                    lupperArm = insertBoundedPart(mesh + "*|", "Left Upper Arm");
                    break;
                case ESM::PRT_RFoot:                             //15
                    if(isBeast)   
                        rBeastFoot = insertFreePart(mesh, ":<");
                    else
                        rfoot = insertBoundedPart(mesh, "Right Foot");
                    break;
                case ESM::PRT_LFoot:                             //16
                    if(isBeast)   
                        lBeastFoot = insertFreePart(mesh, "::");
                    else
                        lfoot = insertBoundedPart(mesh + "*|", "Left Foot");
                    break;

                 case ESM::PRT_RAnkle:                          //17
                    rAnkle = insertBoundedPart(mesh , "Right Ankle");
                    break;
                case ESM::PRT_LAnkle:                          //18
                    lAnkle = insertBoundedPart(mesh + "*|", "Left Ankle");
                    break;
                 case ESM::PRT_RKnee:                          //19
                    rKnee = insertBoundedPart(mesh , "Right Knee");
                    break;
                case ESM::PRT_LKnee:                          //20
                    lKnee = insertBoundedPart(mesh + "*|", "Left Knee");
                    break;
                 case ESM::PRT_RLeg:                          //21
                    rUpperLeg = insertBoundedPart(mesh, "Right Upper Leg");
                    break;
                case ESM::PRT_LLeg:                          //22
                    lUpperLeg = insertBoundedPart(mesh + "*|", "Left Upper Leg");
                    break;
                case ESM::PRT_RPauldron:                          //23
                    rclavicle = insertBoundedPart(mesh , "Right Clavicle");
                    break;
                case ESM::PRT_LPauldron:                          //24
                    lclavicle = insertBoundedPart(mesh + "*|", "Left Clavicle");
                    break;
                case ESM::PRT_Weapon:                             //25
                    break;
                case ESM::PRT_Tail:                              //26
                    tail = insertFreePart(mesh, ":*");
                    break;


            }
            return true;
        }
        return false;
    }

    void NpcAnimation::addPartGroup(int group, int priority, std::vector<ESM::PartReference>& parts){
        for(int i = 0; i < parts.size(); i++)
                {
                    ESM::PartReference part = parts[i];
                    

                        const ESM::BodyPart *bodypart = 0;
                        
                        if(isFemale)
                            bodypart = mEnvironment.mWorld->getStore().bodyParts.search (part.female);
                        if(!bodypart)
                            bodypart = mEnvironment.mWorld->getStore().bodyParts.search (part.male);

                        if(bodypart)
                            addOrReplaceIndividualPart(part.part, group,priority,"meshes\\" + bodypart->model);
                    
                }
    }
}



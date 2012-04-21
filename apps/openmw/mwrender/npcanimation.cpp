#include "npcanimation.hpp"
#include "../mwworld/world.hpp"
#include "renderconst.hpp"



using namespace Ogre;
using namespace NifOgre;
namespace MWRender{
NpcAnimation::~NpcAnimation(){

}


NpcAnimation::NpcAnimation(const MWWorld::Ptr& ptr, MWWorld::Environment& _env,OEngine::Render::OgreRenderer& _rend, MWWorld::InventoryStore& _inv): Animation(_env,_rend), mStateID(-1), inv(_inv), timeToChange(0),
    robe(inv.end()), helmet(inv.end()), shirt(inv.end()),
    cuirass(inv.end()), greaves(inv.end()),
    leftpauldron(inv.end()), rightpauldron(inv.end()),
    boots(inv.end()),
    leftglove(inv.end()), rightglove(inv.end()), skirtiter(inv.end()),
    pants(inv.end()),
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
	 lFreeFoot = std::make_pair(blank, blankshape);
	 rFreeFoot = std::make_pair(blank, blankshape);
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


		std::string hairID = ref->base->hair;
        std::string headID = ref->base->head;
        headModel = "meshes\\" +
            mEnvironment.mWorld->getStore().bodyParts.find(headID)->model;

		hairModel = "meshes\\" +
            mEnvironment.mWorld->getStore().bodyParts.find(hairID)->model;
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
        updateParts();

}

void NpcAnimation::updateParts(){

        bool apparelChanged = false;


		//inv.getSlot(MWWorld::InventoryStore::Slot_Robe);
		if(robe != inv.getSlot(MWWorld::InventoryStore::Slot_Robe)){
            //A robe was added or removed
            removePartGroup(MWWorld::InventoryStore::Slot_Robe);
            robe = inv.getSlot(MWWorld::InventoryStore::Slot_Robe);
            apparelChanged = true;
		}
        if(skirtiter != inv.getSlot(MWWorld::InventoryStore::Slot_Skirt)){
            //A robe was added or removed
            removePartGroup(MWWorld::InventoryStore::Slot_Skirt);
            skirtiter = inv.getSlot(MWWorld::InventoryStore::Slot_Skirt);
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
            greaves = inv.getSlot(MWWorld::InventoryStore::Slot_Greaves);
            removePartGroup(MWWorld::InventoryStore::Slot_Greaves);
            apparelChanged = true;
        }
        if(leftpauldron != inv.getSlot(MWWorld::InventoryStore::Slot_LeftPauldron)){
            leftpauldron = inv.getSlot(MWWorld::InventoryStore::Slot_LeftPauldron);
            removePartGroup(MWWorld::InventoryStore::Slot_LeftPauldron);
            apparelChanged = true;

        }
        if(rightpauldron != inv.getSlot(MWWorld::InventoryStore::Slot_RightPauldron)){
            rightpauldron = inv.getSlot(MWWorld::InventoryStore::Slot_RightPauldron);
            removePartGroup(MWWorld::InventoryStore::Slot_RightPauldron);
            apparelChanged = true;

        }
        if(!isBeast && boots != inv.getSlot(MWWorld::InventoryStore::Slot_Boots)){
            boots = inv.getSlot(MWWorld::InventoryStore::Slot_Boots);
            removePartGroup(MWWorld::InventoryStore::Slot_Boots);
            apparelChanged = true;

        }
        if(leftglove != inv.getSlot(MWWorld::InventoryStore::Slot_LeftGauntlet)){
            leftglove = inv.getSlot(MWWorld::InventoryStore::Slot_LeftGauntlet);
            removePartGroup(MWWorld::InventoryStore::Slot_LeftGauntlet);
            apparelChanged = true;

        }
         if(rightglove != inv.getSlot(MWWorld::InventoryStore::Slot_RightGauntlet)){
            rightglove = inv.getSlot(MWWorld::InventoryStore::Slot_RightGauntlet);
            removePartGroup(MWWorld::InventoryStore::Slot_RightGauntlet);
            apparelChanged = true;

        }
        if(shirt != inv.getSlot(MWWorld::InventoryStore::Slot_Shirt)){
            shirt = inv.getSlot(MWWorld::InventoryStore::Slot_Shirt);
            removePartGroup(MWWorld::InventoryStore::Slot_Shirt);
            apparelChanged = true;

        }
        if(pants != inv.getSlot(MWWorld::InventoryStore::Slot_Pants)){
            pants = inv.getSlot(MWWorld::InventoryStore::Slot_Pants);
            removePartGroup(MWWorld::InventoryStore::Slot_Pants);
            apparelChanged = true;

        }

        if(apparelChanged){

             if(robe != inv.end())
            {
                MWWorld::Ptr ptr = *robe;

                const ESM::Clothing *clothes = (ptr.get<ESM::Clothing>())->base;
                std::vector<ESM::PartReference> parts = clothes->parts.parts;
                addPartGroup(MWWorld::InventoryStore::Slot_Robe, 5, parts);
                reserveIndividualPart(ESM::PRT_Groin, MWWorld::InventoryStore::Slot_Robe, 5);
                reserveIndividualPart(ESM::PRT_Skirt, MWWorld::InventoryStore::Slot_Robe, 5);
                reserveIndividualPart(ESM::PRT_RLeg, MWWorld::InventoryStore::Slot_Robe, 5);
                reserveIndividualPart(ESM::PRT_LLeg, MWWorld::InventoryStore::Slot_Robe, 5);
                reserveIndividualPart(ESM::PRT_RUpperarm, MWWorld::InventoryStore::Slot_Robe, 5);
                reserveIndividualPart(ESM::PRT_LUpperarm, MWWorld::InventoryStore::Slot_Robe, 5);
                reserveIndividualPart(ESM::PRT_RKnee, MWWorld::InventoryStore::Slot_Robe, 5);
                reserveIndividualPart(ESM::PRT_LKnee, MWWorld::InventoryStore::Slot_Robe, 5);
                reserveIndividualPart(ESM::PRT_RForearm, MWWorld::InventoryStore::Slot_Robe, 5);
                reserveIndividualPart(ESM::PRT_LForearm, MWWorld::InventoryStore::Slot_Robe, 5);
                reserveIndividualPart(ESM::PRT_RPauldron, MWWorld::InventoryStore::Slot_Robe, 5);
                reserveIndividualPart(ESM::PRT_LPauldron, MWWorld::InventoryStore::Slot_Robe, 5);
            }
             if(skirtiter != inv.end())
            {
                MWWorld::Ptr ptr = *skirtiter;

                const ESM::Clothing *clothes = (ptr.get<ESM::Clothing>())->base;
                std::vector<ESM::PartReference> parts = clothes->parts.parts;
                addPartGroup(MWWorld::InventoryStore::Slot_Skirt, 4, parts);
                reserveIndividualPart(ESM::PRT_Groin, MWWorld::InventoryStore::Slot_Skirt, 4);
                reserveIndividualPart(ESM::PRT_RLeg, MWWorld::InventoryStore::Slot_Skirt, 4);
                reserveIndividualPart(ESM::PRT_LLeg, MWWorld::InventoryStore::Slot_Skirt, 4);
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

             if(leftpauldron != inv.end()){
                const ESM::Armor *armor = (leftpauldron->get<ESM::Armor>())->base;
                std::vector<ESM::PartReference> parts = armor->parts.parts;
                addPartGroup(MWWorld::InventoryStore::Slot_LeftPauldron, 3, parts);

            }
             if(rightpauldron != inv.end()){
                const ESM::Armor *armor = (rightpauldron->get<ESM::Armor>())->base;
                std::vector<ESM::PartReference> parts = armor->parts.parts;
                addPartGroup(MWWorld::InventoryStore::Slot_RightPauldron, 3, parts);

            }
             if(!isBeast && boots != inv.end()){
               if(boots->getTypeName() == typeid(ESM::Clothing).name()){
                    const ESM::Clothing *clothes = (boots->get<ESM::Clothing>())->base;
                    std::vector<ESM::PartReference> parts = clothes->parts.parts;
                    addPartGroup(MWWorld::InventoryStore::Slot_Boots, 2, parts);
               }
               else if(boots->getTypeName() == typeid(ESM::Armor).name())
               {
                   const ESM::Armor *armor = (boots->get<ESM::Armor>())->base;
                    std::vector<ESM::PartReference> parts = armor->parts.parts;
                    addPartGroup(MWWorld::InventoryStore::Slot_Boots, 3, parts);
               }

            }
             if(leftglove != inv.end()){
               if(leftglove->getTypeName() == typeid(ESM::Clothing).name()){
                    const ESM::Clothing *clothes = (leftglove->get<ESM::Clothing>())->base;
                    std::vector<ESM::PartReference> parts = clothes->parts.parts;
                    addPartGroup(MWWorld::InventoryStore::Slot_LeftGauntlet, 2, parts);
               }
               else
               {
                   const ESM::Armor *armor = (leftglove->get<ESM::Armor>())->base;
                    std::vector<ESM::PartReference> parts = armor->parts.parts;
                    addPartGroup(MWWorld::InventoryStore::Slot_LeftGauntlet, 3, parts);
               }

            }
             if(rightglove != inv.end()){
               if(rightglove->getTypeName() == typeid(ESM::Clothing).name()){
                    const ESM::Clothing *clothes = (rightglove->get<ESM::Clothing>())->base;
                    std::vector<ESM::PartReference> parts = clothes->parts.parts;
                    addPartGroup(MWWorld::InventoryStore::Slot_RightGauntlet, 2, parts);
               }
               else
               {
                   const ESM::Armor *armor = (rightglove->get<ESM::Armor>())->base;
                    std::vector<ESM::PartReference> parts = armor->parts.parts;
                    addPartGroup(MWWorld::InventoryStore::Slot_RightGauntlet, 3, parts);
               }

            }

             if(shirt != inv.end()){
                const ESM::Clothing *clothes = (shirt->get<ESM::Clothing>())->base;
                std::vector<ESM::PartReference> parts = clothes->parts.parts;
                addPartGroup(MWWorld::InventoryStore::Slot_Shirt, 2, parts);
            }
              if(pants != inv.end()){
                const ESM::Clothing *clothes = (pants->get<ESM::Clothing>())->base;
                std::vector<ESM::PartReference> parts = clothes->parts.parts;
                addPartGroup(MWWorld::InventoryStore::Slot_Pants, 2, parts);
            }
        }

                if(partpriorities[ESM::PRT_Head] < 1){
                        addOrReplaceIndividualPart(ESM::PRT_Head, -1,1,headModel);
                }
                if(partpriorities[ESM::PRT_Hair] < 1 && partpriorities[ESM::PRT_Head] <= 1){
                        addOrReplaceIndividualPart(ESM::PRT_Hair, -1,1,hairModel);
                }
                 if(partpriorities[ESM::PRT_Neck] < 1){
                    const ESM::BodyPart *neckPart = mEnvironment.mWorld->getStore().bodyParts.search (bodyRaceID + "neck");
                    if(neckPart)
                        addOrReplaceIndividualPart(ESM::PRT_Neck, -1,1,"meshes\\" + neckPart->model);
                }
                if(partpriorities[ESM::PRT_Cuirass] < 1){
                    const ESM::BodyPart *chestPart = mEnvironment.mWorld->getStore().bodyParts.search (bodyRaceID + "chest");
                    if(chestPart)
                        addOrReplaceIndividualPart(ESM::PRT_Cuirass, -1,1,"meshes\\" + chestPart->model);
                }

                 if(partpriorities[ESM::PRT_Groin] < 1){
                    const ESM::BodyPart *groinPart = mEnvironment.mWorld->getStore().bodyParts.search (bodyRaceID + "groin");
                    if(groinPart)
                        addOrReplaceIndividualPart(ESM::PRT_Groin, -1,1,"meshes\\" + groinPart->model);
                }
                if(partpriorities[ESM::PRT_RHand] < 1){
                    const ESM::BodyPart *handPart = mEnvironment.mWorld->getStore().bodyParts.search (bodyRaceID + "hand");
                    if(!handPart)
                        handPart = mEnvironment.mWorld->getStore().bodyParts.search (bodyRaceID + "hands");
                    if(handPart)
                        addOrReplaceIndividualPart(ESM::PRT_RHand, -1,1,"meshes\\" + handPart->model);
                }
                if(partpriorities[ESM::PRT_LHand] < 1){
                    const ESM::BodyPart *handPart = mEnvironment.mWorld->getStore().bodyParts.search (bodyRaceID + "hand");
                    if(!handPart)
                        handPart = mEnvironment.mWorld->getStore().bodyParts.search (bodyRaceID + "hands");
                    if(handPart)
                        addOrReplaceIndividualPart(ESM::PRT_LHand, -1,1,"meshes\\" + handPart->model);
                }

                if(partpriorities[ESM::PRT_RWrist] < 1){
                    const ESM::BodyPart *wristPart = mEnvironment.mWorld->getStore().bodyParts.search (bodyRaceID + "wrist");
                    if(wristPart)
                        addOrReplaceIndividualPart(ESM::PRT_RWrist, -1,1,"meshes\\" + wristPart->model);
                }
                 if(partpriorities[ESM::PRT_LWrist] < 1){
                    const ESM::BodyPart *wristPart = mEnvironment.mWorld->getStore().bodyParts.search (bodyRaceID + "wrist");
                    if(wristPart)
                        addOrReplaceIndividualPart(ESM::PRT_LWrist, -1,1,"meshes\\" + wristPart->model);
                }
                  if(partpriorities[ESM::PRT_RForearm] < 1){
                    const ESM::BodyPart *forearmPart = mEnvironment.mWorld->getStore().bodyParts.search (bodyRaceID + "forearm");
                    if(bodyRaceID == "b_n_argonian_f_")
                        forearmPart = mEnvironment.mWorld->getStore().bodyParts.search ("b_n_argonian_m_forearm");
                    if(forearmPart)
                        addOrReplaceIndividualPart(ESM::PRT_RForearm, -1,1,"meshes\\" + forearmPart->model);
                }
                 if(partpriorities[ESM::PRT_LForearm] < 1){
                    const ESM::BodyPart *forearmPart = mEnvironment.mWorld->getStore().bodyParts.search (bodyRaceID + "forearm");
                    if(bodyRaceID == "b_n_argonian_f_")
                        forearmPart = mEnvironment.mWorld->getStore().bodyParts.search ("b_n_argonian_m_forearm");
                    if(forearmPart)
                        addOrReplaceIndividualPart(ESM::PRT_LForearm, -1,1,"meshes\\" + forearmPart->model);
                }
                  if(partpriorities[ESM::PRT_RUpperarm] < 1){
                    const ESM::BodyPart *armPart = mEnvironment.mWorld->getStore().bodyParts.search (bodyRaceID + "upper arm");
                    if(armPart)
                        addOrReplaceIndividualPart(ESM::PRT_RUpperarm, -1,1,"meshes\\" + armPart->model);
                }
                 if(partpriorities[ESM::PRT_LUpperarm] < 1){
                    const ESM::BodyPart *armPart = mEnvironment.mWorld->getStore().bodyParts.search (bodyRaceID + "upper arm");
                    if(armPart)
                        addOrReplaceIndividualPart(ESM::PRT_LUpperarm, -1,1,"meshes\\" + armPart->model);
                }
                  if(partpriorities[ESM::PRT_RFoot] < 1){
                    const ESM::BodyPart *footPart = mEnvironment.mWorld->getStore().bodyParts.search (bodyRaceID + "foot");
                    if(isBeast && !footPart)
                        footPart = mEnvironment.mWorld->getStore().bodyParts.search (bodyRaceID + "feet");
                    if(footPart)
                        addOrReplaceIndividualPart(ESM::PRT_RFoot, -1,1,"meshes\\" + footPart->model);
                }
                  if(partpriorities[ESM::PRT_LFoot] < 1){
                    const ESM::BodyPart *footPart = mEnvironment.mWorld->getStore().bodyParts.search (bodyRaceID + "foot");
                    if(isBeast && !footPart)
                        footPart = mEnvironment.mWorld->getStore().bodyParts.search (bodyRaceID + "feet");
                    if(footPart)
                        addOrReplaceIndividualPart(ESM::PRT_LFoot, -1,1,"meshes\\" + footPart->model);
                }
                 if(partpriorities[ESM::PRT_RAnkle] < 1){
                    const ESM::BodyPart *anklePart = mEnvironment.mWorld->getStore().bodyParts.search (bodyRaceID + "ankle");
                    if(anklePart)
                        addOrReplaceIndividualPart(ESM::PRT_RAnkle, -1,1,"meshes\\" + anklePart->model);
                }
                 if(partpriorities[ESM::PRT_LAnkle] < 1){
                    const ESM::BodyPart *anklePart = mEnvironment.mWorld->getStore().bodyParts.search (bodyRaceID + "ankle");
                    if(anklePart)
                        addOrReplaceIndividualPart(ESM::PRT_LAnkle, -1,1,"meshes\\" + anklePart->model);
                }
                     if(partpriorities[ESM::PRT_RKnee] < 1){
                    const ESM::BodyPart *kneePart = mEnvironment.mWorld->getStore().bodyParts.search (bodyRaceID + "knee");
                    if(kneePart)
                        addOrReplaceIndividualPart(ESM::PRT_RKnee, -1,1,"meshes\\" + kneePart->model);
                }
                 if(partpriorities[ESM::PRT_LKnee] < 1){
                    const ESM::BodyPart *kneePart = mEnvironment.mWorld->getStore().bodyParts.search (bodyRaceID + "knee");
                    if(kneePart)
                        addOrReplaceIndividualPart(ESM::PRT_LKnee, -1,1,"meshes\\" + kneePart->model);
                }
                   if(partpriorities[ESM::PRT_RLeg] < 1){
                    const ESM::BodyPart *legPart = mEnvironment.mWorld->getStore().bodyParts.search (bodyRaceID + "upper leg");
                    if(legPart)
                        addOrReplaceIndividualPart(ESM::PRT_RLeg, -1,1,"meshes\\" + legPart->model);
                }
                 if(partpriorities[ESM::PRT_LLeg] < 1){
                    const ESM::BodyPart *legPart = mEnvironment.mWorld->getStore().bodyParts.search (bodyRaceID + "upper leg");
                    if(legPart)
                        addOrReplaceIndividualPart(ESM::PRT_LLeg, -1,1,"meshes\\" + legPart->model);
                }
                 if(partpriorities[ESM::PRT_Tail] < 1){
                    const ESM::BodyPart *tailPart = mEnvironment.mWorld->getStore().bodyParts.search (bodyRaceID + "tail");
                    if(tailPart)
                        addOrReplaceIndividualPart(ESM::PRT_Tail, -1,1,"meshes\\" + tailPart->model);
                }








}

Ogre::Entity* NpcAnimation::insertBoundedPart(const std::string &mesh, std::string bonename){

    NIFLoader::load(mesh);
    Ogre::Entity* part = mRend.getScene()->createEntity(mesh);
    part->setVisibilityFlags(RV_Actors);

    base->attachObjectToBone(bonename, part);
    return part;
}
void NpcAnimation::insertFootPart(int type, const std::string &mesh){
    std::string meshAndSuffix = mesh;
    if(type == ESM::PRT_LFoot)
        meshAndSuffix += "*|";
    NIFLoader::load(meshAndSuffix);
     Ogre::Entity* part = mRend.getScene()->createEntity(meshAndSuffix);
    std::vector<Nif::NiTriShapeCopy>* shape = ((NIFLoader::getSingletonPtr())->getShapes(meshAndSuffix));
    if(shape == 0){
        if(type == ESM::PRT_LFoot){
            base->attachObjectToBone("Left Foot", part);
            lfoot = part;
        }
        else if (type == ESM::PRT_RFoot){
            base->attachObjectToBone("Right Foot", part);
            rfoot = part;
        }
    }
    else{
        if(type == ESM::PRT_LFoot)
            lFreeFoot = insertFreePart(mesh, "::");
        else if (type == ESM::PRT_RFoot)
            rFreeFoot = insertFreePart(mesh, ":<");
    }



}
std::pair<Ogre::Entity*, std::vector<Nif::NiTriShapeCopy>*> NpcAnimation::insertFreePart(const std::string &mesh, const std::string suffix){

    std::string meshNumbered = mesh + getUniqueID(mesh + suffix) + suffix;
    NIFLoader::load(meshNumbered);

    Ogre::Entity* part = mRend.getScene()->createEntity(meshNumbered);
    part->setVisibilityFlags(RV_Actors);

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


			if(lFreeFoot.first)
				handleShapes(lFreeFoot.second, lFreeFoot.first, base->getSkeleton());
			if(rFreeFoot.first)
				handleShapes(rFreeFoot.second, rFreeFoot.first, base->getSkeleton());

			if(chest.first)
				handleShapes(chest.second, chest.first, base->getSkeleton());
			if(tail.first)
				handleShapes(tail.second, tail.first, base->getSkeleton());
			if(skirt.first){
				handleShapes(skirt.second, skirt.first, base->getSkeleton());
            }
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
            else if(rFreeFoot.first){
                insert->detachObject(rFreeFoot.first);
                rFreeFoot = zero;
            }
        }
        else if(type == ESM::PRT_LFoot){                //16
            if(lfoot){
                base->detachObjectFromBone(lfoot);
                lfoot = 0;
            }
            else if(lFreeFoot.first){
                insert->detachObject(lFreeFoot.first);
                lFreeFoot = zero;
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

    void NpcAnimation::reserveIndividualPart(int type, int group, int priority){
        if(priority > partpriorities[type]){
            removeIndividualPart(type);
            partpriorities[type] = priority;
            partslots[type] = group;
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
                    groin = insertBoundedPart(mesh, "Groin");
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
                   insertFootPart(type, mesh);
                    break;
                case ESM::PRT_LFoot:                             //16
                   insertFootPart(type, mesh);
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
        for(std::size_t i = 0; i < parts.size(); i++)
                {
                    ESM::PartReference part = parts[i];

                        const ESM::BodyPart *bodypart = 0;

                        if(isFemale)
                            bodypart = mEnvironment.mWorld->getStore().bodyParts.search (part.female);
                        if(!bodypart)
                            bodypart = mEnvironment.mWorld->getStore().bodyParts.search (part.male);
                        if(bodypart){
                            addOrReplaceIndividualPart(part.part, group,priority,"meshes\\" + bodypart->model);
                        }
                       else
                            reserveIndividualPart(part.part, group, priority);

                }
    }
}

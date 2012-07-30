#include "npcanimation.hpp"

#include <OgreSceneManager.h>
#include <OgreEntity.h>
#include <OgreSubEntity.h>

#include <components/esm_store/store.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"

#include "renderconst.hpp"

using namespace Ogre;
using namespace NifOgre;

namespace MWRender{
NpcAnimation::~NpcAnimation()
{
    removeEntities(head);
    removeEntities(hair);
    removeEntities(neck);
    removeEntities(chest);
    removeEntities(groin);
    removeEntities(skirt);
    removeEntities(rHand);
    removeEntities(lHand);
    removeEntities(rWrist);
    removeEntities(lWrist);
    removeEntities(rForearm);
    removeEntities(lForearm);
    removeEntities(rupperArm);
    removeEntities(lupperArm);
    removeEntities(rfoot);
    removeEntities(lfoot);
    removeEntities(rAnkle);
    removeEntities(lAnkle);
    removeEntities(rKnee);
    removeEntities(lKnee);
    removeEntities(rUpperLeg);
    removeEntities(lUpperLeg);
    removeEntities(rclavicle);
    removeEntities(lclavicle);
    removeEntities(tail);
}


NpcAnimation::NpcAnimation(const MWWorld::Ptr& ptr, OEngine::Render::OgreRenderer& _rend, MWWorld::InventoryStore& _inv)
  : Animation(_rend), mStateID(-1), mInv(_inv), timeToChange(0),
    robe(mInv.end()), helmet(mInv.end()), shirt(mInv.end()),
    cuirass(mInv.end()), greaves(mInv.end()),
    leftpauldron(mInv.end()), rightpauldron(mInv.end()),
    boots(mInv.end()),
    leftglove(mInv.end()), rightglove(mInv.end()), skirtiter(mInv.end()),
    pants(mInv.end())
{
    MWWorld::LiveCellRef<ESM::NPC> *ref = ptr.get<ESM::NPC>();

    for (int init = 0; init < 27; init++)
    {
        mPartslots[init] = -1;  //each slot is empty
        mPartPriorities[init] = 0;
    }

    const ESMS::ESMStore &store = MWBase::Environment::get().getWorld()->getStore();
    const ESM::Race *race = store.races.find(ref->base->race);

    std::string hairID = ref->base->hair;
    std::string headID = ref->base->head;
    headModel = "meshes\\" + store.bodyParts.find(headID)->model;
    hairModel = "meshes\\" + store.bodyParts.find(hairID)->model;
    npcName = ref->base->name;

    isFemale = !!(ref->base->flags&ESM::NPC::Female);
    isBeast = !!(race->data.flags&ESM::Race::Beast);

    bodyRaceID = "b_n_"+ref->base->race;
    std::transform(bodyRaceID.begin(), bodyRaceID.end(), bodyRaceID.begin(), ::tolower);

    /*std::cout << "Race: " << ref->base->race ;
    if(female)
        std::cout << " Sex: Female" << " Height: " << race->data.height.female << "\n";
    else
        std::cout << " Sex: Male" << " Height: " << race->data.height.male << "\n";
    */

    mInsert = ptr.getRefData().getBaseNode();
    assert(mInsert);

    std::string smodel = (!isBeast ? "meshes\\base_anim.nif" : "meshes\\base_animkna.nif");

    mEntityList = NifOgre::NIFLoader::createEntities(mInsert, &mTextKeys, smodel);
    for(size_t i = 0;i < mEntityList.mEntities.size();i++)
    {
        Ogre::Entity *base = mEntityList.mEntities[i];

        base->setVisibilityFlags(RV_Actors);
        bool transparent = false;
        for(unsigned int j=0;j < base->getNumSubEntities();++j)
        {
            Ogre::MaterialPtr mat = base->getSubEntity(j)->getMaterial();
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

    if(isFemale)
        mInsert->scale(race->data.height.female, race->data.height.female, race->data.height.female);
    else
        mInsert->scale(race->data.height.male, race->data.height.male, race->data.height.male);
    updateParts();
}

void NpcAnimation::updateParts()
{
    bool apparelChanged = false;

    const struct {
        MWWorld::ContainerStoreIterator *iter;
        int slot;
    } slotlist[] = {
        { &robe, MWWorld::InventoryStore::Slot_Robe },
        { &skirtiter, MWWorld::InventoryStore::Slot_Skirt },
        { &helmet, MWWorld::InventoryStore::Slot_Helmet },
        { &cuirass, MWWorld::InventoryStore::Slot_Cuirass },
        { &greaves, MWWorld::InventoryStore::Slot_Greaves },
        { &leftpauldron, MWWorld::InventoryStore::Slot_LeftPauldron },
        { &rightpauldron, MWWorld::InventoryStore::Slot_RightPauldron },
        { &boots, MWWorld::InventoryStore::Slot_Boots },
        { &leftglove, MWWorld::InventoryStore::Slot_LeftGauntlet },
        { &rightglove, MWWorld::InventoryStore::Slot_RightGauntlet },
        { &shirt, MWWorld::InventoryStore::Slot_Shirt },
        { &pants, MWWorld::InventoryStore::Slot_Pants },
    };
    for(size_t i = 0;i < sizeof(slotlist)/sizeof(slotlist[0]);i++)
    {
        MWWorld::ContainerStoreIterator iter = mInv.getSlot(slotlist[i].slot);
        if(*slotlist[i].iter != iter)
        {
            *slotlist[i].iter = iter;
            removePartGroup(slotlist[i].slot);
            apparelChanged = true;
        }
    }

    if(apparelChanged)
    {
        if(robe != mInv.end())
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
        if(skirtiter != mInv.end())
        {
            MWWorld::Ptr ptr = *skirtiter;

            const ESM::Clothing *clothes = (ptr.get<ESM::Clothing>())->base;
            std::vector<ESM::PartReference> parts = clothes->parts.parts;
            addPartGroup(MWWorld::InventoryStore::Slot_Skirt, 4, parts);
            reserveIndividualPart(ESM::PRT_Groin, MWWorld::InventoryStore::Slot_Skirt, 4);
            reserveIndividualPart(ESM::PRT_RLeg, MWWorld::InventoryStore::Slot_Skirt, 4);
            reserveIndividualPart(ESM::PRT_LLeg, MWWorld::InventoryStore::Slot_Skirt, 4);
        }

        if(helmet != mInv.end())
        {
            removeIndividualPart(ESM::PRT_Hair);
            const ESM::Armor *armor = (helmet->get<ESM::Armor>())->base;
            std::vector<ESM::PartReference> parts = armor->parts.parts;
            addPartGroup(MWWorld::InventoryStore::Slot_Helmet, 3, parts);
        }
        if(cuirass != mInv.end())
        {
            const ESM::Armor *armor = (cuirass->get<ESM::Armor>())->base;
            std::vector<ESM::PartReference> parts = armor->parts.parts;
            addPartGroup(MWWorld::InventoryStore::Slot_Cuirass, 3, parts);
        }
        if(greaves != mInv.end())
        {
            const ESM::Armor *armor = (greaves->get<ESM::Armor>())->base;
            std::vector<ESM::PartReference> parts = armor->parts.parts;
            addPartGroup(MWWorld::InventoryStore::Slot_Greaves, 3, parts);
        }

        if(leftpauldron != mInv.end())
        {
            const ESM::Armor *armor = (leftpauldron->get<ESM::Armor>())->base;
            std::vector<ESM::PartReference> parts = armor->parts.parts;
            addPartGroup(MWWorld::InventoryStore::Slot_LeftPauldron, 3, parts);
        }
        if(rightpauldron != mInv.end())
        {
            const ESM::Armor *armor = (rightpauldron->get<ESM::Armor>())->base;
            std::vector<ESM::PartReference> parts = armor->parts.parts;
            addPartGroup(MWWorld::InventoryStore::Slot_RightPauldron, 3, parts);
        }
        if(boots != mInv.end())
        {
            if(boots->getTypeName() == typeid(ESM::Clothing).name())
            {
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
        if(leftglove != mInv.end())
        {
            if(leftglove->getTypeName() == typeid(ESM::Clothing).name())
            {
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
        if(rightglove != mInv.end())
        {
            if(rightglove->getTypeName() == typeid(ESM::Clothing).name())
            {
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

        if(shirt != mInv.end())
        {
            const ESM::Clothing *clothes = (shirt->get<ESM::Clothing>())->base;
            std::vector<ESM::PartReference> parts = clothes->parts.parts;
            addPartGroup(MWWorld::InventoryStore::Slot_Shirt, 2, parts);
        }
        if(pants != mInv.end())
        {
            const ESM::Clothing *clothes = (pants->get<ESM::Clothing>())->base;
            std::vector<ESM::PartReference> parts = clothes->parts.parts;
            addPartGroup(MWWorld::InventoryStore::Slot_Pants, 2, parts);
        }
    }

    if(mPartPriorities[ESM::PRT_Head] < 1)
        addOrReplaceIndividualPart(ESM::PRT_Head, -1,1, headModel);
    if(mPartPriorities[ESM::PRT_Hair] < 1 && mPartPriorities[ESM::PRT_Head] <= 1)
        addOrReplaceIndividualPart(ESM::PRT_Hair, -1,1, hairModel);

    static const struct {
        ESM::PartReferenceType type;
        const char name[2][12];
    } PartTypeList[] = {
        { ESM::PRT_Neck,      { "neck", "" } },
        { ESM::PRT_Cuirass,   { "chest", "" } },
        { ESM::PRT_Groin,     { "groin", "" } },
        { ESM::PRT_RHand,     { "hand", "hands" } },
        { ESM::PRT_LHand,     { "hand", "hands" } },
        { ESM::PRT_RWrist,    { "wrist", "" } },
        { ESM::PRT_LWrist,    { "wrist", "" } },
        { ESM::PRT_RForearm,  { "forearm", "" } },
        { ESM::PRT_LForearm,  { "forearm", "" } },
        { ESM::PRT_RUpperarm, { "upper arm", "" } },
        { ESM::PRT_LUpperarm, { "upper arm", "" } },
        { ESM::PRT_RFoot,     { "foot", "feet" } },
        { ESM::PRT_LFoot,     { "foot", "feet" } },
        { ESM::PRT_RAnkle,    { "ankle", "" } },
        { ESM::PRT_LAnkle,    { "ankle", "" } },
        { ESM::PRT_RKnee,     { "knee", "" } },
        { ESM::PRT_LKnee,     { "knee", "" } },
        { ESM::PRT_RLeg,      { "upper leg", "" } },
        { ESM::PRT_LLeg,      { "upper leg", "" } },
        { ESM::PRT_Tail,      { "tail", "" } }
    };

    const ESMS::ESMStore &store = MWBase::Environment::get().getWorld()->getStore();
    for(size_t i = 0;i < sizeof(PartTypeList)/sizeof(PartTypeList[0]);i++)
    {
        if(mPartPriorities[PartTypeList[i].type] < 1)
        {
            const ESM::BodyPart *part = NULL;
            bool tryfemale = isFemale;
            int ni = 0;
            do {
                part = store.bodyParts.search(bodyRaceID+(tryfemale?"_f_":"_m_")+PartTypeList[i].name[ni]);
                if(part) break;

                ni ^= 1;
                if(ni == 0)
                {
                    if(!tryfemale)
                        break;
                    tryfemale = false;
                }
            } while(1);

            if(part)
                addOrReplaceIndividualPart(PartTypeList[i].type, -1,1, "meshes\\"+part->model);
        }
    }
}

NifOgre::EntityList NpcAnimation::insertBoundedPart(const std::string &mesh, const std::string &bonename)
{
    NifOgre::EntityList entities = NIFLoader::createEntities(mEntityList.mSkelBase, bonename,
                                                             mInsert, mesh);
    std::vector<Ogre::Entity*> &parts = entities.mEntities;
    for(size_t i = 0;i < parts.size();i++)
        parts[i]->setVisibilityFlags(RV_Actors);
    return entities;
}

void NpcAnimation::runAnimation(float timepassed)
{
    if(timeToChange > .2)
    {
        timeToChange = 0;
        updateParts();
    }
    timeToChange += timepassed;

    Animation::runAnimation(timepassed);
}

void NpcAnimation::removeEntities(NifOgre::EntityList &entities)
{
    assert(&entities != &mEntityList);

    Ogre::SceneManager *sceneMgr = mInsert->getCreator();
    for(size_t i = 0;i < entities.mEntities.size();i++)
    {
        entities.mEntities[i]->detachFromParent();
        sceneMgr->destroyEntity(entities.mEntities[i]);
    }
    entities.mEntities.clear();
    entities.mSkelBase = NULL;
}

void NpcAnimation::removeIndividualPart(int type)
{
    mPartPriorities[type] = 0;
    mPartslots[type] = -1;

    if(type == ESM::PRT_Head)   //0
        removeEntities(head);
    else if(type == ESM::PRT_Hair) //1
        removeEntities(hair);
    else if(type == ESM::PRT_Neck) //2
        removeEntities(neck);
    else if(type == ESM::PRT_Cuirass)//3
        removeEntities(chest);
    else if(type == ESM::PRT_Groin)//4
        removeEntities(groin);
    else if(type == ESM::PRT_Skirt)//5
        removeEntities(skirt);
    else if(type == ESM::PRT_RHand)//6
        removeEntities(rHand);
    else if(type == ESM::PRT_LHand)//7
        removeEntities(lHand);
    else if(type == ESM::PRT_RWrist)//8
        removeEntities(rWrist);
    else if(type == ESM::PRT_LWrist) //9
        removeEntities(lWrist);
    else if(type == ESM::PRT_Shield) //10
    {
    }
    else if(type == ESM::PRT_RForearm) //11
        removeEntities(rForearm);
    else if(type == ESM::PRT_LForearm) //12
        removeEntities(lForearm);
    else if(type == ESM::PRT_RUpperarm) //13
        removeEntities(rupperArm);
    else if(type == ESM::PRT_LUpperarm) //14
        removeEntities(lupperArm);
    else if(type == ESM::PRT_RFoot)                 //15
        removeEntities(rfoot);
    else if(type == ESM::PRT_LFoot)                //16
        removeEntities(lfoot);
    else if(type == ESM::PRT_RAnkle)    //17
        removeEntities(rAnkle);
    else if(type == ESM::PRT_LAnkle)    //18
        removeEntities(lAnkle);
    else if(type == ESM::PRT_RKnee)    //19
        removeEntities(rKnee);
    else if(type == ESM::PRT_LKnee)    //20
        removeEntities(lKnee);
    else if(type == ESM::PRT_RLeg)    //21
        removeEntities(rUpperLeg);
    else if(type == ESM::PRT_LLeg)    //22
        removeEntities(lUpperLeg);
    else if(type == ESM::PRT_RPauldron)    //23
        removeEntities(rclavicle);
    else if(type == ESM::PRT_LPauldron)    //24
        removeEntities(lclavicle);
    else if(type == ESM::PRT_Weapon)                 //25
    {
    }
    else if(type == ESM::PRT_Tail)    //26
        removeEntities(tail);
}

void NpcAnimation::reserveIndividualPart(int type, int group, int priority)
{
    if(priority > mPartPriorities[type])
    {
        removeIndividualPart(type);
        mPartPriorities[type] = priority;
        mPartslots[type] = group;
    }
}

void NpcAnimation::removePartGroup(int group)
{
    for(int i = 0; i < 27; i++)
    {
        if(mPartslots[i] == group)
            removeIndividualPart(i);
    }
}

bool NpcAnimation::addOrReplaceIndividualPart(int type, int group, int priority, const std::string &mesh)
{
    if(priority <= mPartPriorities[type])
        return false;

    removeIndividualPart(type);
    mPartslots[type] = group;
    mPartPriorities[type] = priority;
    switch(type)
    {
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
            chest = insertBoundedPart(mesh, "Chest");
            break;
        case ESM::PRT_Groin:                          //4
            groin = insertBoundedPart(mesh, "Groin");
            break;
        case ESM::PRT_Skirt:                          //5
            skirt = insertBoundedPart(mesh, "Groin");
            break;
        case ESM::PRT_RHand:                         //6
            rHand = insertBoundedPart(mesh, "Right Hand");
            break;
        case ESM::PRT_LHand:                         //7
            lHand = insertBoundedPart(mesh, "Left Hand");
            break;
        case ESM::PRT_RWrist:                          //8
            rWrist = insertBoundedPart(mesh, "Right Wrist");
            break;
        case ESM::PRT_LWrist:                          //9
            lWrist = insertBoundedPart(mesh, "Left Wrist");
            break;
        case ESM::PRT_Shield:                         //10
            break;
        case ESM::PRT_RForearm:                          //11
            rForearm = insertBoundedPart(mesh, "Right Forearm");
            break;
        case ESM::PRT_LForearm:                          //12
            lForearm = insertBoundedPart(mesh, "Left Forearm");
            break;
        case ESM::PRT_RUpperarm:                          //13
            rupperArm = insertBoundedPart(mesh, "Right Upper Arm");
            break;
        case ESM::PRT_LUpperarm:                          //14
            lupperArm = insertBoundedPart(mesh, "Left Upper Arm");
            break;
        case ESM::PRT_RFoot:                             //15
            rfoot = insertBoundedPart(mesh, "Right Foot");
            break;
        case ESM::PRT_LFoot:                             //16
            lfoot = insertBoundedPart(mesh, "Left Foot");
            break;
        case ESM::PRT_RAnkle:                          //17
            rAnkle = insertBoundedPart(mesh, "Right Ankle");
            break;
        case ESM::PRT_LAnkle:                          //18
            lAnkle = insertBoundedPart(mesh, "Left Ankle");
            break;
        case ESM::PRT_RKnee:                          //19
            rKnee = insertBoundedPart(mesh, "Right Knee");
            break;
        case ESM::PRT_LKnee:                          //20
            lKnee = insertBoundedPart(mesh, "Left Knee");
            break;
        case ESM::PRT_RLeg:                          //21
            rUpperLeg = insertBoundedPart(mesh, "Right Upper Leg");
            break;
        case ESM::PRT_LLeg:                          //22
            lUpperLeg = insertBoundedPart(mesh, "Left Upper Leg");
            break;
        case ESM::PRT_RPauldron:                          //23
            rclavicle = insertBoundedPart(mesh , "Right Clavicle");
            break;
        case ESM::PRT_LPauldron:                          //24
            lclavicle = insertBoundedPart(mesh, "Left Clavicle");
            break;
        case ESM::PRT_Weapon:                             //25
            break;
        case ESM::PRT_Tail:                              //26
            tail = insertBoundedPart(mesh, "Tail");
            break;
    }
    return true;
}

void NpcAnimation::addPartGroup(int group, int priority, std::vector<ESM::PartReference> &parts)
{
    for(std::size_t i = 0; i < parts.size(); i++)
    {
        ESM::PartReference &part = parts[i];

        const ESM::BodyPart *bodypart = 0;
        if(isFemale)
            bodypart = MWBase::Environment::get().getWorld()->getStore().bodyParts.search(part.female);
        if(!bodypart)
            bodypart = MWBase::Environment::get().getWorld()->getStore().bodyParts.search(part.male);

        if(bodypart)
            addOrReplaceIndividualPart(part.part, group,priority,"meshes\\" + bodypart->model);
        else
            reserveIndividualPart(part.part, group, priority);
    }
}

}

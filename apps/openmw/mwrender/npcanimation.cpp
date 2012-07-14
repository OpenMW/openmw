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
}


NpcAnimation::NpcAnimation(const MWWorld::Ptr& ptr, OEngine::Render::OgreRenderer& _rend, MWWorld::InventoryStore& _inv)
  : Animation(_rend), mStateID(-1), inv(_inv), timeToChange(0),
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
    MWWorld::LiveCellRef<ESM::NPC> *ref = ptr.get<ESM::NPC>();

    for (int init = 0; init < 27; init++)
    {
        partslots[init] = -1;  //each slot is empty
        partpriorities[init] = 0;
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

    bodyRaceID = "b_n_"+race->name;
    std::transform(bodyRaceID.begin(), bodyRaceID.end(), bodyRaceID.begin(), ::tolower);

    /*std::cout << "Race: " << ref->base->race ;
    if(female)
        std::cout << " Sex: Female" << " Height: " << race->data.height.female << "\n";
    else
        std::cout << " Sex: Male" << " Height: " << race->data.height.male << "\n";
    */

    insert = ptr.getRefData().getBaseNode();
    assert(insert);

    std::string smodel = (!isBeast ? "meshes\\base_anim.nif" : "meshes\\base_animkna.nif");

    // FIXME: There can be more than one!
    NifOgre::MeshPairList meshes = NifOgre::NIFLoader::load(smodel);
    base = mRend.getScene()->createEntity(meshes[0].first->getName());

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

    insert->attachObject(base);
    if(isFemale)
        insert->scale(race->data.height.female, race->data.height.female, race->data.height.female);
    else
        insert->scale(race->data.height.male, race->data.height.male, race->data.height.male);

    updateParts();
}

void NpcAnimation::updateParts()
{
    bool apparelChanged = false;

    //inv.getSlot(MWWorld::InventoryStore::Slot_Robe);
    if(robe != inv.getSlot(MWWorld::InventoryStore::Slot_Robe))
    {
        // A robe was added or removed
        robe = inv.getSlot(MWWorld::InventoryStore::Slot_Robe);
        removePartGroup(MWWorld::InventoryStore::Slot_Robe);
        apparelChanged = true;
    }
    if(skirtiter != inv.getSlot(MWWorld::InventoryStore::Slot_Skirt))
    {
        skirtiter = inv.getSlot(MWWorld::InventoryStore::Slot_Skirt);
        removePartGroup(MWWorld::InventoryStore::Slot_Skirt);
        apparelChanged = true;
    }
    if(helmet != inv.getSlot(MWWorld::InventoryStore::Slot_Helmet))
    {
        helmet = inv.getSlot(MWWorld::InventoryStore::Slot_Helmet);
        removePartGroup(MWWorld::InventoryStore::Slot_Helmet);
        apparelChanged = true;
    }
    if(cuirass != inv.getSlot(MWWorld::InventoryStore::Slot_Cuirass))
    {
        cuirass = inv.getSlot(MWWorld::InventoryStore::Slot_Cuirass);
        removePartGroup(MWWorld::InventoryStore::Slot_Cuirass);
        apparelChanged = true;
    }
    if(greaves != inv.getSlot(MWWorld::InventoryStore::Slot_Greaves))
    {
        greaves = inv.getSlot(MWWorld::InventoryStore::Slot_Greaves);
        removePartGroup(MWWorld::InventoryStore::Slot_Greaves);
        apparelChanged = true;
    }
    if(leftpauldron != inv.getSlot(MWWorld::InventoryStore::Slot_LeftPauldron))
    {
        leftpauldron = inv.getSlot(MWWorld::InventoryStore::Slot_LeftPauldron);
        removePartGroup(MWWorld::InventoryStore::Slot_LeftPauldron);
        apparelChanged = true;
    }
    if(rightpauldron != inv.getSlot(MWWorld::InventoryStore::Slot_RightPauldron))
    {
        rightpauldron = inv.getSlot(MWWorld::InventoryStore::Slot_RightPauldron);
        removePartGroup(MWWorld::InventoryStore::Slot_RightPauldron);
        apparelChanged = true;
    }
    if(!isBeast && boots != inv.getSlot(MWWorld::InventoryStore::Slot_Boots))
    {
        boots = inv.getSlot(MWWorld::InventoryStore::Slot_Boots);
        removePartGroup(MWWorld::InventoryStore::Slot_Boots);
        apparelChanged = true;
    }
    if(leftglove != inv.getSlot(MWWorld::InventoryStore::Slot_LeftGauntlet))
    {
        leftglove = inv.getSlot(MWWorld::InventoryStore::Slot_LeftGauntlet);
        removePartGroup(MWWorld::InventoryStore::Slot_LeftGauntlet);
        apparelChanged = true;
    }
    if(rightglove != inv.getSlot(MWWorld::InventoryStore::Slot_RightGauntlet))
    {
        rightglove = inv.getSlot(MWWorld::InventoryStore::Slot_RightGauntlet);
        removePartGroup(MWWorld::InventoryStore::Slot_RightGauntlet);
        apparelChanged = true;
    }
    if(shirt != inv.getSlot(MWWorld::InventoryStore::Slot_Shirt))
    {
        shirt = inv.getSlot(MWWorld::InventoryStore::Slot_Shirt);
        removePartGroup(MWWorld::InventoryStore::Slot_Shirt);
        apparelChanged = true;
    }
    if(pants != inv.getSlot(MWWorld::InventoryStore::Slot_Pants))
    {
        pants = inv.getSlot(MWWorld::InventoryStore::Slot_Pants);
        removePartGroup(MWWorld::InventoryStore::Slot_Pants);
        apparelChanged = true;
    }

    if(apparelChanged)
    {
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

        if(helmet != inv.end())
        {
            removeIndividualPart(ESM::PRT_Hair);
            const ESM::Armor *armor = (helmet->get<ESM::Armor>())->base;
            std::vector<ESM::PartReference> parts = armor->parts.parts;
            addPartGroup(MWWorld::InventoryStore::Slot_Helmet, 3, parts);
        }
        if(cuirass != inv.end())
        {
            const ESM::Armor *armor = (cuirass->get<ESM::Armor>())->base;
            std::vector<ESM::PartReference> parts = armor->parts.parts;
            addPartGroup(MWWorld::InventoryStore::Slot_Cuirass, 3, parts);
        }
        if(greaves != inv.end())
        {
            const ESM::Armor *armor = (greaves->get<ESM::Armor>())->base;
            std::vector<ESM::PartReference> parts = armor->parts.parts;
            addPartGroup(MWWorld::InventoryStore::Slot_Greaves, 3, parts);
        }

        if(leftpauldron != inv.end())
        {
            const ESM::Armor *armor = (leftpauldron->get<ESM::Armor>())->base;
            std::vector<ESM::PartReference> parts = armor->parts.parts;
            addPartGroup(MWWorld::InventoryStore::Slot_LeftPauldron, 3, parts);
        }
        if(rightpauldron != inv.end())
        {
            const ESM::Armor *armor = (rightpauldron->get<ESM::Armor>())->base;
            std::vector<ESM::PartReference> parts = armor->parts.parts;
            addPartGroup(MWWorld::InventoryStore::Slot_RightPauldron, 3, parts);
        }
        if(!isBeast && boots != inv.end())
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
        if(leftglove != inv.end())
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
        if(rightglove != inv.end())
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

        if(shirt != inv.end())
        {
            const ESM::Clothing *clothes = (shirt->get<ESM::Clothing>())->base;
            std::vector<ESM::PartReference> parts = clothes->parts.parts;
            addPartGroup(MWWorld::InventoryStore::Slot_Shirt, 2, parts);
        }
        if(pants != inv.end())
        {
            const ESM::Clothing *clothes = (pants->get<ESM::Clothing>())->base;
            std::vector<ESM::PartReference> parts = clothes->parts.parts;
            addPartGroup(MWWorld::InventoryStore::Slot_Pants, 2, parts);
        }
    }

    if(partpriorities[ESM::PRT_Head] < 1)
        addOrReplaceIndividualPart(ESM::PRT_Head, -1,1, headModel);
    if(partpriorities[ESM::PRT_Hair] < 1 && partpriorities[ESM::PRT_Head] <= 1)
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
        if(partpriorities[PartTypeList[i].type] < 1)
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

Ogre::Entity* NpcAnimation::insertBoundedPart(const std::string &mesh, const std::string &bonename)
{
    // FIXME: There can be more than one!
    NifOgre::MeshPairList meshes = NIFLoader::load(mesh);
    Ogre::Entity* part = mRend.getScene()->createEntity(meshes[0].first->getName());
    part->setVisibilityFlags(RV_Actors);

    base->attachObjectToBone(bonename, part);
    return part;
}

void NpcAnimation::runAnimation(float timepassed)
{
    if(timeToChange > .2)
    {
        timeToChange = 0;
        updateParts();
    }

    timeToChange += timepassed;

    //1. Add the amount of time passed to time

    //2. Handle the animation transforms dependent on time

    //3. Handle the shapes dependent on animation transforms
    if(animate > 0)
    {
        time += timepassed;
        if(time > stopTime)
        {
            animate--;
            if(animate == 0)
                time = stopTime;
            else
                time = startTime + (time - stopTime);
        }

        handleAnimationTransforms();
    }
}

void NpcAnimation::removeIndividualPart(int type)
{
    partpriorities[type] = 0;
    partslots[type] = -1;

    if(type == ESM::PRT_Head && head)   //0
    {
        base->detachObjectFromBone(head);
        head = 0;
    }
    else if(type == ESM::PRT_Hair && hair) //1
    {
        base->detachObjectFromBone(hair);
        hair = 0;
    }
    else if(type == ESM::PRT_Neck && neck) //2
    {
        base->detachObjectFromBone(neck);
        neck = 0;
    }
    else if(type == ESM::PRT_Groin && groin)//4
    {
        base->detachObjectFromBone(groin);
        groin = 0;
    }
    else if(type == ESM::PRT_RWrist && rWrist)//8
    {
        base->detachObjectFromBone(rWrist);
        rWrist = 0;
    }
    else if(type == ESM::PRT_LWrist && lWrist) //9
    {
        base->detachObjectFromBone(lWrist);
        lWrist = 0;
    }
    else if(type == ESM::PRT_Shield) //10
    {
    }
    else if(type == ESM::PRT_RForearm && rForearm) //11
    {
        base->detachObjectFromBone(rForearm);
        rForearm = 0;
    }
    else if(type == ESM::PRT_LForearm && lForearm) //12
    {
        base->detachObjectFromBone(lForearm);
        lForearm = 0;
    }
    else if(type == ESM::PRT_RUpperarm && rupperArm) //13
    {
        base->detachObjectFromBone(rupperArm);
        rupperArm = 0;
    }
    else if(type == ESM::PRT_LUpperarm && lupperArm) //14
    {
        base->detachObjectFromBone(lupperArm);
        lupperArm = 0;
    }
    else if(type == ESM::PRT_RFoot && rfoot)                 //15
    {
        base->detachObjectFromBone(rfoot);
        rfoot = 0;
    }
    else if(type == ESM::PRT_LFoot && lfoot)                //16
    {
        base->detachObjectFromBone(lfoot);
        lfoot = 0;
    }
    else if(type == ESM::PRT_RAnkle && rAnkle)    //17
    {
        base->detachObjectFromBone(rAnkle);
        rAnkle = 0;
    }
    else if(type == ESM::PRT_LAnkle && lAnkle)    //18
    {
        base->detachObjectFromBone(lAnkle);
        lAnkle = 0;
    }
    else if(type == ESM::PRT_RKnee && rKnee)    //19
    {
        base->detachObjectFromBone(rKnee);
        rKnee = 0;
    }
    else if(type == ESM::PRT_LKnee && lKnee)    //20
    {
        base->detachObjectFromBone(lKnee);
        lKnee = 0;
    }
    else if(type == ESM::PRT_RLeg && rUpperLeg)    //21
    {
        base->detachObjectFromBone(rUpperLeg);
        rUpperLeg = 0;
    }
    else if(type == ESM::PRT_LLeg && lUpperLeg)    //22
    {
        base->detachObjectFromBone(lUpperLeg);
        lUpperLeg = 0;
    }
    else if(type == ESM::PRT_RPauldron && rclavicle)    //23
    {
        base->detachObjectFromBone(rclavicle);
        rclavicle = 0;
    }
    else if(type == ESM::PRT_LPauldron && lclavicle)    //24
    {
        base->detachObjectFromBone(lclavicle);
        lclavicle = 0;
    }
    else if(type == ESM::PRT_Weapon)                 //25
    {
    }
}

void NpcAnimation::reserveIndividualPart(int type, int group, int priority)
{
    if(priority > partpriorities[type])
    {
        removeIndividualPart(type);
        partpriorities[type] = priority;
        partslots[type] = group;
    }
}

void NpcAnimation::removePartGroup(int group)
{
    for(int i = 0; i < 27; i++)
    {
        if(partslots[i] == group)
            removeIndividualPart(i);
    }
}

bool NpcAnimation::addOrReplaceIndividualPart(int type, int group, int priority, const std::string &mesh)
{
    if(priority <= partpriorities[type])
        return false;

    removeIndividualPart(type);
    partslots[type] = group;
    partpriorities[type] = priority;
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
            break;
        case ESM::PRT_Groin:                          //4
            groin = insertBoundedPart(mesh, "Groin");
            break;
        case ESM::PRT_Skirt:                          //5
            break;
        case ESM::PRT_RHand:                         //6
            break;
        case ESM::PRT_LHand:                         //7
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
            lupperArm = insertBoundedPart(mesh, "Right Foot");
            break;
        case ESM::PRT_LFoot:                             //16
            lupperArm = insertBoundedPart(mesh, "Left Foot");
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

#include "npcanimation.hpp"

#include <OgreSceneManager.h>
#include <OgreEntity.h>
#include <OgreSubEntity.h>

#include "../mwworld/esmstore.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"

#include "renderconst.hpp"

using namespace Ogre;
using namespace NifOgre;

namespace MWRender{
NpcAnimation::~NpcAnimation()
{
    removeEntities(mHead);
    removeEntities(mHair);
    removeEntities(mNeck);
    removeEntities(mChest);
    removeEntities(mGroin);
    removeEntities(mSkirt);
    removeEntities(mHandL);
    removeEntities(mHandR);
    removeEntities(mWristL);
    removeEntities(mWristR);
    removeEntities(mForearmL);
    removeEntities(mForearmR);
    removeEntities(mUpperArmL);
    removeEntities(mUpperArmR);
    removeEntities(mFootL);
    removeEntities(mFootR);
    removeEntities(mAnkleL);
    removeEntities(mAnkleR);
    removeEntities(mKneeL);
    removeEntities(mKneeR);
    removeEntities(mUpperLegL);
    removeEntities(mUpperLegR);
    removeEntities(mClavicleL);
    removeEntities(mClavicleR);
    removeEntities(mTail);
}


NpcAnimation::NpcAnimation(const MWWorld::Ptr& ptr, Ogre::SceneNode* node, MWWorld::InventoryStore& inv, int visibilityFlags)
  : Animation(),
    mStateID(-1),
    mInv(inv),
    mTimeToChange(0),
    mVisibilityFlags(visibilityFlags),
    mRobe(mInv.end()),
    mHelmet(mInv.end()),
    mShirt(mInv.end()),
    mCuirass(mInv.end()),
    mGreaves(mInv.end()),
    mPauldronL(mInv.end()),
    mPauldronR(mInv.end()),
    mBoots(mInv.end()),
    mPants(mInv.end()),
    mGloveL(mInv.end()),
    mGloveR(mInv.end()),
    mSkirtIter(mInv.end())
{
    mNpc = ptr.get<ESM::NPC>()->mBase;

    for (int init = 0; init < 27; init++)
    {
        mPartslots[init] = -1;  //each slot is empty
        mPartPriorities[init] = 0;
    }

    const MWWorld::ESMStore &store =
        MWBase::Environment::get().getWorld()->getStore();
    const ESM::Race *race = store.get<ESM::Race>().find(mNpc->mRace);

    mHeadModel = "meshes\\" + store.get<ESM::BodyPart>().find(mNpc->mHead)->mModel;
    mHairModel = "meshes\\" + store.get<ESM::BodyPart>().find(mNpc->mHair)->mModel;

    mBodyPrefix = "b_n_" + mNpc->mRace;
    std::transform(mBodyPrefix.begin(), mBodyPrefix.end(), mBodyPrefix.begin(), ::tolower);

    mInsert = node;
    assert(mInsert);

    bool isBeast = (race->mData.mFlags & ESM::Race::Beast) != 0;
    std::string smodel = (!isBeast ? "meshes\\base_anim.nif" : "meshes\\base_animkna.nif");

    mEntityList = NifOgre::NIFLoader::createEntities(mInsert, &mTextKeys, smodel);
    for(size_t i = 0;i < mEntityList.mEntities.size();i++)
    {
        Ogre::Entity *base = mEntityList.mEntities[i];

        base->getUserObjectBindings ().setUserAny (Ogre::Any(-1));

        base->setVisibilityFlags(mVisibilityFlags);
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

    float scale = race->mData.mHeight.mMale;
    if (!mNpc->isMale()) {
        scale = race->mData.mHeight.mFemale;
    }
    mInsert->scale(scale, scale, scale);

    updateParts();
}

void NpcAnimation::updateParts()
{
    bool apparelChanged = false;

    const struct {
        MWWorld::ContainerStoreIterator *iter;
        int slot;
    } slotlist[] = {
        { &mRobe, MWWorld::InventoryStore::Slot_Robe },
        { &mSkirtIter, MWWorld::InventoryStore::Slot_Skirt },
        { &mHelmet, MWWorld::InventoryStore::Slot_Helmet },
        { &mCuirass, MWWorld::InventoryStore::Slot_Cuirass },
        { &mGreaves, MWWorld::InventoryStore::Slot_Greaves },
        { &mPauldronL, MWWorld::InventoryStore::Slot_LeftPauldron },
        { &mPauldronR, MWWorld::InventoryStore::Slot_RightPauldron },
        { &mBoots, MWWorld::InventoryStore::Slot_Boots },
        { &mGloveL, MWWorld::InventoryStore::Slot_LeftGauntlet },
        { &mGloveR, MWWorld::InventoryStore::Slot_RightGauntlet },
        { &mShirt, MWWorld::InventoryStore::Slot_Shirt },
        { &mPants, MWWorld::InventoryStore::Slot_Pants },
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
        if(mRobe != mInv.end())
        {
            MWWorld::Ptr ptr = *mRobe;

            const ESM::Clothing *clothes = (ptr.get<ESM::Clothing>())->mBase;
            std::vector<ESM::PartReference> parts = clothes->mParts.mParts;
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
        if(mSkirtIter != mInv.end())
        {
            MWWorld::Ptr ptr = *mSkirtIter;

            const ESM::Clothing *clothes = (ptr.get<ESM::Clothing>())->mBase;
            std::vector<ESM::PartReference> parts = clothes->mParts.mParts;
            addPartGroup(MWWorld::InventoryStore::Slot_Skirt, 4, parts);
            reserveIndividualPart(ESM::PRT_Groin, MWWorld::InventoryStore::Slot_Skirt, 4);
            reserveIndividualPart(ESM::PRT_RLeg, MWWorld::InventoryStore::Slot_Skirt, 4);
            reserveIndividualPart(ESM::PRT_LLeg, MWWorld::InventoryStore::Slot_Skirt, 4);
        }

        if(mHelmet != mInv.end())
        {
            removeIndividualPart(ESM::PRT_Hair);
            const ESM::Armor *armor = (mHelmet->get<ESM::Armor>())->mBase;
            std::vector<ESM::PartReference> parts = armor->mParts.mParts;
            addPartGroup(MWWorld::InventoryStore::Slot_Helmet, 3, parts);
        }
        if(mCuirass != mInv.end())
        {
            const ESM::Armor *armor = (mCuirass->get<ESM::Armor>())->mBase;
            std::vector<ESM::PartReference> parts = armor->mParts.mParts;
            addPartGroup(MWWorld::InventoryStore::Slot_Cuirass, 3, parts);
        }
        if(mGreaves != mInv.end())
        {
            const ESM::Armor *armor = (mGreaves->get<ESM::Armor>())->mBase;
            std::vector<ESM::PartReference> parts = armor->mParts.mParts;
            addPartGroup(MWWorld::InventoryStore::Slot_Greaves, 3, parts);
        }

        if(mPauldronL != mInv.end())
        {
            const ESM::Armor *armor = (mPauldronL->get<ESM::Armor>())->mBase;
            std::vector<ESM::PartReference> parts = armor->mParts.mParts;
            addPartGroup(MWWorld::InventoryStore::Slot_LeftPauldron, 3, parts);
        }
        if(mPauldronR != mInv.end())
        {
            const ESM::Armor *armor = (mPauldronR->get<ESM::Armor>())->mBase;
            std::vector<ESM::PartReference> parts = armor->mParts.mParts;
            addPartGroup(MWWorld::InventoryStore::Slot_RightPauldron, 3, parts);
        }
        if(mBoots != mInv.end())
        {
            if(mBoots->getTypeName() == typeid(ESM::Clothing).name())
            {
                const ESM::Clothing *clothes = (mBoots->get<ESM::Clothing>())->mBase;
                std::vector<ESM::PartReference> parts = clothes->mParts.mParts;
                addPartGroup(MWWorld::InventoryStore::Slot_Boots, 2, parts);
            }
            else if(mBoots->getTypeName() == typeid(ESM::Armor).name())
            {
                const ESM::Armor *armor = (mBoots->get<ESM::Armor>())->mBase;
                std::vector<ESM::PartReference> parts = armor->mParts.mParts;
                addPartGroup(MWWorld::InventoryStore::Slot_Boots, 3, parts);
            }
        }
        if(mGloveL != mInv.end())
        {
            if(mGloveL->getTypeName() == typeid(ESM::Clothing).name())
            {
                const ESM::Clothing *clothes = (mGloveL->get<ESM::Clothing>())->mBase;
                std::vector<ESM::PartReference> parts = clothes->mParts.mParts;
                addPartGroup(MWWorld::InventoryStore::Slot_LeftGauntlet, 2, parts);
            }
            else
            {
                const ESM::Armor *armor = (mGloveL->get<ESM::Armor>())->mBase;
                std::vector<ESM::PartReference> parts = armor->mParts.mParts;
                addPartGroup(MWWorld::InventoryStore::Slot_LeftGauntlet, 3, parts);
            }
        }
        if(mGloveR != mInv.end())
        {
            if(mGloveR->getTypeName() == typeid(ESM::Clothing).name())
            {
                const ESM::Clothing *clothes = (mGloveR->get<ESM::Clothing>())->mBase;
                std::vector<ESM::PartReference> parts = clothes->mParts.mParts;
                addPartGroup(MWWorld::InventoryStore::Slot_RightGauntlet, 2, parts);
            }
            else
            {
                const ESM::Armor *armor = (mGloveR->get<ESM::Armor>())->mBase;
                std::vector<ESM::PartReference> parts = armor->mParts.mParts;
                addPartGroup(MWWorld::InventoryStore::Slot_RightGauntlet, 3, parts);
            }

        }

        if(mShirt != mInv.end())
        {
            const ESM::Clothing *clothes = (mShirt->get<ESM::Clothing>())->mBase;
            std::vector<ESM::PartReference> parts = clothes->mParts.mParts;
            addPartGroup(MWWorld::InventoryStore::Slot_Shirt, 2, parts);
        }
        if(mPants != mInv.end())
        {
            const ESM::Clothing *clothes = (mPants->get<ESM::Clothing>())->mBase;
            std::vector<ESM::PartReference> parts = clothes->mParts.mParts;
            addPartGroup(MWWorld::InventoryStore::Slot_Pants, 2, parts);
        }
    }

    if(mPartPriorities[ESM::PRT_Head] < 1)
        addOrReplaceIndividualPart(ESM::PRT_Head, -1,1, mHeadModel);
    if(mPartPriorities[ESM::PRT_Hair] < 1 && mPartPriorities[ESM::PRT_Head] <= 1)
        addOrReplaceIndividualPart(ESM::PRT_Hair, -1,1, mHairModel);

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

    const MWWorld::ESMStore &store = MWBase::Environment::get().getWorld()->getStore();
    for(size_t i = 0;i < sizeof(PartTypeList)/sizeof(PartTypeList[0]);i++)
    {
        if(mPartPriorities[PartTypeList[i].type] < 1)
        {
            const ESM::BodyPart *part = NULL;
            const MWWorld::Store<ESM::BodyPart> &partStore =
                store.get<ESM::BodyPart>();

            if (!mNpc->isMale()) {
                part = partStore.search(mBodyPrefix + "_f_" + PartTypeList[i].name[0]);
                if (part == 0) {
                    part = partStore.search(mBodyPrefix + "_f_" + PartTypeList[i].name[1]);
                }
            }
            if (part == 0) {
                part = partStore.search(mBodyPrefix + "_m_" + PartTypeList[i].name[0]);
            }
            if (part == 0) {
                part = partStore.search(mBodyPrefix + "_m_" + PartTypeList[i].name[1]);
            }

            if(part)
                addOrReplaceIndividualPart(PartTypeList[i].type, -1,1, "meshes\\"+part->mModel);
        }
    }
}

NifOgre::EntityList NpcAnimation::insertBoundedPart(const std::string &mesh, int group, const std::string &bonename)
{
    NifOgre::EntityList entities = NIFLoader::createEntities(mEntityList.mSkelBase, bonename,
                                                             mInsert, mesh);
    std::vector<Ogre::Entity*> &parts = entities.mEntities;
    for(size_t i = 0;i < parts.size();i++)
    {
        parts[i]->setVisibilityFlags(mVisibilityFlags);
        parts[i]->getUserObjectBindings ().setUserAny (Ogre::Any(group));
    }
    return entities;
}

void NpcAnimation::runAnimation(float timepassed)
{
    if(mTimeToChange > .2)
    {
        mTimeToChange = 0;
        updateParts();
    }
    mTimeToChange += timepassed;

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
        removeEntities(mHead);
    else if(type == ESM::PRT_Hair) //1
        removeEntities(mHair);
    else if(type == ESM::PRT_Neck) //2
        removeEntities(mNeck);
    else if(type == ESM::PRT_Cuirass)//3
        removeEntities(mChest);
    else if(type == ESM::PRT_Groin)//4
        removeEntities(mGroin);
    else if(type == ESM::PRT_Skirt)//5
        removeEntities(mSkirt);
    else if(type == ESM::PRT_RHand)//6
        removeEntities(mHandR);
    else if(type == ESM::PRT_LHand)//7
        removeEntities(mHandL);
    else if(type == ESM::PRT_RWrist)//8
        removeEntities(mWristR);
    else if(type == ESM::PRT_LWrist) //9
        removeEntities(mWristL);
    else if(type == ESM::PRT_Shield) //10
    {
    }
    else if(type == ESM::PRT_RForearm) //11
        removeEntities(mForearmR);
    else if(type == ESM::PRT_LForearm) //12
        removeEntities(mForearmL);
    else if(type == ESM::PRT_RUpperarm) //13
        removeEntities(mUpperArmR);
    else if(type == ESM::PRT_LUpperarm) //14
        removeEntities(mUpperArmL);
    else if(type == ESM::PRT_RFoot)                 //15
        removeEntities(mFootR);
    else if(type == ESM::PRT_LFoot)                //16
        removeEntities(mFootL);
    else if(type == ESM::PRT_RAnkle)    //17
        removeEntities(mAnkleR);
    else if(type == ESM::PRT_LAnkle)    //18
        removeEntities(mAnkleL);
    else if(type == ESM::PRT_RKnee)    //19
        removeEntities(mKneeR);
    else if(type == ESM::PRT_LKnee)    //20
        removeEntities(mKneeL);
    else if(type == ESM::PRT_RLeg)    //21
        removeEntities(mUpperLegR);
    else if(type == ESM::PRT_LLeg)    //22
        removeEntities(mUpperLegL);
    else if(type == ESM::PRT_RPauldron)    //23
        removeEntities(mClavicleR);
    else if(type == ESM::PRT_LPauldron)    //24
        removeEntities(mClavicleL);
    else if(type == ESM::PRT_Weapon)                 //25
    {
    }
    else if(type == ESM::PRT_Tail)    //26
        removeEntities(mTail);
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
            mHead = insertBoundedPart(mesh, group, "Head");
            break;
        case ESM::PRT_Hair:                          //1
            mHair = insertBoundedPart(mesh, group, "Head");
            break;
        case ESM::PRT_Neck:                          //2
            mNeck = insertBoundedPart(mesh, group, "Neck");
            break;
        case ESM::PRT_Cuirass:                          //3
            mChest = insertBoundedPart(mesh, group, "Chest");
            break;
        case ESM::PRT_Groin:                          //4
            mGroin = insertBoundedPart(mesh, group, "Groin");
            break;
        case ESM::PRT_Skirt:                          //5
            mSkirt = insertBoundedPart(mesh, group, "Groin");
            break;
        case ESM::PRT_RHand:                         //6
            mHandR = insertBoundedPart(mesh, group, "Right Hand");
            break;
        case ESM::PRT_LHand:                         //7
            mHandL = insertBoundedPart(mesh, group, "Left Hand");
            break;
        case ESM::PRT_RWrist:                          //8
            mWristR = insertBoundedPart(mesh, group, "Right Wrist");
            break;
        case ESM::PRT_LWrist:                          //9
            mWristL = insertBoundedPart(mesh, group, "Left Wrist");
            break;
        case ESM::PRT_Shield:                         //10
            break;
        case ESM::PRT_RForearm:                          //11
            mForearmR = insertBoundedPart(mesh, group, "Right Forearm");
            break;
        case ESM::PRT_LForearm:                          //12
            mForearmL = insertBoundedPart(mesh, group, "Left Forearm");
            break;
        case ESM::PRT_RUpperarm:                          //13
            mUpperArmR = insertBoundedPart(mesh, group, "Right Upper Arm");
            break;
        case ESM::PRT_LUpperarm:                          //14
            mUpperArmL = insertBoundedPart(mesh, group, "Left Upper Arm");
            break;
        case ESM::PRT_RFoot:                             //15
            mFootR = insertBoundedPart(mesh, group, "Right Foot");
            break;
        case ESM::PRT_LFoot:                             //16
            mFootL = insertBoundedPart(mesh, group, "Left Foot");
            break;
        case ESM::PRT_RAnkle:                          //17
            mAnkleR = insertBoundedPart(mesh, group, "Right Ankle");
            break;
        case ESM::PRT_LAnkle:                          //18
            mAnkleL = insertBoundedPart(mesh, group, "Left Ankle");
            break;
        case ESM::PRT_RKnee:                          //19
            mKneeR = insertBoundedPart(mesh, group, "Right Knee");
            break;
        case ESM::PRT_LKnee:                          //20
            mKneeL = insertBoundedPart(mesh, group, "Left Knee");
            break;
        case ESM::PRT_RLeg:                          //21
            mUpperLegR = insertBoundedPart(mesh, group, "Right Upper Leg");
            break;
        case ESM::PRT_LLeg:                          //22
            mUpperLegL = insertBoundedPart(mesh, group, "Left Upper Leg");
            break;
        case ESM::PRT_RPauldron:                          //23
            mClavicleR = insertBoundedPart(mesh , group, "Right Clavicle");
            break;
        case ESM::PRT_LPauldron:                          //24
            mClavicleL = insertBoundedPart(mesh, group, "Left Clavicle");
            break;
        case ESM::PRT_Weapon:                             //25
            break;
        case ESM::PRT_Tail:                              //26
            mTail = insertBoundedPart(mesh, group, "Tail");
            break;
    }
    return true;
}

void NpcAnimation::addPartGroup(int group, int priority, std::vector<ESM::PartReference> &parts)
{
    for(std::size_t i = 0; i < parts.size(); i++)
    {
        ESM::PartReference &part = parts[i];

        const MWWorld::Store<ESM::BodyPart> &partStore =
            MWBase::Environment::get().getWorld()->getStore().get<ESM::BodyPart>();

        const ESM::BodyPart *bodypart = 0;
        if(!mNpc->isMale())
            bodypart = partStore.search(part.mFemale);
        if(!bodypart)
            bodypart = partStore.search(part.mMale);

        if(bodypart)
            addOrReplaceIndividualPart(part.mPart, group, priority,"meshes\\" + bodypart->mModel);
        else
            reserveIndividualPart(part.mPart, group, priority);
    }
}

void NpcAnimation::forceUpdate ()
{
    updateParts();
}

}

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

namespace MWRender
{

const NpcAnimation::PartInfo NpcAnimation::sPartList[NpcAnimation::sPartListSize] = {
    { ESM::PRT_Head, &NpcAnimation::mHead, "Head" },
    { ESM::PRT_Hair, &NpcAnimation::mHair, "Head" },
    { ESM::PRT_Neck, &NpcAnimation::mNeck, "Neck" },
    { ESM::PRT_Cuirass, &NpcAnimation::mChest, "Chest" },
    { ESM::PRT_Groin, &NpcAnimation::mGroin, "Groin" },
    { ESM::PRT_Skirt, &NpcAnimation::mSkirt, "Groin" },
    { ESM::PRT_RHand, &NpcAnimation::mHandR, "Right Hand" },
    { ESM::PRT_LHand, &NpcAnimation::mHandL, "Left Hand" },
    { ESM::PRT_RWrist, &NpcAnimation::mWristR, "Right Wrist" },
    { ESM::PRT_LWrist, &NpcAnimation::mWristL, "Left Wrist" },
    { ESM::PRT_Shield, &NpcAnimation::mShield, "Shield" },
    { ESM::PRT_RForearm, &NpcAnimation::mForearmR, "Right Forearm" },
    { ESM::PRT_LForearm, &NpcAnimation::mForearmL, "Left Forearm" },
    { ESM::PRT_RUpperarm, &NpcAnimation::mUpperArmR, "Right Upper Arm" },
    { ESM::PRT_LUpperarm, &NpcAnimation::mUpperArmL, "Left Upper Arm" },
    { ESM::PRT_RFoot, &NpcAnimation::mFootR, "Right Foot" },
    { ESM::PRT_LFoot, &NpcAnimation::mFootL, "Left Foot" },
    { ESM::PRT_RAnkle, &NpcAnimation::mAnkleR, "Right Ankle" },
    { ESM::PRT_LAnkle, &NpcAnimation::mAnkleL, "Left Ankle" },
    { ESM::PRT_RKnee, &NpcAnimation::mKneeR, "Right Knee" },
    { ESM::PRT_LKnee, &NpcAnimation::mKneeL, "Left Knee" },
    { ESM::PRT_RLeg, &NpcAnimation::mUpperLegR, "Right Upper Leg" },
    { ESM::PRT_LLeg, &NpcAnimation::mUpperLegL, "Left Upper Leg" },
    { ESM::PRT_RPauldron, &NpcAnimation::mClavicleR, "Right Clavicle" },
    { ESM::PRT_LPauldron, &NpcAnimation::mClavicleL, "Left Clavicle" },
    { ESM::PRT_Weapon, &NpcAnimation::mWeapon, "Weapon" },
    { ESM::PRT_Tail, &NpcAnimation::mTail, "Tail" }
};

NpcAnimation::~NpcAnimation()
{
    for(size_t i = 0;i < sPartListSize;i++)
        removeEntities(this->*sPartList[i].ents);
}


NpcAnimation::NpcAnimation(const MWWorld::Ptr& ptr, Ogre::SceneNode* node, MWWorld::InventoryStore& inv, int visibilityFlags)
  : Animation(),
    mInv(inv),
    mStateID(-1),
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

    for(size_t i = 0;i < sPartListSize;i++)
    {
        mPartslots[i] = -1;  //each slot is empty
        mPartPriorities[i] = 0;
    }

    const MWWorld::ESMStore &store =
        MWBase::Environment::get().getWorld()->getStore();
    const ESM::Race *race = store.get<ESM::Race>().find(mNpc->mRace);

    mHeadModel = "meshes\\" + store.get<ESM::BodyPart>().find(mNpc->mHead)->mModel;
    mHairModel = "meshes\\" + store.get<ESM::BodyPart>().find(mNpc->mHair)->mModel;

    mBodyPrefix = "b_n_" + mNpc->mRace;
    std::transform(mBodyPrefix.begin(), mBodyPrefix.end(), mBodyPrefix.begin(), ::tolower);

    bool isBeast = (race->mData.mFlags & ESM::Race::Beast) != 0;
    std::string smodel = (!isBeast ? "meshes\\base_anim.nif" : "meshes\\base_animkna.nif");

    createEntityList(node, smodel);
    for(size_t i = 0;i < mEntityList.mEntities.size();i++)
    {
        Ogre::Entity *base = mEntityList.mEntities[i];

        base->getUserObjectBindings().setUserAny(Ogre::Any(-1));
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

    static const struct {
        MWWorld::ContainerStoreIterator NpcAnimation::*iter;
        int slot;
    } slotlist[] = {
        { &NpcAnimation::mRobe, MWWorld::InventoryStore::Slot_Robe },
        { &NpcAnimation::mSkirtIter, MWWorld::InventoryStore::Slot_Skirt },
        { &NpcAnimation::mHelmet, MWWorld::InventoryStore::Slot_Helmet },
        { &NpcAnimation::mCuirass, MWWorld::InventoryStore::Slot_Cuirass },
        { &NpcAnimation::mGreaves, MWWorld::InventoryStore::Slot_Greaves },
        { &NpcAnimation::mPauldronL, MWWorld::InventoryStore::Slot_LeftPauldron },
        { &NpcAnimation::mPauldronR, MWWorld::InventoryStore::Slot_RightPauldron },
        { &NpcAnimation::mBoots, MWWorld::InventoryStore::Slot_Boots },
        { &NpcAnimation::mGloveL, MWWorld::InventoryStore::Slot_LeftGauntlet },
        { &NpcAnimation::mGloveR, MWWorld::InventoryStore::Slot_RightGauntlet },
        { &NpcAnimation::mShirt, MWWorld::InventoryStore::Slot_Shirt },
        { &NpcAnimation::mPants, MWWorld::InventoryStore::Slot_Pants },
    };
    for(size_t i = 0;i < sizeof(slotlist)/sizeof(slotlist[0]);i++)
    {
        MWWorld::ContainerStoreIterator iter = mInv.getSlot(slotlist[i].slot);
        if(this->*slotlist[i].iter != iter)
        {
            this->*slotlist[i].iter = iter;
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
            const MWWorld::Store<ESM::BodyPart> &partStore = store.get<ESM::BodyPart>();

            if(!mNpc->isMale())
            {
                part = partStore.search(mBodyPrefix + "_f_" + PartTypeList[i].name[0]);
                if(part == 0)
                    part = partStore.search(mBodyPrefix + "_f_" + PartTypeList[i].name[1]);
            }
            if(part == 0)
                part = partStore.search(mBodyPrefix + "_m_" + PartTypeList[i].name[0]);
            if(part == 0)
                part = partStore.search(mBodyPrefix + "_m_" + PartTypeList[i].name[1]);

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
        parts[i]->getUserObjectBindings().setUserAny(Ogre::Any(group));
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

    for(size_t i = 0;i < sPartListSize;i++)
    {
        if(type == sPartList[i].type)
        {
            removeEntities(this->*sPartList[i].ents);
            break;
        }
    }
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

    for(size_t i = 0;i < sPartListSize;i++)
    {
        if(type == sPartList[i].type)
        {
            this->*sPartList[i].ents = insertBoundedPart(mesh, group, sPartList[i].name);
            break;
        }
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

#include "npcanimation.hpp"

#include <OgreSceneManager.h>
#include <OgreEntity.h>
#include <OgreParticleSystem.h>
#include <OgreSubEntity.h>

#include "../mwworld/esmstore.hpp"
#include "../mwworld/inventorystore.hpp"
#include "../mwworld/class.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/mechanicsmanager.hpp"

#include "renderconst.hpp"


namespace MWRender
{

const NpcAnimation::PartInfo NpcAnimation::sPartList[NpcAnimation::sPartListSize] = {
    { ESM::PRT_Head, "Head" },
    { ESM::PRT_Hair, "Head" },
    { ESM::PRT_Neck, "Neck" },
    { ESM::PRT_Cuirass, "Chest" },
    { ESM::PRT_Groin, "Groin" },
    { ESM::PRT_Skirt, "Groin" },
    { ESM::PRT_RHand, "Right Hand" },
    { ESM::PRT_LHand, "Left Hand" },
    { ESM::PRT_RWrist, "Right Wrist" },
    { ESM::PRT_LWrist, "Left Wrist" },
    { ESM::PRT_Shield, "Shield" },
    { ESM::PRT_RForearm, "Right Forearm" },
    { ESM::PRT_LForearm, "Left Forearm" },
    { ESM::PRT_RUpperarm, "Right Upper Arm" },
    { ESM::PRT_LUpperarm, "Left Upper Arm" },
    { ESM::PRT_RFoot, "Right Foot" },
    { ESM::PRT_LFoot, "Left Foot" },
    { ESM::PRT_RAnkle, "Right Ankle" },
    { ESM::PRT_LAnkle, "Left Ankle" },
    { ESM::PRT_RKnee, "Right Knee" },
    { ESM::PRT_LKnee, "Left Knee" },
    { ESM::PRT_RLeg, "Right Upper Leg" },
    { ESM::PRT_LLeg, "Left Upper Leg" },
    { ESM::PRT_RPauldron, "Right Clavicle" },
    { ESM::PRT_LPauldron, "Left Clavicle" },
    { ESM::PRT_Weapon, "Weapon" },
    { ESM::PRT_Tail, "Tail" }
};

NpcAnimation::~NpcAnimation()
{
    Ogre::SceneManager *sceneMgr = mInsert->getCreator();
    for(size_t i = 0;i < sPartListSize;i++)
        destroyObjectList(sceneMgr, mObjectParts[i]);
}


NpcAnimation::NpcAnimation(const MWWorld::Ptr& ptr, Ogre::SceneNode* node, MWWorld::InventoryStore& inv, int visibilityFlags, ViewMode viewMode)
  : Animation(ptr),
    mStateID(-1),
    mTimeToChange(0),
    mVisibilityFlags(visibilityFlags),
    mRobe(inv.end()),
    mHelmet(inv.end()),
    mShirt(inv.end()),
    mCuirass(inv.end()),
    mGreaves(inv.end()),
    mPauldronL(inv.end()),
    mPauldronR(inv.end()),
    mBoots(inv.end()),
    mPants(inv.end()),
    mGloveL(inv.end()),
    mGloveR(inv.end()),
    mSkirtIter(inv.end()),
    mViewMode(viewMode)
{
    mNpc = mPtr.get<ESM::NPC>()->mBase;

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
    Misc::StringUtils::toLower(mBodyPrefix);

    bool isBeast = (race->mData.mFlags & ESM::Race::Beast) != 0;
    std::string smodel = (!isBeast ? "meshes\\base_anim.nif" : "meshes\\base_animkna.nif");

    addObjectList(node, smodel, true);
    if(mBodyPrefix.find("argonian") != std::string::npos)
        addObjectList(node, "meshes\\argonian_swimkna.nif", true);
    else if(!mNpc->isMale() && !isBeast)
        addObjectList(node, "meshes\\base_anim_female.nif", true);
    if(mNpc->mModel.length() > 0)
        addObjectList(node, "meshes\\"+mNpc->mModel, true);
    if(mViewMode == VM_FirstPerson)
    {
        /* A bit counter-intuitive, but unlike third-person anims, it seems
         * beast races get both base_anim.1st.nif and base_animkna.1st.nif.
         */
        addObjectList(node, "meshes\\base_anim.1st.nif", true);
        if(isBeast)
            addObjectList(node, "meshes\\base_animkna.1st.nif", true);
        if(!mNpc->isMale() && !isBeast)
            addObjectList(node, "meshes\\base_anim_female.1st.nif", true);
    }

    forceUpdate();
}

void NpcAnimation::setViewMode(NpcAnimation::ViewMode viewMode)
{
    assert(viewMode != VM_HeadOnly);
    mViewMode = viewMode;

    Ogre::SceneNode *node = mInsert->getParentSceneNode();

    const MWWorld::ESMStore &store = MWBase::Environment::get().getWorld()->getStore();
    const ESM::Race *race = store.get<ESM::Race>().find(mNpc->mRace);
    bool isBeast = (race->mData.mFlags & ESM::Race::Beast) != 0;

    clearExtraSources();
    if(mBodyPrefix.find("argonian") != std::string::npos)
        addObjectList(node, "meshes\\argonian_swimkna.nif", true);
    else if(!mNpc->isMale() && !isBeast)
        addObjectList(node, "meshes\\base_anim_female.nif", true);
    if(mNpc->mModel.length() > 0)
        addObjectList(node, "meshes\\"+mNpc->mModel, true);
    if(mViewMode == VM_FirstPerson)
    {
        /* A bit counter-intuitive, but unlike third-person anims, it seems
         * beast races get both base_anim.1st.nif and base_animkna.1st.nif.
         */
        addObjectList(node, "meshes\\base_anim.1st.nif", true);
        if(isBeast)
            addObjectList(node, "meshes\\base_animkna.1st.nif", true);
        if(!mNpc->isMale() && !isBeast)
            addObjectList(node, "meshes\\base_anim_female.1st.nif", true);
    }
    MWBase::Environment::get().getMechanicsManager()->forceStateUpdate(mPtr);

    for(size_t i = 0;i < sPartListSize;i++)
        removeIndividualPart(i);
    forceUpdate();
}

void NpcAnimation::updateParts(bool forceupdate)
{
    static const struct {
        MWWorld::ContainerStoreIterator NpcAnimation::*mPart;
        int mSlot;
        int mBasePriority;
    } slotlist[] = {
        // FIXME: Priority is based on the number of reserved slots. There should be a better way.
        { &NpcAnimation::mRobe,      MWWorld::InventoryStore::Slot_Robe,         12 },
        { &NpcAnimation::mSkirtIter, MWWorld::InventoryStore::Slot_Skirt,         3 },
        { &NpcAnimation::mHelmet,    MWWorld::InventoryStore::Slot_Helmet,        0 },
        { &NpcAnimation::mCuirass,   MWWorld::InventoryStore::Slot_Cuirass,       0 },
        { &NpcAnimation::mGreaves,   MWWorld::InventoryStore::Slot_Greaves,       0 },
        { &NpcAnimation::mPauldronL, MWWorld::InventoryStore::Slot_LeftPauldron,  0 },
        { &NpcAnimation::mPauldronR, MWWorld::InventoryStore::Slot_RightPauldron, 0 },
        { &NpcAnimation::mBoots,     MWWorld::InventoryStore::Slot_Boots,         0 },
        { &NpcAnimation::mGloveL,    MWWorld::InventoryStore::Slot_LeftGauntlet,  0 },
        { &NpcAnimation::mGloveR,    MWWorld::InventoryStore::Slot_RightGauntlet, 0 },
        { &NpcAnimation::mShirt,     MWWorld::InventoryStore::Slot_Shirt,         0 },
        { &NpcAnimation::mPants,     MWWorld::InventoryStore::Slot_Pants,         0 },
    };
    static const size_t slotlistsize = sizeof(slotlist)/sizeof(slotlist[0]);

    MWWorld::InventoryStore &inv = MWWorld::Class::get(mPtr).getInventoryStore(mPtr);
    for(size_t i = 0;!forceupdate && i < slotlistsize;i++)
    {
        MWWorld::ContainerStoreIterator iter = inv.getSlot(slotlist[i].mSlot);
        if(this->*slotlist[i].mPart != iter)
        {
            forceupdate = true;
            break;
        }
    }
    if(!forceupdate)
        return;

    /* FIXME: Remove this once we figure out how to show what in first-person */
    if(mViewMode == VM_FirstPerson)
    {
        for(size_t i = 0;i < slotlistsize;i++)
            this->*slotlist[i].mPart = inv.getSlot(slotlist[i].mSlot);
        return;
    }

    for(size_t i = 0;i < slotlistsize && mViewMode != VM_HeadOnly;i++)
    {
        MWWorld::ContainerStoreIterator iter = inv.getSlot(slotlist[i].mSlot);

        this->*slotlist[i].mPart = iter;
        removePartGroup(slotlist[i].mSlot);

        if(this->*slotlist[i].mPart == inv.end())
            continue;

        if(slotlist[i].mSlot == MWWorld::InventoryStore::Slot_Helmet)
            removeIndividualPart(ESM::PRT_Hair);

        int prio = 1;
        MWWorld::ContainerStoreIterator &store = this->*slotlist[i].mPart;
        if(store->getTypeName() == typeid(ESM::Clothing).name())
        {
            prio = ((slotlist[i].mBasePriority+1)<<1) + 0;
            const ESM::Clothing *clothes = store->get<ESM::Clothing>()->mBase;
            addPartGroup(slotlist[i].mSlot, prio, clothes->mParts.mParts);
        }
        else if(store->getTypeName() == typeid(ESM::Armor).name())
        {
            prio = ((slotlist[i].mBasePriority+1)<<1) + 1;
            const ESM::Armor *armor = store->get<ESM::Armor>()->mBase;
            addPartGroup(slotlist[i].mSlot, prio, armor->mParts.mParts);
        }

        if(slotlist[i].mSlot == MWWorld::InventoryStore::Slot_Robe)
        {
            ESM::PartReferenceType parts[] = {
                ESM::PRT_Groin, ESM::PRT_Skirt, ESM::PRT_RLeg, ESM::PRT_LLeg,
                ESM::PRT_RUpperarm, ESM::PRT_LUpperarm, ESM::PRT_RKnee, ESM::PRT_LKnee,
                ESM::PRT_RForearm, ESM::PRT_LForearm, ESM::PRT_RPauldron, ESM::PRT_LPauldron
            };
            size_t parts_size = sizeof(parts)/sizeof(parts[0]);
            for(int p = 0;p < parts_size;p++)
                reserveIndividualPart(parts[p], slotlist[i].mSlot, prio);
        }
        else if(slotlist[i].mSlot == MWWorld::InventoryStore::Slot_Skirt)
        {
            reserveIndividualPart(ESM::PRT_Groin, slotlist[i].mSlot, prio);
            reserveIndividualPart(ESM::PRT_RLeg, slotlist[i].mSlot, prio);
            reserveIndividualPart(ESM::PRT_LLeg, slotlist[i].mSlot, prio);
        }
    }

    if(mViewMode != VM_FirstPerson)
    {
        if(mPartPriorities[ESM::PRT_Head] < 1)
            addOrReplaceIndividualPart(ESM::PRT_Head, -1,1, mHeadModel);
        if(mPartPriorities[ESM::PRT_Hair] < 1 && mPartPriorities[ESM::PRT_Head] <= 1)
            addOrReplaceIndividualPart(ESM::PRT_Hair, -1,1, mHairModel);
    }
    if(mViewMode == VM_HeadOnly)
        return;

    std::map<int, int> bodypartMap;
    bodypartMap[ESM::PRT_Neck] = ESM::BodyPart::MP_Neck;
    bodypartMap[ESM::PRT_Cuirass] = ESM::BodyPart::MP_Chest;
    bodypartMap[ESM::PRT_Groin] = ESM::BodyPart::MP_Groin;
    bodypartMap[ESM::PRT_RHand] = ESM::BodyPart::MP_Hand;
    bodypartMap[ESM::PRT_LHand] = ESM::BodyPart::MP_Hand;
    bodypartMap[ESM::PRT_RWrist] = ESM::BodyPart::MP_Wrist;
    bodypartMap[ESM::PRT_LWrist] = ESM::BodyPart::MP_Wrist;
    bodypartMap[ESM::PRT_RForearm] = ESM::BodyPart::MP_Forearm;
    bodypartMap[ESM::PRT_LForearm] = ESM::BodyPart::MP_Forearm;
    bodypartMap[ESM::PRT_RUpperarm] = ESM::BodyPart::MP_Upperarm;
    bodypartMap[ESM::PRT_LUpperarm] = ESM::BodyPart::MP_Upperarm;
    bodypartMap[ESM::PRT_RFoot] = ESM::BodyPart::MP_Foot;
    bodypartMap[ESM::PRT_LFoot] = ESM::BodyPart::MP_Foot;
    bodypartMap[ESM::PRT_RAnkle] = ESM::BodyPart::MP_Ankle;
    bodypartMap[ESM::PRT_LAnkle] = ESM::BodyPart::MP_Ankle;
    bodypartMap[ESM::PRT_RKnee] = ESM::BodyPart::MP_Knee;
    bodypartMap[ESM::PRT_LKnee] = ESM::BodyPart::MP_Knee;
    bodypartMap[ESM::PRT_RLeg] = ESM::BodyPart::MP_Upperleg;
    bodypartMap[ESM::PRT_LLeg] = ESM::BodyPart::MP_Upperleg;
    bodypartMap[ESM::PRT_Tail] = ESM::BodyPart::MP_Tail;

    const MWWorld::ESMStore &store = MWBase::Environment::get().getWorld()->getStore();

    const int Flag_Female = 0x01;
    const int Flag_FirstPerson = 0x02;

    int flags = 0;
    if (!mNpc->isMale())
        flags |= Flag_Female;
    if (mViewMode == VM_FirstPerson)
        flags |= Flag_FirstPerson;

    // Remember body parts so we only have to search through the store once for each race/gender/viewmode combination
    static std::map< std::pair<std::string, int> , std::vector<const ESM::BodyPart*> > sRaceMapping;
    std::string race = Misc::StringUtils::lowerCase(mNpc->mRace);
    std::pair<std::string, int> thisCombination = std::make_pair(race, flags);
    if (sRaceMapping.find(thisCombination) == sRaceMapping.end())
    {
        sRaceMapping[thisCombination].resize(ESM::PRT_Count);
        for (int i=0; i<ESM::PRT_Count; ++i)
            sRaceMapping[thisCombination][i] = NULL;

        const MWWorld::Store<ESM::BodyPart> &partStore = store.get<ESM::BodyPart>();

        for (MWWorld::Store<ESM::BodyPart>::iterator it = partStore.begin(); it != partStore.end(); ++it)
        {
            const ESM::BodyPart& bodypart = *it;
            if (bodypart.mData.mFlags & ESM::BodyPart::BPF_NotPlayable)
                continue;
            if (bodypart.mData.mType != ESM::BodyPart::MT_Skin)
            {
                continue;
            }
            if (!mNpc->isMale() != (bodypart.mData.mFlags & ESM::BodyPart::BPF_Female))
                continue;
            if (!Misc::StringUtils::ciEqual(bodypart.mRace, mNpc->mRace))
                continue;

            bool firstPerson = (bodypart.mId.size() >= 3)
                    && bodypart.mId[bodypart.mId.size()-3] == '1'
                    && bodypart.mId[bodypart.mId.size()-2] == 's'
                    && bodypart.mId[bodypart.mId.size()-1] == 't';
            if (firstPerson != (mViewMode == VM_FirstPerson))
                continue;
            for (std::map<int, int>::iterator bIt = bodypartMap.begin(); bIt != bodypartMap.end(); ++bIt )
                if (bIt->second == bodypart.mData.mPart)
                    sRaceMapping[thisCombination][bIt->first] = &*it;
        }
    }

    for (int part = ESM::PRT_Neck; part < ESM::PRT_Count; ++part)
    {
        const ESM::BodyPart* bodypart = sRaceMapping[thisCombination][part];
        if (mPartPriorities[part] < 1 && bodypart)
            addOrReplaceIndividualPart(part, -1,1, "meshes\\"+bodypart->mModel);
    }
}

NifOgre::ObjectList NpcAnimation::insertBoundedPart(const std::string &model, int group, const std::string &bonename)
{
    NifOgre::ObjectList objects = NifOgre::Loader::createObjects(mSkelBase, bonename, mInsert, model);
    setRenderProperties(objects, mVisibilityFlags, RQG_Main, RQG_Alpha);

    for(size_t i = 0;i < objects.mEntities.size();i++)
        objects.mEntities[i]->getUserObjectBindings().setUserAny(Ogre::Any(group));
    for(size_t i = 0;i < objects.mParticles.size();i++)
        objects.mParticles[i]->getUserObjectBindings().setUserAny(Ogre::Any(group));

    if(objects.mSkelBase)
    {
        Ogre::AnimationStateSet *aset = objects.mSkelBase->getAllAnimationStates();
        Ogre::AnimationStateIterator asiter = aset->getAnimationStateIterator();
        while(asiter.hasMoreElements())
        {
            Ogre::AnimationState *state = asiter.getNext();
            state->setEnabled(false);
            state->setLoop(false);
        }
        Ogre::SkeletonInstance *skelinst = objects.mSkelBase->getSkeleton();
        Ogre::Skeleton::BoneIterator boneiter = skelinst->getBoneIterator();
        while(boneiter.hasMoreElements())
            boneiter.getNext()->setManuallyControlled(true);
    }

    return objects;
}

Ogre::Vector3 NpcAnimation::runAnimation(float timepassed)
{
    if(mTimeToChange <= 0.0f)
    {
        mTimeToChange = 0.2f;
        updateParts();
    }
    mTimeToChange -= timepassed;

    Ogre::Vector3 ret = Animation::runAnimation(timepassed);

    Ogre::SkeletonInstance *baseinst = mSkelBase->getSkeleton();
    for(size_t i = 0;i < sPartListSize;i++)
    {
        Ogre::Entity *ent = mObjectParts[i].mSkelBase;
        if(!ent) continue;
        updateSkeletonInstance(baseinst, ent->getSkeleton());
        ent->getAllAnimationStates()->_notifyDirty();
    }

    return ret;
}

void NpcAnimation::removeIndividualPart(int type)
{
    mPartPriorities[type] = 0;
    mPartslots[type] = -1;

    for(size_t i = 0;i < sPartListSize;i++)
    {
        if(type == sPartList[i].type)
        {
            destroyObjectList(mInsert->getCreator(), mObjectParts[i]);
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
            mObjectParts[i] = insertBoundedPart(mesh, group, sPartList[i].name);
            break;
        }
    }
    return true;
}

void NpcAnimation::addPartGroup(int group, int priority, const std::vector<ESM::PartReference> &parts)
{
    const char *ext = (mViewMode == VM_FirstPerson) ? ".1st" : "";
    for(std::size_t i = 0; i < parts.size(); i++)
    {
        const ESM::PartReference &part = parts[i];

        const MWWorld::Store<ESM::BodyPart> &partStore =
            MWBase::Environment::get().getWorld()->getStore().get<ESM::BodyPart>();

        const ESM::BodyPart *bodypart = 0;
        if(!mNpc->isMale())
            bodypart = partStore.search(part.mFemale+ext);
        if(!bodypart)
            bodypart = partStore.search(part.mMale+ext);

        if(bodypart)
            addOrReplaceIndividualPart(part.mPart, group, priority, "meshes\\"+bodypart->mModel);
        else
            reserveIndividualPart(part.mPart, group, priority);
    }
}

}

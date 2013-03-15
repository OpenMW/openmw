#include "npcanimation.hpp"

#include <OgreSceneManager.h>
#include <OgreEntity.h>
#include <OgreSubEntity.h>

#include "../mwworld/esmstore.hpp"
#include "../mwworld/inventorystore.hpp"
#include "../mwworld/class.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"

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
    for(size_t i = 0;i < sPartListSize;i++)
        removeEntities(mEntityParts[i]);
}


NpcAnimation::NpcAnimation(const MWWorld::Ptr& ptr, Ogre::SceneNode* node, MWWorld::InventoryStore& inv, int visibilityFlags, bool headOnly)
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
    mHeadOnly(headOnly)
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

    createEntityList(node, smodel);
    for(size_t i = 0;i < mEntityList.mEntities.size();i++)
    {
        Ogre::Entity *base = mEntityList.mEntities[i];

        base->getUserObjectBindings().setUserAny(Ogre::Any(-1));
        if (mVisibilityFlags != 0)
            base->setVisibilityFlags(mVisibilityFlags);

        for(unsigned int j=0; j < base->getNumSubEntities(); ++j)
        {
            Ogre::SubEntity* subEnt = base->getSubEntity(j);
            subEnt->setRenderQueueGroup(subEnt->getMaterial()->isTransparent() ? RQG_Alpha : RQG_Main);
        }
    }

    std::vector<std::string> skelnames(1, smodel);
    if(!mNpc->isMale() && !isBeast)
        skelnames.push_back("meshes\\base_anim_female.nif");
    else if(mBodyPrefix.find("argonian") != std::string::npos)
        skelnames.push_back("meshes\\argonian_swimkna.nif");
    if(mNpc->mModel.length() > 0)
        skelnames.push_back("meshes\\"+Misc::StringUtils::lowerCase(mNpc->mModel));
    setAnimationSources(skelnames);

    updateParts(true);
}

void NpcAnimation::updateParts(bool forceupdate)
{
    static const struct {
        int numRemoveParts; // Max: 1
        ESM::PartReferenceType removeParts[1];

        MWWorld::ContainerStoreIterator NpcAnimation::*part;
        int slot;

        int numReserveParts; // Max: 12
        ESM::PartReferenceType reserveParts[12];
    } slotlist[] = {
        { 0, { },
          &NpcAnimation::mRobe, MWWorld::InventoryStore::Slot_Robe,
          12, { ESM::PRT_Groin, ESM::PRT_Skirt, ESM::PRT_RLeg, ESM::PRT_LLeg,
                ESM::PRT_RUpperarm, ESM::PRT_LUpperarm, ESM::PRT_RKnee, ESM::PRT_LKnee,
                ESM::PRT_RForearm, ESM::PRT_LForearm, ESM::PRT_RPauldron, ESM::PRT_LPauldron }
        },

        { 0, { },
          &NpcAnimation::mSkirtIter, MWWorld::InventoryStore::Slot_Skirt,
          3, { ESM::PRT_Groin, ESM::PRT_RLeg, ESM::PRT_LLeg }
        },

        { 1, { ESM::PRT_Hair },
          &NpcAnimation::mHelmet, MWWorld::InventoryStore::Slot_Helmet,
          0, { }
        },

        { 0, { },
          &NpcAnimation::mCuirass, MWWorld::InventoryStore::Slot_Cuirass,
          0, { }
        },

        { 0, { },
          &NpcAnimation::mGreaves, MWWorld::InventoryStore::Slot_Greaves,
          0, { }
        },

        { 0, { },
          &NpcAnimation::mPauldronL, MWWorld::InventoryStore::Slot_LeftPauldron,
          0, { }
        },

        { 0, { },
          &NpcAnimation::mPauldronR, MWWorld::InventoryStore::Slot_RightPauldron,
          0, { }
        },

        { 0, { },
          &NpcAnimation::mBoots, MWWorld::InventoryStore::Slot_Boots,
          0, { }
        },

        { 0, { },
          &NpcAnimation::mGloveL, MWWorld::InventoryStore::Slot_LeftGauntlet,
          0, { }
        },

        { 0, { },
          &NpcAnimation::mGloveR, MWWorld::InventoryStore::Slot_RightGauntlet,
          0, { }
        },

        { 0, { },
          &NpcAnimation::mShirt, MWWorld::InventoryStore::Slot_Shirt,
          0, { }
        },

        { 0, { },
          &NpcAnimation::mPants, MWWorld::InventoryStore::Slot_Pants,
          0, { }
        },
    };
    static const size_t slotlistsize = sizeof(slotlist)/sizeof(slotlist[0]);

    MWWorld::InventoryStore &inv = MWWorld::Class::get(mPtr).getInventoryStore(mPtr);
    for(size_t i = 0;!forceupdate && i < slotlistsize;i++)
    {
        MWWorld::ContainerStoreIterator iter = inv.getSlot(slotlist[i].slot);
        if(this->*slotlist[i].part != iter)
        {
            forceupdate = true;
            break;
        }
    }
    if(!forceupdate)
        return;

    for(size_t i = 0;i < slotlistsize && !mHeadOnly;i++)
    {
        MWWorld::ContainerStoreIterator iter = inv.getSlot(slotlist[i].slot);

        this->*slotlist[i].part = iter;
        removePartGroup(slotlist[i].slot);

        if(this->*slotlist[i].part == inv.end())
            continue;

        for(int rem = 0;rem < slotlist[i].numRemoveParts;rem++)
            removeIndividualPart(slotlist[i].removeParts[rem]);

        int prio = 1;
        MWWorld::ContainerStoreIterator &store = this->*slotlist[i].part;
        if(store->getTypeName() == typeid(ESM::Clothing).name())
        {
            prio = ((slotlist[i].numReserveParts+1)<<1) + 0;
            const ESM::Clothing *clothes = store->get<ESM::Clothing>()->mBase;
            addPartGroup(slotlist[i].slot, prio, clothes->mParts.mParts);
        }
        else if(store->getTypeName() == typeid(ESM::Armor).name())
        {
            prio = ((slotlist[i].numReserveParts+1)<<1) + 1;
            const ESM::Armor *armor = store->get<ESM::Armor>()->mBase;
            addPartGroup(slotlist[i].slot, prio, armor->mParts.mParts);
        }

        for(int res = 0;res < slotlist[i].numReserveParts;res++)
            reserveIndividualPart(slotlist[i].reserveParts[res], slotlist[i].slot, prio);
    }

    if(mPartPriorities[ESM::PRT_Head] < 1)
        addOrReplaceIndividualPart(ESM::PRT_Head, -1,1, mHeadModel);
    if(mPartPriorities[ESM::PRT_Hair] < 1 && mPartPriorities[ESM::PRT_Head] <= 1)
        addOrReplaceIndividualPart(ESM::PRT_Hair, -1,1, mHairModel);

    if (mHeadOnly)
        return;

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
    NifOgre::EntityList entities = NifOgre::Loader::createEntities(mEntityList.mSkelBase, bonename,
                                                                   mInsert, mesh);
    std::vector<Ogre::Entity*> &parts = entities.mEntities;
    for(size_t i = 0;i < parts.size();i++)
    {
        parts[i]->getUserObjectBindings().setUserAny(Ogre::Any(group));
        if (mVisibilityFlags != 0)
            parts[i]->setVisibilityFlags(mVisibilityFlags);

        for(unsigned int j=0; j < parts[i]->getNumSubEntities(); ++j)
        {
            Ogre::SubEntity* subEnt = parts[i]->getSubEntity(j);
            subEnt->setRenderQueueGroup(subEnt->getMaterial()->isTransparent() ? RQG_Alpha : RQG_Main);
        }
    }
    if(entities.mSkelBase)
    {
        Ogre::AnimationStateSet *aset = entities.mSkelBase->getAllAnimationStates();
        Ogre::AnimationStateIterator asiter = aset->getAnimationStateIterator();
        while(asiter.hasMoreElements())
        {
            Ogre::AnimationState *state = asiter.getNext();
            state->setEnabled(false);
            state->setLoop(false);
        }
        Ogre::SkeletonInstance *skelinst = entities.mSkelBase->getSkeleton();
        Ogre::Skeleton::BoneIterator boneiter = skelinst->getBoneIterator();
        while(boneiter.hasMoreElements())
            boneiter.getNext()->setManuallyControlled(true);
    }
    return entities;
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
    const Ogre::SkeletonInstance *skelsrc = mEntityList.mSkelBase->getSkeleton();
    for(size_t i = 0;i < sPartListSize;i++)
    {
        Ogre::Entity *ent = mEntityParts[i].mSkelBase;
        if(!ent) continue;
        updateSkeletonInstance(skelsrc, ent->getSkeleton());
        ent->getAllAnimationStates()->_notifyDirty();
    }
    return ret;
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
            removeEntities(mEntityParts[i]);
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
            mEntityParts[i] = insertBoundedPart(mesh, group, sPartList[i].name);
            break;
        }
    }
    return true;
}

void NpcAnimation::addPartGroup(int group, int priority, const std::vector<ESM::PartReference> &parts)
{
    for(std::size_t i = 0; i < parts.size(); i++)
    {
        const ESM::PartReference &part = parts[i];

        const MWWorld::Store<ESM::BodyPart> &partStore =
            MWBase::Environment::get().getWorld()->getStore().get<ESM::BodyPart>();

        const ESM::BodyPart *bodypart = 0;
        if(!mNpc->isMale())
            bodypart = partStore.search(part.mFemale);
        if(!bodypart)
            bodypart = partStore.search(part.mMale);

        if(bodypart)
            addOrReplaceIndividualPart(part.mPart, group, priority, "meshes\\"+bodypart->mModel);
        else
            reserveIndividualPart(part.mPart, group, priority);
    }
}

Ogre::Node* NpcAnimation::getHeadNode()
{
    return mEntityList.mSkelBase->getSkeleton()->getBone("Bip01 Head");
}

}

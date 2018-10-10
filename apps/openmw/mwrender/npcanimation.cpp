#include "npcanimation.hpp"

#include <osg/UserDataContainer>
#include <osg/MatrixTransform>
#include <osg/Depth>

#include <osgUtil/RenderBin>
#include <osgUtil/CullVisitor>

#include <components/debug/debuglog.hpp>

#include <components/misc/rng.hpp>

#include <components/misc/resourcehelpers.hpp>

#include <components/resource/resourcesystem.hpp>
#include <components/resource/scenemanager.hpp>
#include <components/sceneutil/actorutil.hpp>
#include <components/sceneutil/attach.hpp>
#include <components/sceneutil/visitor.hpp>
#include <components/sceneutil/skeleton.hpp>
#include <components/sceneutil/lightmanager.hpp>

#include <components/nifosg/nifloader.hpp> // TextKeyMapHolder

#include "../mwworld/esmstore.hpp"
#include "../mwworld/inventorystore.hpp"
#include "../mwworld/class.hpp"
#include "../mwworld/player.hpp"

#include "../mwmechanics/npcstats.hpp"
#include "../mwmechanics/actorutil.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/mechanicsmanager.hpp"
#include "../mwbase/soundmanager.hpp"

#include "camera.hpp"
#include "rotatecontroller.hpp"
#include "renderbin.hpp"
#include "vismask.hpp"

namespace
{

std::string getVampireHead(const std::string& race, bool female)
{
    static std::map <std::pair<std::string,int>, const ESM::BodyPart* > sVampireMapping;

    std::pair<std::string, int> thisCombination = std::make_pair(race, int(female));

    if (sVampireMapping.find(thisCombination) == sVampireMapping.end())
    {
        const MWWorld::ESMStore &store = MWBase::Environment::get().getWorld()->getStore();
        const MWWorld::Store<ESM::BodyPart> &partStore = store.get<ESM::BodyPart>();
        for(MWWorld::Store<ESM::BodyPart>::iterator it = partStore.begin(); it != partStore.end(); ++it)
        {
            const ESM::BodyPart& bodypart = *it;
            if (!bodypart.mData.mVampire)
                continue;
            if (bodypart.mData.mType != ESM::BodyPart::MT_Skin)
                continue;
            if (bodypart.mData.mPart != ESM::BodyPart::MP_Head)
                continue;
            if (female != (bodypart.mData.mFlags & ESM::BodyPart::BPF_Female))
                continue;
            if (!Misc::StringUtils::ciEqual(bodypart.mRace, race))
                continue;
            sVampireMapping[thisCombination] = &*it;
        }
    }

    if (sVampireMapping.find(thisCombination) == sVampireMapping.end())
        sVampireMapping[thisCombination] = nullptr;

    const ESM::BodyPart* bodyPart = sVampireMapping[thisCombination];
    if (!bodyPart)
        return std::string();
    return "meshes\\" + bodyPart->mModel;
}

}


namespace MWRender
{

class HeadAnimationTime : public SceneUtil::ControllerSource
{
private:
    MWWorld::Ptr mReference;
    float mTalkStart;
    float mTalkStop;
    float mBlinkStart;
    float mBlinkStop;

    float mBlinkTimer;

    bool mEnabled;

    float mValue;
private:
    void resetBlinkTimer();
public:
    HeadAnimationTime(const MWWorld::Ptr& reference);

    void updatePtr(const MWWorld::Ptr& updated);

    void update(float dt);

    void setEnabled(bool enabled);

    void setTalkStart(float value);
    void setTalkStop(float value);
    void setBlinkStart(float value);
    void setBlinkStop(float value);

    virtual float getValue(osg::NodeVisitor* nv);
};

// --------------------------------------------------------------------------------

/// Subclass RotateController to add a Z-offset for sneaking in first person mode.
/// @note We use inheritance instead of adding another controller, so that we do not have to compute the worldOrient twice.
/// @note Must be set on a MatrixTransform.
class NeckController : public RotateController
{
public:
    NeckController(osg::Node* relativeTo)
        : RotateController(relativeTo)
    {
    }

    void setOffset(const osg::Vec3f& offset)
    {
        mOffset = offset;
    }

    virtual void operator()(osg::Node* node, osg::NodeVisitor* nv)
    {
        osg::MatrixTransform* transform = static_cast<osg::MatrixTransform*>(node);
        osg::Matrix matrix = transform->getMatrix();

        osg::Quat worldOrient = getWorldOrientation(node);
        osg::Quat orient = worldOrient * mRotate * worldOrient.inverse() * matrix.getRotate();

        matrix.setRotate(orient);
        matrix.setTrans(matrix.getTrans() + worldOrient.inverse() * mOffset);

        transform->setMatrix(matrix);

        traverse(node,nv);
    }

private:
    osg::Vec3f mOffset;
};

// --------------------------------------------------------------------------------------------------------------

HeadAnimationTime::HeadAnimationTime(const MWWorld::Ptr& reference)
    : mReference(reference), mTalkStart(0), mTalkStop(0), mBlinkStart(0), mBlinkStop(0), mEnabled(true), mValue(0)
{
    resetBlinkTimer();
}

void HeadAnimationTime::updatePtr(const MWWorld::Ptr &updated)
{
    mReference = updated;
}

void HeadAnimationTime::setEnabled(bool enabled)
{
    mEnabled = enabled;
}

void HeadAnimationTime::resetBlinkTimer()
{
    mBlinkTimer = -(2.0f + Misc::Rng::rollDice(6));
}

void HeadAnimationTime::update(float dt)
{
    if (!mEnabled)
        return;

    if (MWBase::Environment::get().getSoundManager()->sayDone(mReference))
    {
        mBlinkTimer += dt;

        float duration = mBlinkStop - mBlinkStart;

        if (mBlinkTimer >= 0 && mBlinkTimer <= duration)
        {
            mValue = mBlinkStart + mBlinkTimer;
        }
        else
            mValue = mBlinkStop;

        if (mBlinkTimer > duration)
            resetBlinkTimer();
    }
    else
    {
        // FIXME: would be nice to hold on to the SoundPtr so we don't have to retrieve it every frame
        mValue = mTalkStart +
            (mTalkStop - mTalkStart) *
            std::min(1.f, MWBase::Environment::get().getSoundManager()->getSaySoundLoudness(mReference)*2); // Rescale a bit (most voices are not very loud)
    }
}

float HeadAnimationTime::getValue(osg::NodeVisitor*)
{
    return mValue;
}

void HeadAnimationTime::setTalkStart(float value)
{
    mTalkStart = value;
}

void HeadAnimationTime::setTalkStop(float value)
{
    mTalkStop = value;
}

void HeadAnimationTime::setBlinkStart(float value)
{
    mBlinkStart = value;
}

void HeadAnimationTime::setBlinkStop(float value)
{
    mBlinkStop = value;
}

// ----------------------------------------------------

static NpcAnimation::PartBoneMap createPartListMap()
{
    NpcAnimation::PartBoneMap result;
    result.insert(std::make_pair(ESM::PRT_Head, "Head"));
    result.insert(std::make_pair(ESM::PRT_Hair, "Head")); // note it uses "Head" as attach bone, but "Hair" as filter
    result.insert(std::make_pair(ESM::PRT_Neck, "Neck"));
    result.insert(std::make_pair(ESM::PRT_Cuirass, "Chest"));
    result.insert(std::make_pair(ESM::PRT_Groin, "Groin"));
    result.insert(std::make_pair(ESM::PRT_Skirt, "Groin"));
    result.insert(std::make_pair(ESM::PRT_RHand, "Right Hand"));
    result.insert(std::make_pair(ESM::PRT_LHand, "Left Hand"));
    result.insert(std::make_pair(ESM::PRT_RWrist, "Right Wrist"));
    result.insert(std::make_pair(ESM::PRT_LWrist, "Left Wrist"));
    result.insert(std::make_pair(ESM::PRT_Shield, "Shield Bone"));
    result.insert(std::make_pair(ESM::PRT_RForearm, "Right Forearm"));
    result.insert(std::make_pair(ESM::PRT_LForearm, "Left Forearm"));
    result.insert(std::make_pair(ESM::PRT_RUpperarm, "Right Upper Arm"));
    result.insert(std::make_pair(ESM::PRT_LUpperarm, "Left Upper Arm"));
    result.insert(std::make_pair(ESM::PRT_RFoot, "Right Foot"));
    result.insert(std::make_pair(ESM::PRT_LFoot, "Left Foot"));
    result.insert(std::make_pair(ESM::PRT_RAnkle, "Right Ankle"));
    result.insert(std::make_pair(ESM::PRT_LAnkle, "Left Ankle"));
    result.insert(std::make_pair(ESM::PRT_RKnee, "Right Knee"));
    result.insert(std::make_pair(ESM::PRT_LKnee, "Left Knee"));
    result.insert(std::make_pair(ESM::PRT_RLeg, "Right Upper Leg"));
    result.insert(std::make_pair(ESM::PRT_LLeg, "Left Upper Leg"));
    result.insert(std::make_pair(ESM::PRT_RPauldron, "Right Clavicle"));
    result.insert(std::make_pair(ESM::PRT_LPauldron, "Left Clavicle"));
    result.insert(std::make_pair(ESM::PRT_Weapon, "Weapon Bone"));
    result.insert(std::make_pair(ESM::PRT_Tail, "Tail"));
    return result;
}
const NpcAnimation::PartBoneMap NpcAnimation::sPartList = createPartListMap();

NpcAnimation::~NpcAnimation()
{
    mAmmunition.reset();
}

NpcAnimation::NpcAnimation(const MWWorld::Ptr& ptr, osg::ref_ptr<osg::Group> parentNode, Resource::ResourceSystem* resourceSystem,
                           bool disableSounds, ViewMode viewMode, float firstPersonFieldOfView)
  : ActorAnimation(ptr, parentNode, resourceSystem),
    mViewMode(viewMode),
    mShowWeapons(false),
    mShowCarriedLeft(true),
    mNpcType(Type_Normal),
    mFirstPersonFieldOfView(firstPersonFieldOfView),
    mSoundsDisabled(disableSounds),
    mAccurateAiming(false),
    mAimingFactor(0.f)
{
    mNpc = mPtr.get<ESM::NPC>()->mBase;

    mHeadAnimationTime = std::shared_ptr<HeadAnimationTime>(new HeadAnimationTime(mPtr));
    mWeaponAnimationTime = std::shared_ptr<WeaponAnimationTime>(new WeaponAnimationTime(this));

    for(size_t i = 0;i < ESM::PRT_Count;i++)
    {
        mPartslots[i] = -1;  //each slot is empty
        mPartPriorities[i] = 0;
    }

    updateNpcBase();
}

void NpcAnimation::setViewMode(NpcAnimation::ViewMode viewMode)
{
    assert(viewMode != VM_HeadOnly);
    if(mViewMode == viewMode)
        return;

    mViewMode = viewMode;
    rebuild();

    setRenderBin();
}

/// @brief A RenderBin callback to clear the depth buffer before rendering.
class DepthClearCallback : public osgUtil::RenderBin::DrawCallback
{
public:
    DepthClearCallback()
    {
        mDepth = new osg::Depth;
        mDepth->setWriteMask(true);
    }

    virtual void drawImplementation(osgUtil::RenderBin* bin, osg::RenderInfo& renderInfo, osgUtil::RenderLeaf*& previous)
    {
        renderInfo.getState()->applyAttribute(mDepth);

        glClear(GL_DEPTH_BUFFER_BIT);

        bin->drawImplementation(renderInfo, previous);
    }

    osg::ref_ptr<osg::Depth> mDepth;
};

/// Overrides Field of View to given value for rendering the subgraph.
/// Must be added as cull callback.
class OverrideFieldOfViewCallback : public osg::NodeCallback
{
public:
    OverrideFieldOfViewCallback(float fov)
        : mFov(fov)
    {
    }

    virtual void operator()(osg::Node* node, osg::NodeVisitor* nv)
    {
        osgUtil::CullVisitor* cv = static_cast<osgUtil::CullVisitor*>(nv);
        float fov, aspect, zNear, zFar;
        if (cv->getProjectionMatrix()->getPerspective(fov, aspect, zNear, zFar))
        {
            fov = mFov;
            osg::RefMatrix* newProjectionMatrix = new osg::RefMatrix(*cv->getProjectionMatrix());
            newProjectionMatrix->makePerspective(fov, aspect, zNear, zFar);
            cv->pushProjectionMatrix(newProjectionMatrix);
            traverse(node, nv);
            cv->popProjectionMatrix();
        }
        else
            traverse(node, nv);
    }

private:
    float mFov;
};

void NpcAnimation::setRenderBin()
{
    if (mViewMode == VM_FirstPerson)
    {
        static bool prototypeAdded = false;
        if (!prototypeAdded)
        {
            osg::ref_ptr<osgUtil::RenderBin> depthClearBin (new osgUtil::RenderBin);
            depthClearBin->setDrawCallback(new DepthClearCallback);
            osgUtil::RenderBin::addRenderBinPrototype("DepthClear", depthClearBin);
            prototypeAdded = true;
        }

        osg::StateSet* stateset = mObjectRoot->getOrCreateStateSet();
        stateset->setRenderBinDetails(RenderBin_FirstPerson, "DepthClear", osg::StateSet::OVERRIDE_RENDERBIN_DETAILS);
    }
    else
        Animation::setRenderBin();
}

void NpcAnimation::rebuild()
{
    updateNpcBase();

    MWBase::Environment::get().getMechanicsManager()->forceStateUpdate(mPtr);
}

int NpcAnimation::getSlot(const osg::NodePath &path) const
{
    for (int i=0; i<ESM::PRT_Count; ++i)
    {
        PartHolderPtr part = mObjectParts[i];
        if (!part.get())
            continue;
        if (std::find(path.begin(), path.end(), part->getNode().get()) != path.end())
        {
            return mPartslots[i];
        }
    }
    return -1;
}

void NpcAnimation::updateNpcBase()
{
    clearAnimSources();
    for(size_t i = 0;i < ESM::PRT_Count;i++)
        removeIndividualPart((ESM::PartReferenceType)i);

    const MWWorld::ESMStore &store = MWBase::Environment::get().getWorld()->getStore();
    const ESM::Race *race = store.get<ESM::Race>().find(mNpc->mRace);
    bool isWerewolf = (mNpcType == Type_Werewolf);
    bool isVampire = (mNpcType == Type_Vampire);
    bool isFemale = !mNpc->isMale();

    if (isWerewolf)
    {
        mHeadModel = "meshes\\" + store.get<ESM::BodyPart>().find("WerewolfHead")->mModel;
        mHairModel = "meshes\\" + store.get<ESM::BodyPart>().find("WerewolfHair")->mModel;
    }
    else
    {
        mHeadModel = "";
        const std::string& vampireHead = getVampireHead(mNpc->mRace, isFemale);
        if (isVampire && !vampireHead.empty())
            mHeadModel = vampireHead;
        else if (!mNpc->mHead.empty())
        {
            const ESM::BodyPart* bp = store.get<ESM::BodyPart>().search(mNpc->mHead);
            if (bp)
                mHeadModel = "meshes\\" + bp->mModel;
            else
                Log(Debug::Warning) << "Warning: Failed to load body part '" << mNpc->mHead << "'";
        }

        mHairModel = "";
        if (!mNpc->mHair.empty())
        {
            const ESM::BodyPart* bp = store.get<ESM::BodyPart>().search(mNpc->mHair);
            if (bp)
                mHairModel = "meshes\\" + bp->mModel;
            else
                Log(Debug::Warning) << "Warning: Failed to load body part '" << mNpc->mHair << "'";
        }
    }

    bool is1stPerson = mViewMode == VM_FirstPerson;
    bool isBeast = (race->mData.mFlags & ESM::Race::Beast) != 0;

    std::string smodel = SceneUtil::getActorSkeleton(is1stPerson, isFemale, isBeast, isWerewolf);
    smodel = Misc::ResourceHelpers::correctActorModelPath(smodel, mResourceSystem->getVFS());

    setObjectRoot(smodel, true, true, false);

    if(!is1stPerson)
    {
        const std::string base = "meshes\\xbase_anim.nif";
        if (smodel != base)
            addAnimSource(base, smodel);

        addAnimSource(smodel, smodel);

        if(!isWerewolf)
        {
            if(mNpc->mModel.length() > 0)
                addAnimSource(Misc::ResourceHelpers::correctActorModelPath("meshes\\" + mNpc->mModel, mResourceSystem->getVFS()), smodel);
            if(Misc::StringUtils::lowerCase(mNpc->mRace).find("argonian") != std::string::npos)
                addAnimSource("meshes\\xargonian_swimkna.nif", smodel);
        }
    }
    else
    {
        const std::string base = "meshes\\xbase_anim.1st.nif";
        if (smodel != base)
            addAnimSource(base, smodel);

        addAnimSource(smodel, smodel);

        mObjectRoot->setNodeMask(Mask_FirstPerson);
        mObjectRoot->addCullCallback(new OverrideFieldOfViewCallback(mFirstPersonFieldOfView));
    }

    updateParts();

    mWeaponAnimationTime->updateStartTime();
}

void NpcAnimation::updateParts()
{
    if (!mObjectRoot.get())
        return;

    const MWWorld::Class &cls = mPtr.getClass();

    NpcType curType = Type_Normal;
    if (cls.getCreatureStats(mPtr).getMagicEffects().get(ESM::MagicEffect::Vampirism).getMagnitude() > 0)
        curType = Type_Vampire;
    if (cls.getNpcStats(mPtr).isWerewolf())
        curType = Type_Werewolf;

    if (curType != mNpcType)
    {
        mNpcType = curType;
        rebuild();
        return;
    }

    static const struct {
        int mSlot;
        int mBasePriority;
    } slotlist[] = {
        // FIXME: Priority is based on the number of reserved slots. There should be a better way.
        { MWWorld::InventoryStore::Slot_Robe,         12 },
        { MWWorld::InventoryStore::Slot_Skirt,         3 },
        { MWWorld::InventoryStore::Slot_Helmet,        0 },
        { MWWorld::InventoryStore::Slot_Cuirass,       0 },
        { MWWorld::InventoryStore::Slot_Greaves,       0 },
        { MWWorld::InventoryStore::Slot_LeftPauldron,  0 },
        { MWWorld::InventoryStore::Slot_RightPauldron, 0 },
        { MWWorld::InventoryStore::Slot_Boots,         0 },
        { MWWorld::InventoryStore::Slot_LeftGauntlet,  0 },
        { MWWorld::InventoryStore::Slot_RightGauntlet, 0 },
        { MWWorld::InventoryStore::Slot_Shirt,         0 },
        { MWWorld::InventoryStore::Slot_Pants,         0 },
        { MWWorld::InventoryStore::Slot_CarriedLeft,   0 },
        { MWWorld::InventoryStore::Slot_CarriedRight,  0 }
    };
    static const size_t slotlistsize = sizeof(slotlist)/sizeof(slotlist[0]);

    bool wasArrowAttached = (mAmmunition.get() != nullptr);
    mAmmunition.reset();

    const MWWorld::InventoryStore& inv = mPtr.getClass().getInventoryStore(mPtr);
    for(size_t i = 0;i < slotlistsize && mViewMode != VM_HeadOnly;i++)
    {
        MWWorld::ConstContainerStoreIterator store = inv.getSlot(slotlist[i].mSlot);

        removePartGroup(slotlist[i].mSlot);

        if(store == inv.end())
            continue;

        if(slotlist[i].mSlot == MWWorld::InventoryStore::Slot_Helmet)
            removeIndividualPart(ESM::PRT_Hair);

        int prio = 1;
        bool enchantedGlow = !store->getClass().getEnchantment(*store).empty();
        osg::Vec4f glowColor = getEnchantmentColor(*store);
        if(store->getTypeName() == typeid(ESM::Clothing).name())
        {
            prio = ((slotlist[i].mBasePriority+1)<<1) + 0;
            const ESM::Clothing *clothes = store->get<ESM::Clothing>()->mBase;
            addPartGroup(slotlist[i].mSlot, prio, clothes->mParts.mParts, enchantedGlow, &glowColor);
        }
        else if(store->getTypeName() == typeid(ESM::Armor).name())
        {
            prio = ((slotlist[i].mBasePriority+1)<<1) + 1;
            const ESM::Armor *armor = store->get<ESM::Armor>()->mBase;
            addPartGroup(slotlist[i].mSlot, prio, armor->mParts.mParts, enchantedGlow, &glowColor);
        }

        if(slotlist[i].mSlot == MWWorld::InventoryStore::Slot_Robe)
        {
            ESM::PartReferenceType parts[] = {
                ESM::PRT_Groin, ESM::PRT_Skirt, ESM::PRT_RLeg, ESM::PRT_LLeg,
                ESM::PRT_RUpperarm, ESM::PRT_LUpperarm, ESM::PRT_RKnee, ESM::PRT_LKnee,
                ESM::PRT_RForearm, ESM::PRT_LForearm
            };
            size_t parts_size = sizeof(parts)/sizeof(parts[0]);
            for(size_t p = 0;p < parts_size;++p)
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
        if(mPartPriorities[ESM::PRT_Head] < 1 && !mHeadModel.empty())
            addOrReplaceIndividualPart(ESM::PRT_Head, -1,1, mHeadModel);
        if(mPartPriorities[ESM::PRT_Hair] < 1 && mPartPriorities[ESM::PRT_Head] <= 1 && !mHairModel.empty())
            addOrReplaceIndividualPart(ESM::PRT_Hair, -1,1, mHairModel);
    }
    if(mViewMode == VM_HeadOnly)
        return;

    if(mPartPriorities[ESM::PRT_Shield] < 1)
    {
        MWWorld::ConstContainerStoreIterator store = inv.getSlot(MWWorld::InventoryStore::Slot_CarriedLeft);
        MWWorld::ConstPtr part;
        if(store != inv.end() && (part=*store).getTypeName() == typeid(ESM::Light).name())
        {
            const ESM::Light *light = part.get<ESM::Light>()->mBase;
            addOrReplaceIndividualPart(ESM::PRT_Shield, MWWorld::InventoryStore::Slot_CarriedLeft,
                                       1, "meshes\\"+light->mModel);
            if (mObjectParts[ESM::PRT_Shield])
                addExtraLight(mObjectParts[ESM::PRT_Shield]->getNode()->asGroup(), light);
        }
    }

    showWeapons(mShowWeapons);
    showCarriedLeft(mShowCarriedLeft);

    bool isWerewolf = (mNpcType == Type_Werewolf);
    std::string race = (isWerewolf ? "werewolf" : Misc::StringUtils::lowerCase(mNpc->mRace));

    const std::vector<const ESM::BodyPart*> &parts = getBodyParts(race, !mNpc->isMale(), mViewMode == VM_FirstPerson, isWerewolf);
    for(int part = ESM::PRT_Neck; part < ESM::PRT_Count; ++part)
    {
        if(mPartPriorities[part] < 1)
        {
            const ESM::BodyPart* bodypart = parts[part];
            if(bodypart)
                addOrReplaceIndividualPart((ESM::PartReferenceType)part, -1, 1,
                                           "meshes\\"+bodypart->mModel);
        }
    }

    if (wasArrowAttached)
        attachArrow();

    if (mAlpha != 1.f)
        mResourceSystem->getSceneManager()->recreateShaders(mObjectRoot);
}



PartHolderPtr NpcAnimation::insertBoundedPart(const std::string& model, const std::string& bonename, const std::string& bonefilter, bool enchantedGlow, osg::Vec4f* glowColor)
{
    osg::ref_ptr<osg::Node> instance = mResourceSystem->getSceneManager()->getInstance(model);

    const NodeMap& nodeMap = getNodeMap();
    NodeMap::const_iterator found = nodeMap.find(Misc::StringUtils::lowerCase(bonename));
    if (found == nodeMap.end())
        throw std::runtime_error("Can't find attachment node " + bonename);

    osg::ref_ptr<osg::Node> attached = SceneUtil::attach(instance, mObjectRoot, bonefilter, found->second);
    if (enchantedGlow)
        addGlow(attached, *glowColor);

    return PartHolderPtr(new PartHolder(attached));
}

osg::Vec3f NpcAnimation::runAnimation(float timepassed)
{
    osg::Vec3f ret = Animation::runAnimation(timepassed);

    mHeadAnimationTime->update(timepassed);

    if (mFirstPersonNeckController)
    {
        if (mAccurateAiming)
            mAimingFactor = 1.f;
        else
            mAimingFactor = std::max(0.f, mAimingFactor - timepassed * 0.5f);

        float rotateFactor = 0.75f + 0.25f * mAimingFactor;

        mFirstPersonNeckController->setRotate(osg::Quat(mPtr.getRefData().getPosition().rot[0] * rotateFactor, osg::Vec3f(-1,0,0)));
        mFirstPersonNeckController->setOffset(mFirstPersonOffset);
    }

    WeaponAnimation::configureControllers(mPtr.getRefData().getPosition().rot[0]);

    return ret;
}

void NpcAnimation::removeIndividualPart(ESM::PartReferenceType type)
{
    mPartPriorities[type] = 0;
    mPartslots[type] = -1;

    mObjectParts[type].reset();
    if (!mSoundIds[type].empty() && !mSoundsDisabled)
    {
        MWBase::Environment::get().getSoundManager()->stopSound3D(mPtr, mSoundIds[type]);
        mSoundIds[type].clear();
    }
}

void NpcAnimation::reserveIndividualPart(ESM::PartReferenceType type, int group, int priority)
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
    for(int i = 0; i < ESM::PRT_Count; i++)
    {
        if(mPartslots[i] == group)
            removeIndividualPart((ESM::PartReferenceType)i);
    }
}

bool NpcAnimation::isFirstPersonPart(const ESM::BodyPart* bodypart)
{
    return (bodypart->mId.size() >= 3)
        && bodypart->mId[bodypart->mId.size()-3] == '1'
        && bodypart->mId[bodypart->mId.size()-2] == 's'
        && bodypart->mId[bodypart->mId.size()-1] == 't';
}

bool NpcAnimation::isFemalePart(const ESM::BodyPart* bodypart)
{
    return bodypart->mData.mFlags & ESM::BodyPart::BPF_Female;
}

bool NpcAnimation::addOrReplaceIndividualPart(ESM::PartReferenceType type, int group, int priority, const std::string &mesh, bool enchantedGlow, osg::Vec4f* glowColor)
{
    if(priority <= mPartPriorities[type])
        return false;

    removeIndividualPart(type);
    mPartslots[type] = group;
    mPartPriorities[type] = priority;
    try
    {
        const std::string& bonename = sPartList.at(type);
        // PRT_Hair seems to be the only type that breaks consistency and uses a filter that's different from the attachment bone
        const std::string bonefilter = (type == ESM::PRT_Hair) ? "hair" : bonename;
        mObjectParts[type] = insertBoundedPart(mesh, bonename, bonefilter, enchantedGlow, glowColor);
    }
    catch (std::exception& e)
    {
        Log(Debug::Error) << "Error adding NPC part: " << e.what();
        return false;
    }

    if (!mSoundsDisabled)
    {
        const MWWorld::InventoryStore& inv = mPtr.getClass().getInventoryStore(mPtr);
        MWWorld::ConstContainerStoreIterator csi = inv.getSlot(group < 0 ? MWWorld::InventoryStore::Slot_Helmet : group);
        if (csi != inv.end())
        {
            mSoundIds[type] = csi->getClass().getSound(*csi);
            if (!mSoundIds[type].empty())
            {
                MWBase::Environment::get().getSoundManager()->playSound3D(mPtr, mSoundIds[type],
                    1.0f, 1.0f, MWSound::Type::Sfx, MWSound::PlayMode::Loop
                );
            }
        }
    }

    osg::Node* node = mObjectParts[type]->getNode();
    if (node->getNumChildrenRequiringUpdateTraversal() > 0)
    {
        std::shared_ptr<SceneUtil::ControllerSource> src;
        if (type == ESM::PRT_Head)
        {
            src = mHeadAnimationTime;

            if (node->getUserDataContainer())
            {
                for (unsigned int i=0; i<node->getUserDataContainer()->getNumUserObjects(); ++i)
                {
                    osg::Object* obj = node->getUserDataContainer()->getUserObject(i);
                    if (NifOsg::TextKeyMapHolder* keys = dynamic_cast<NifOsg::TextKeyMapHolder*>(obj))
                    {
                        for (NifOsg::TextKeyMap::const_iterator it = keys->mTextKeys.begin(); it != keys->mTextKeys.end(); ++it)
                        {
                            if (Misc::StringUtils::ciEqual(it->second, "talk: start"))
                                mHeadAnimationTime->setTalkStart(it->first);
                            if (Misc::StringUtils::ciEqual(it->second, "talk: stop"))
                                mHeadAnimationTime->setTalkStop(it->first);
                            if (Misc::StringUtils::ciEqual(it->second, "blink: start"))
                                mHeadAnimationTime->setBlinkStart(it->first);
                            if (Misc::StringUtils::ciEqual(it->second, "blink: stop"))
                                mHeadAnimationTime->setBlinkStop(it->first);
                        }

                        break;
                    }
                }
            }
        }
        else if (type == ESM::PRT_Weapon)
            src = mWeaponAnimationTime;
        else
            src.reset(new NullAnimationTime);

        SceneUtil::AssignControllerSourcesVisitor assignVisitor(src);
        node->accept(assignVisitor);
    }

    return true;
}

void NpcAnimation::addPartGroup(int group, int priority, const std::vector<ESM::PartReference> &parts, bool enchantedGlow, osg::Vec4f* glowColor)
{
    const MWWorld::ESMStore &store = MWBase::Environment::get().getWorld()->getStore();
    const MWWorld::Store<ESM::BodyPart> &partStore = store.get<ESM::BodyPart>();

    const char *ext = (mViewMode == VM_FirstPerson) ? ".1st" : "";
    std::vector<ESM::PartReference>::const_iterator part(parts.begin());
    for(;part != parts.end();++part)
    {
        const ESM::BodyPart *bodypart = 0;
        if(!mNpc->isMale() && !part->mFemale.empty())
        {
            bodypart = partStore.search(part->mFemale+ext);
            if(!bodypart && mViewMode == VM_FirstPerson)
            {
                bodypart = partStore.search(part->mFemale);
                if(bodypart && !(bodypart->mData.mPart == ESM::BodyPart::MP_Hand ||
                                 bodypart->mData.mPart == ESM::BodyPart::MP_Wrist ||
                                 bodypart->mData.mPart == ESM::BodyPart::MP_Forearm ||
                                 bodypart->mData.mPart == ESM::BodyPart::MP_Upperarm))
                    bodypart = nullptr;
            }
            else if (!bodypart)
                Log(Debug::Warning) << "Warning: Failed to find body part '" << part->mFemale << "'";
        }
        if(!bodypart && !part->mMale.empty())
        {
            bodypart = partStore.search(part->mMale+ext);
            if(!bodypart && mViewMode == VM_FirstPerson)
            {
                bodypart = partStore.search(part->mMale);
                if(bodypart && !(bodypart->mData.mPart == ESM::BodyPart::MP_Hand ||
                                 bodypart->mData.mPart == ESM::BodyPart::MP_Wrist ||
                                 bodypart->mData.mPart == ESM::BodyPart::MP_Forearm ||
                                 bodypart->mData.mPart == ESM::BodyPart::MP_Upperarm))
                    bodypart = nullptr;
            }
            else if (!bodypart)
                Log(Debug::Warning) << "Warning: Failed to find body part '" << part->mMale << "'";
        }

        if(bodypart)
            addOrReplaceIndividualPart((ESM::PartReferenceType)part->mPart, group, priority, "meshes\\"+bodypart->mModel, enchantedGlow, glowColor);
        else
            reserveIndividualPart((ESM::PartReferenceType)part->mPart, group, priority);
    }
}

void NpcAnimation::addControllers()
{
    Animation::addControllers();

    mFirstPersonNeckController = nullptr;
    WeaponAnimation::deleteControllers();

    if (mViewMode == VM_FirstPerson)
    {
        NodeMap::iterator found = mNodeMap.find("bip01 neck");
        if (found != mNodeMap.end())
        {
            osg::MatrixTransform* node = found->second.get();
            mFirstPersonNeckController = new NeckController(mObjectRoot.get());
            node->addUpdateCallback(mFirstPersonNeckController);
            mActiveControllers.insert(std::make_pair(node, mFirstPersonNeckController));
        }
    }
    else if (mViewMode == VM_Normal)
    {
        WeaponAnimation::addControllers(mNodeMap, mActiveControllers, mObjectRoot.get());
    }
}

void NpcAnimation::showWeapons(bool showWeapon)
{
    mShowWeapons = showWeapon;
    mAmmunition.reset();
    if(showWeapon)
    {
        const MWWorld::InventoryStore& inv = mPtr.getClass().getInventoryStore(mPtr);
        MWWorld::ConstContainerStoreIterator weapon = inv.getSlot(MWWorld::InventoryStore::Slot_CarriedRight);
        if(weapon != inv.end())
        {
            osg::Vec4f glowColor = getEnchantmentColor(*weapon);
            std::string mesh = weapon->getClass().getModel(*weapon);
            addOrReplaceIndividualPart(ESM::PRT_Weapon, MWWorld::InventoryStore::Slot_CarriedRight, 1,
                                       mesh, !weapon->getClass().getEnchantment(*weapon).empty(), &glowColor);

            // Crossbows start out with a bolt attached
            if (weapon->getTypeName() == typeid(ESM::Weapon).name() &&
                    weapon->get<ESM::Weapon>()->mBase->mData.mType == ESM::Weapon::MarksmanCrossbow)
            {
                MWWorld::ConstContainerStoreIterator ammo = inv.getSlot(MWWorld::InventoryStore::Slot_Ammunition);
                if (ammo != inv.end() && ammo->get<ESM::Weapon>()->mBase->mData.mType == ESM::Weapon::Bolt)
                    attachArrow();
            }
        }
        if (mAlpha != 1.f)
            mResourceSystem->getSceneManager()->recreateShaders(mObjectRoot);
    }
    else
    {
        removeIndividualPart(ESM::PRT_Weapon);
        // If we remove/hide weapon from player, we should reset attack animation as well
        if (mPtr == MWMechanics::getPlayer())
            MWBase::Environment::get().getWorld()->getPlayer().setAttackingOrSpell(false);
    }
}

void NpcAnimation::showCarriedLeft(bool show)
{
    mShowCarriedLeft = show;
    const MWWorld::InventoryStore& inv = mPtr.getClass().getInventoryStore(mPtr);
    MWWorld::ConstContainerStoreIterator iter = inv.getSlot(MWWorld::InventoryStore::Slot_CarriedLeft);
    if(show && iter != inv.end())
    {
        osg::Vec4f glowColor = getEnchantmentColor(*iter);
        std::string mesh = iter->getClass().getModel(*iter);
        if (addOrReplaceIndividualPart(ESM::PRT_Shield, MWWorld::InventoryStore::Slot_CarriedLeft, 1,
                                   mesh, !iter->getClass().getEnchantment(*iter).empty(), &glowColor))
        {
            if (iter->getTypeName() == typeid(ESM::Light).name() && mObjectParts[ESM::PRT_Shield])
                addExtraLight(mObjectParts[ESM::PRT_Shield]->getNode()->asGroup(), iter->get<ESM::Light>()->mBase);
        }
        if (mAlpha != 1.f)
            mResourceSystem->getSceneManager()->recreateShaders(mObjectRoot);
    }
    else
        removeIndividualPart(ESM::PRT_Shield);
}

void NpcAnimation::attachArrow()
{
    WeaponAnimation::attachArrow(mPtr);
}

void NpcAnimation::releaseArrow(float attackStrength)
{
    WeaponAnimation::releaseArrow(mPtr, attackStrength);
}

osg::Group* NpcAnimation::getArrowBone()
{
    PartHolderPtr part = mObjectParts[ESM::PRT_Weapon];
    if (!part)
        return nullptr;

    SceneUtil::FindByNameVisitor findVisitor ("ArrowBone");
    part->getNode()->accept(findVisitor);

    return findVisitor.mFoundNode;
}

osg::Node* NpcAnimation::getWeaponNode()
{
    PartHolderPtr part = mObjectParts[ESM::PRT_Weapon];
    if (!part)
        return nullptr;
    return part->getNode();
}

Resource::ResourceSystem* NpcAnimation::getResourceSystem()
{
    return mResourceSystem;
}

void NpcAnimation::permanentEffectAdded(const ESM::MagicEffect *magicEffect, bool isNew)
{
    // During first auto equip, we don't play any sounds.
    // Basically we don't want sounds when the actor is first loaded,
    // the items should appear as if they'd always been equipped.
    if (isNew)
    {
        static const std::string schools[] = {
            "alteration", "conjuration", "destruction", "illusion", "mysticism", "restoration"
        };

        MWBase::SoundManager *sndMgr = MWBase::Environment::get().getSoundManager();
        if(!magicEffect->mHitSound.empty())
            sndMgr->playSound3D(mPtr, magicEffect->mHitSound, 1.0f, 1.0f);
        else
            sndMgr->playSound3D(mPtr, schools[magicEffect->mData.mSchool]+" hit", 1.0f, 1.0f);
    }

    if (!magicEffect->mHit.empty())
    {
        const ESM::Static* castStatic = MWBase::Environment::get().getWorld()->getStore().get<ESM::Static>().find (magicEffect->mHit);
        bool loop = (magicEffect->mData.mFlags & ESM::MagicEffect::ContinuousVfx) != 0;
        // Don't play particle VFX unless the effect is new or it should be looping.
        if (isNew || loop)
            addEffect("meshes\\" + castStatic->mModel, magicEffect->mIndex, loop, "", magicEffect->mParticle);
    }
}

void NpcAnimation::enableHeadAnimation(bool enable)
{
    mHeadAnimationTime->setEnabled(enable);
}

void NpcAnimation::setWeaponGroup(const std::string &group)
{
    mWeaponAnimationTime->setGroup(group);
}

void NpcAnimation::equipmentChanged()
{
    updateParts();
}

void NpcAnimation::setVampire(bool vampire)
{
    if (mNpcType == Type_Werewolf) // we can't have werewolf vampires, can we
        return;
    if ((mNpcType == Type_Vampire) != vampire)
    {
        if (mPtr == MWMechanics::getPlayer())
            MWBase::Environment::get().getWorld()->reattachPlayerCamera();
        else
            rebuild();
    }
}

void NpcAnimation::setFirstPersonOffset(const osg::Vec3f &offset)
{
    mFirstPersonOffset = offset;
}

void NpcAnimation::updatePtr(const MWWorld::Ptr &updated)
{
    Animation::updatePtr(updated);
    mHeadAnimationTime->updatePtr(updated);
}

// Remember body parts so we only have to search through the store once for each race/gender/viewmode combination
typedef std::map< std::pair<std::string,int>,std::vector<const ESM::BodyPart*> > RaceMapping;
static RaceMapping sRaceMapping;

const std::vector<const ESM::BodyPart *>& NpcAnimation::getBodyParts(const std::string &race, bool female, bool firstPerson, bool werewolf)
{
    static const int Flag_FirstPerson = 1<<1;
    static const int Flag_Female      = 1<<0;

    int flags = (werewolf ? -1 : 0);
    if(female)
        flags |= Flag_Female;
    if(firstPerson)
        flags |= Flag_FirstPerson;

    RaceMapping::iterator found = sRaceMapping.find(std::make_pair(race, flags));
    if (found != sRaceMapping.end())
        return found->second;
    else
    {
        std::vector<const ESM::BodyPart*>& parts = sRaceMapping[std::make_pair(race, flags)];

        typedef std::multimap<ESM::BodyPart::MeshPart,ESM::PartReferenceType> BodyPartMapType;
        static BodyPartMapType sBodyPartMap;
        if(sBodyPartMap.empty())
        {
            sBodyPartMap.insert(std::make_pair(ESM::BodyPart::MP_Neck, ESM::PRT_Neck));
            sBodyPartMap.insert(std::make_pair(ESM::BodyPart::MP_Chest, ESM::PRT_Cuirass));
            sBodyPartMap.insert(std::make_pair(ESM::BodyPart::MP_Groin, ESM::PRT_Groin));
            sBodyPartMap.insert(std::make_pair(ESM::BodyPart::MP_Hand, ESM::PRT_RHand));
            sBodyPartMap.insert(std::make_pair(ESM::BodyPart::MP_Hand, ESM::PRT_LHand));
            sBodyPartMap.insert(std::make_pair(ESM::BodyPart::MP_Wrist, ESM::PRT_RWrist));
            sBodyPartMap.insert(std::make_pair(ESM::BodyPart::MP_Wrist, ESM::PRT_LWrist));
            sBodyPartMap.insert(std::make_pair(ESM::BodyPart::MP_Forearm, ESM::PRT_RForearm));
            sBodyPartMap.insert(std::make_pair(ESM::BodyPart::MP_Forearm, ESM::PRT_LForearm));
            sBodyPartMap.insert(std::make_pair(ESM::BodyPart::MP_Upperarm, ESM::PRT_RUpperarm));
            sBodyPartMap.insert(std::make_pair(ESM::BodyPart::MP_Upperarm, ESM::PRT_LUpperarm));
            sBodyPartMap.insert(std::make_pair(ESM::BodyPart::MP_Foot, ESM::PRT_RFoot));
            sBodyPartMap.insert(std::make_pair(ESM::BodyPart::MP_Foot, ESM::PRT_LFoot));
            sBodyPartMap.insert(std::make_pair(ESM::BodyPart::MP_Ankle, ESM::PRT_RAnkle));
            sBodyPartMap.insert(std::make_pair(ESM::BodyPart::MP_Ankle, ESM::PRT_LAnkle));
            sBodyPartMap.insert(std::make_pair(ESM::BodyPart::MP_Knee, ESM::PRT_RKnee));
            sBodyPartMap.insert(std::make_pair(ESM::BodyPart::MP_Knee, ESM::PRT_LKnee));
            sBodyPartMap.insert(std::make_pair(ESM::BodyPart::MP_Upperleg, ESM::PRT_RLeg));
            sBodyPartMap.insert(std::make_pair(ESM::BodyPart::MP_Upperleg, ESM::PRT_LLeg));
            sBodyPartMap.insert(std::make_pair(ESM::BodyPart::MP_Tail, ESM::PRT_Tail));
        }

        parts.resize(ESM::PRT_Count, nullptr);

        const MWWorld::ESMStore &store = MWBase::Environment::get().getWorld()->getStore();
        const MWWorld::Store<ESM::BodyPart> &partStore = store.get<ESM::BodyPart>();
        for(MWWorld::Store<ESM::BodyPart>::iterator it = partStore.begin(); it != partStore.end(); ++it)
        {
            if(werewolf)
                break;
            const ESM::BodyPart& bodypart = *it;
            if (bodypart.mData.mFlags & ESM::BodyPart::BPF_NotPlayable)
                continue;
            if (bodypart.mData.mType != ESM::BodyPart::MT_Skin)
                continue;

            if (!Misc::StringUtils::ciEqual(bodypart.mRace, race))
                continue;

            bool partFirstPerson = isFirstPersonPart(&bodypart);

            bool isHand = bodypart.mData.mPart == ESM::BodyPart::MP_Hand ||
                                    bodypart.mData.mPart == ESM::BodyPart::MP_Wrist ||
                                    bodypart.mData.mPart == ESM::BodyPart::MP_Forearm ||
                                    bodypart.mData.mPart == ESM::BodyPart::MP_Upperarm;

            bool isSameGender = isFemalePart(&bodypart) == female;

            /* A fallback for the arms if 1st person is missing:
             1. Try to use 3d person skin for same gender
             2. Try to use 1st person skin for male, if female == true
             3. Try to use 3d person skin for male, if female == true

             A fallback in another cases: allow to use male bodyparts, if female == true
            */
            if (firstPerson && isHand && !partFirstPerson)
            {
                // Allow 3rd person skins as a fallback for the arms if 1st person is missing
                BodyPartMapType::const_iterator bIt = sBodyPartMap.lower_bound(BodyPartMapType::key_type(bodypart.mData.mPart));
                while(bIt != sBodyPartMap.end() && bIt->first == bodypart.mData.mPart)
                {
                    // If we have no fallback bodypart now and bodypart is for same gender (1)
                    if(!parts[bIt->second] && isSameGender)
                       parts[bIt->second] = &bodypart;

                    // If we have fallback bodypart for other gender and found fallback for current gender (1)
                    else if(isSameGender && isFemalePart(parts[bIt->second]) != female)
                       parts[bIt->second] = &bodypart;

                    // If we have no fallback bodypart and searching for female bodyparts (3)
                    else if(!parts[bIt->second] && female)
                       parts[bIt->second] = &bodypart;

                    ++bIt;
                }

                continue;
            }

            // Don't allow to use podyparts for a different view
            if (partFirstPerson != firstPerson)
                continue;

            if (female && !isFemalePart(&bodypart))
            {
                // Allow male parts as fallback for females if female parts are missing
                BodyPartMapType::const_iterator bIt = sBodyPartMap.lower_bound(BodyPartMapType::key_type(bodypart.mData.mPart));
                while(bIt != sBodyPartMap.end() && bIt->first == bodypart.mData.mPart)
                {
                    // If we have no fallback bodypart now
                    if(!parts[bIt->second])
                        parts[bIt->second] = &bodypart;

                    // If we have 3d person fallback bodypart for hand and 1st person fallback found (2)
                    else if(isHand && !isFirstPersonPart(parts[bIt->second]) && partFirstPerson)
                        parts[bIt->second] = &bodypart;

                    ++bIt;
                }

                continue;
            }

            // Don't allow to use podyparts for another gender
            if (female != isFemalePart(&bodypart))
                continue;

            // Use properly found bodypart, replacing fallbacks
            BodyPartMapType::const_iterator bIt = sBodyPartMap.lower_bound(BodyPartMapType::key_type(bodypart.mData.mPart));
            while(bIt != sBodyPartMap.end() && bIt->first == bodypart.mData.mPart)
            {
                parts[bIt->second] = &bodypart;
                ++bIt;
            }
        }
        return parts;
    }
}

void NpcAnimation::setAccurateAiming(bool enabled)
{
    mAccurateAiming = enabled;
}

}

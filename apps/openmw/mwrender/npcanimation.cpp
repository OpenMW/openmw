#include "npcanimation.hpp"

#include <osg/Depth>
#include <osg/MatrixTransform>
#include <osg/UserDataContainer>

#include <osgUtil/CullVisitor>
#include <osgUtil/RenderBin>

#include <components/debug/debuglog.hpp>

#include <components/misc/rng.hpp>

#include <components/misc/resourcehelpers.hpp>

#include <components/esm3/loadbody.hpp>
#include <components/esm3/loadmgef.hpp>
#include <components/esm3/loadrace.hpp>
#include <components/resource/resourcesystem.hpp>
#include <components/resource/scenemanager.hpp>
#include <components/sceneutil/depth.hpp>
#include <components/sceneutil/keyframe.hpp>
#include <components/sceneutil/lightcommon.hpp>
#include <components/sceneutil/visitor.hpp>
#include <components/settings/values.hpp>

#include <components/vfs/manager.hpp>

#include "../mwworld/class.hpp"
#include "../mwworld/esmstore.hpp"
#include "../mwworld/inventorystore.hpp"

#include "../mwmechanics/actorutil.hpp"
#include "../mwmechanics/npcstats.hpp"
#include "../mwmechanics/weapontype.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/mechanicsmanager.hpp"
#include "../mwbase/soundmanager.hpp"
#include "../mwbase/world.hpp"

#include "actorutil.hpp"
#include "postprocessor.hpp"
#include "renderbin.hpp"
#include "rotatecontroller.hpp"
#include "vismask.hpp"

namespace
{

    std::string getVampireHead(const ESM::RefId& race, bool female)
    {
        static std::map<std::pair<ESM::RefId, int>, const ESM::BodyPart*> sVampireMapping;

        std::pair<ESM::RefId, int> thisCombination = std::make_pair(race, int(female));

        if (sVampireMapping.find(thisCombination) == sVampireMapping.end())
        {
            const MWWorld::ESMStore& store = *MWBase::Environment::get().getESMStore();
            for (const ESM::BodyPart& bodypart : store.get<ESM::BodyPart>())
            {
                if (!bodypart.mData.mVampire)
                    continue;
                if (bodypart.mData.mType != ESM::BodyPart::MT_Skin)
                    continue;
                if (bodypart.mData.mPart != ESM::BodyPart::MP_Head)
                    continue;
                if (female != (bodypart.mData.mFlags & ESM::BodyPart::BPF_Female))
                    continue;
                if (!(bodypart.mRace == race))
                    continue;
                sVampireMapping[thisCombination] = &bodypart;
            }
        }

        sVampireMapping.emplace(thisCombination, nullptr);

        const ESM::BodyPart* bodyPart = sVampireMapping[thisCombination];
        if (!bodyPart)
            return std::string();
        return Misc::ResourceHelpers::correctMeshPath(bodyPart->mModel);
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

        float getValue(osg::NodeVisitor* nv) override;
    };

    // --------------------------------------------------------------------------------------------------------------

    HeadAnimationTime::HeadAnimationTime(const MWWorld::Ptr& reference)
        : mReference(reference)
        , mTalkStart(0)
        , mTalkStop(0)
        , mBlinkStart(0)
        , mBlinkStop(0)
        , mEnabled(true)
        , mValue(0)
    {
        resetBlinkTimer();
    }

    void HeadAnimationTime::updatePtr(const MWWorld::Ptr& updated)
    {
        mReference = updated;
    }

    void HeadAnimationTime::setEnabled(bool enabled)
    {
        mEnabled = enabled;
    }

    void HeadAnimationTime::resetBlinkTimer()
    {
        auto& prng = MWBase::Environment::get().getWorld()->getPrng();
        mBlinkTimer = -(2.0f + Misc::Rng::rollDice(6, prng));
    }

    void HeadAnimationTime::update(float dt)
    {
        if (!mEnabled)
            return;

        if (dt == 0.f)
            return;

        if (!MWBase::Environment::get().getSoundManager()->sayActive(mReference))
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
            mValue = mTalkStart
                + (mTalkStop - mTalkStart)
                    * std::min(1.f,
                        MWBase::Environment::get().getSoundManager()->getSaySoundLoudness(mReference)
                            * 2); // Rescale a bit (most voices are not very loud)
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

    NpcAnimation::NpcType NpcAnimation::getNpcType() const
    {
        const MWWorld::Class& cls = mPtr.getClass();
        // Dead vampires should typically stay vampires.
        if (mNpcType == Type_Vampire && cls.getNpcStats(mPtr).isDead() && !cls.getNpcStats(mPtr).isWerewolf())
            return mNpcType;
        return getNpcType(mPtr);
    }

    NpcAnimation::NpcType NpcAnimation::getNpcType(const MWWorld::Ptr& ptr)
    {
        const MWWorld::Class& cls = ptr.getClass();
        NpcAnimation::NpcType curType = Type_Normal;
        if (cls.getCreatureStats(ptr).getMagicEffects().getOrDefault(ESM::MagicEffect::Vampirism).getMagnitude() > 0)
            curType = Type_Vampire;
        if (cls.getNpcStats(ptr).isWerewolf())
            curType = Type_Werewolf;

        return curType;
    }

    static const inline NpcAnimation::PartBoneMap createPartListMap()
    {
        return { { ESM::PRT_Head, "Head" },
            { ESM::PRT_Hair, "Head" }, // note it uses "Head" as attach bone, but "Hair" as filter
            { ESM::PRT_Neck, "Neck" }, { ESM::PRT_Cuirass, "Chest" }, { ESM::PRT_Groin, "Groin" },
            { ESM::PRT_Skirt, "Groin" }, { ESM::PRT_RHand, "Right Hand" }, { ESM::PRT_LHand, "Left Hand" },
            { ESM::PRT_RWrist, "Right Wrist" }, { ESM::PRT_LWrist, "Left Wrist" }, { ESM::PRT_Shield, "Shield Bone" },
            { ESM::PRT_RForearm, "Right Forearm" }, { ESM::PRT_LForearm, "Left Forearm" },
            { ESM::PRT_RUpperarm, "Right Upper Arm" }, { ESM::PRT_LUpperarm, "Left Upper Arm" },
            { ESM::PRT_RFoot, "Right Foot" }, { ESM::PRT_LFoot, "Left Foot" }, { ESM::PRT_RAnkle, "Right Ankle" },
            { ESM::PRT_LAnkle, "Left Ankle" }, { ESM::PRT_RKnee, "Right Knee" }, { ESM::PRT_LKnee, "Left Knee" },
            { ESM::PRT_RLeg, "Right Upper Leg" }, { ESM::PRT_LLeg, "Left Upper Leg" },
            { ESM::PRT_RPauldron, "Right Clavicle" }, { ESM::PRT_LPauldron, "Left Clavicle" },
            { ESM::PRT_Weapon, "Weapon Bone" }, // Fallback. The real node name depends on the current weapon type.
            { ESM::PRT_Tail, "Tail" } };
    }
    const NpcAnimation::PartBoneMap NpcAnimation::sPartList = createPartListMap();

    NpcAnimation::~NpcAnimation()
    {
        mAmmunition.reset();
    }

    NpcAnimation::NpcAnimation(const MWWorld::Ptr& ptr, osg::ref_ptr<osg::Group> parentNode,
        Resource::ResourceSystem* resourceSystem, bool disableSounds, ViewMode viewMode, float firstPersonFieldOfView)
        : ActorAnimation(ptr, std::move(parentNode), resourceSystem)
        , mViewMode(viewMode)
        , mShowWeapons(false)
        , mShowCarriedLeft(true)
        , mNpcType(getNpcType(ptr))
        , mFirstPersonFieldOfView(firstPersonFieldOfView)
        , mSoundsDisabled(disableSounds)
        , mAccurateAiming(false)
        , mAimingFactor(0.f)
    {
        mNpc = mPtr.get<ESM::NPC>()->mBase;

        mHeadAnimationTime = std::make_shared<HeadAnimationTime>(mPtr);
        mWeaponAnimationTime = std::make_shared<WeaponAnimationTime>(this);

        for (size_t i = 0; i < ESM::PRT_Count; i++)
        {
            mPartslots[i] = -1; // each slot is empty
            mPartPriorities[i] = 0;
        }

        std::fill(mSounds.begin(), mSounds.end(), nullptr);

        updateNpcBase();
    }

    void NpcAnimation::setViewMode(NpcAnimation::ViewMode viewMode)
    {
        assert(viewMode != VM_HeadOnly);
        if (mViewMode == viewMode)
            return;
        // FIXME: sheathing state must be consistent if the third person skeleton doesn't have the necessary node, but
        // third person skeleton is unavailable in first person view. This is a hack to avoid cosmetic issues.
        bool viewChange = mViewMode == VM_FirstPerson || viewMode == VM_FirstPerson;
        mViewMode = viewMode;
        MWBase::Environment::get().getWorld()->scaleObject(
            mPtr, mPtr.getCellRef().getScale(), true); // apply race height after view change

        mAmmunition.reset();
        rebuild();
        setRenderBin();

        if (viewChange && Settings::game().mShieldSheathing)
        {
            int weaptype = ESM::Weapon::None;
            MWMechanics::getActiveWeapon(mPtr, &weaptype);
            showCarriedLeft(updateCarriedLeftVisible(weaptype));
        }
    }

    /// @brief A RenderBin callback to clear the depth buffer before rendering.
    /// Switches depth attachments to a proxy renderbuffer, reattaches original depth then redraws first person root.
    /// This gives a complete depth buffer which can be used for postprocessing, buffer resolves as if depth was never
    /// cleared.
    class DepthClearCallback : public osgUtil::RenderBin::DrawCallback
    {
    public:
        DepthClearCallback()
        {
            mDepth = new SceneUtil::AutoDepth;
            mDepth->setWriteMask(true);

            mStateSet = new osg::StateSet;
            mStateSet->setAttributeAndModes(new osg::ColorMask(false, false, false, false), osg::StateAttribute::ON);
            mStateSet->setMode(GL_LIGHTING, osg::StateAttribute::OFF | osg::StateAttribute::OVERRIDE);
        }

        void drawImplementation(
            osgUtil::RenderBin* bin, osg::RenderInfo& renderInfo, osgUtil::RenderLeaf*& previous) override
        {
            osg::State* state = renderInfo.getState();

            PostProcessor* postProcessor = static_cast<PostProcessor*>(renderInfo.getCurrentCamera()->getUserData());

            state->applyAttribute(mDepth);

            unsigned int frameId = state->getFrameStamp()->getFrameNumber() % 2;

            postProcessor->getFbo(PostProcessor::FBO_FirstPerson, frameId)->apply(*state);
            glClear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
            // color accumulation pass
            bin->drawImplementation(renderInfo, previous);

            auto primaryFBO = postProcessor->getPrimaryFbo(frameId);
            primaryFBO->apply(*state);

//            postProcessor->getFbo(PostProcessor::FBO_OpaqueDepth, frameId)->apply(*state);

            // depth accumulation pass
            osg::ref_ptr<osg::StateSet> restore = bin->getStateSet();
            bin->setStateSet(mStateSet);
            bin->drawImplementation(renderInfo, previous);
            bin->setStateSet(restore);

            primaryFBO->apply(*state);

            state->checkGLErrors("after DepthClearCallback::drawImplementation");
        }

        osg::ref_ptr<osg::Depth> mDepth;
        osg::ref_ptr<osg::StateSet> mStateSet;
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

        void operator()(osg::Node* node, osg::NodeVisitor* nv) override
        {
            osgUtil::CullVisitor* cv = static_cast<osgUtil::CullVisitor*>(nv);
            float fov, aspect, zNear, zFar;
            if (cv->getProjectionMatrix()->getPerspective(fov, aspect, zNear, zFar) && std::abs(fov - mFov) > 0.001)
            {
                fov = mFov;
                osg::ref_ptr<osg::RefMatrix> newProjectionMatrix = new osg::RefMatrix();
                newProjectionMatrix->makePerspective(fov, aspect, zNear, zFar);
                osg::ref_ptr<osg::RefMatrix> invertedOldMatrix = cv->getProjectionMatrix();
                invertedOldMatrix = new osg::RefMatrix(osg::RefMatrix::inverse(*invertedOldMatrix));
                osg::ref_ptr<osg::RefMatrix> viewMatrix = new osg::RefMatrix(*cv->getModelViewMatrix());
                viewMatrix->postMult(*newProjectionMatrix);
                viewMatrix->postMult(*invertedOldMatrix);
                cv->pushModelViewMatrix(viewMatrix, osg::Transform::ReferenceFrame::ABSOLUTE_RF);
                traverse(node, nv);
                cv->popModelViewMatrix();
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
                osg::ref_ptr<osgUtil::RenderBin> depthClearBin(new osgUtil::RenderBin);
                depthClearBin->setDrawCallback(new DepthClearCallback());
                osgUtil::RenderBin::addRenderBinPrototype("DepthClear", depthClearBin);
                prototypeAdded = true;
            }
            mObjectRoot->getOrCreateStateSet()->setRenderBinDetails(
                RenderBin_FirstPerson, "DepthClear", osg::StateSet::OVERRIDE_RENDERBIN_DETAILS);
        }
        else if (osg::StateSet* stateset = mObjectRoot->getStateSet())
            stateset->setRenderBinToInherit();
    }

    void NpcAnimation::rebuild()
    {
        mScabbard.reset();
        mHolsteredShield.reset();
        updateNpcBase();

        MWBase::Environment::get().getMechanicsManager()->forceStateUpdate(mPtr);
    }

    int NpcAnimation::getSlot(const osg::NodePath& path) const
    {
        for (int i = 0; i < ESM::PRT_Count; ++i)
        {
            const PartHolder* const part = mObjectParts[i].get();
            if (part == nullptr)
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
        for (size_t i = 0; i < ESM::PRT_Count; i++)
            removeIndividualPart((ESM::PartReferenceType)i);

        const MWWorld::ESMStore& store = *MWBase::Environment::get().getESMStore();
        const ESM::Race* race = store.get<ESM::Race>().find(mNpc->mRace);
        NpcType curType = getNpcType();
        bool isWerewolf = (curType == Type_Werewolf);
        bool isVampire = (curType == Type_Vampire);
        bool isFemale = !mNpc->isMale();

        mHeadModel.clear();
        mHairModel.clear();

        const ESM::RefId headName = isWerewolf ? ESM::RefId::stringRefId("WerewolfHead") : mNpc->mHead;
        const ESM::RefId hairName = isWerewolf ? ESM::RefId::stringRefId("WerewolfHair") : mNpc->mHair;

        if (!headName.empty())
        {
            const ESM::BodyPart* bp = store.get<ESM::BodyPart>().search(headName);
            if (bp)
                mHeadModel = Misc::ResourceHelpers::correctMeshPath(bp->mModel);
            else
                Log(Debug::Warning) << "Warning: Failed to load body part '" << headName << "'";
        }

        if (!hairName.empty())
        {
            const ESM::BodyPart* bp = store.get<ESM::BodyPart>().search(hairName);
            if (bp)
                mHairModel = Misc::ResourceHelpers::correctMeshPath(bp->mModel);
            else
                Log(Debug::Warning) << "Warning: Failed to load body part '" << hairName << "'";
        }

        const std::string vampireHead = getVampireHead(mNpc->mRace, isFemale);
        if (!isWerewolf && isVampire && !vampireHead.empty())
            mHeadModel = vampireHead;

        bool is1stPerson = mViewMode == VM_FirstPerson;
        bool isBeast = (race->mData.mFlags & ESM::Race::Beast) != 0;

        std::string_view base;
        if (!isWerewolf)
        {
            if (!is1stPerson)
                base = Settings::models().mXbaseanim.get().value();
            else
                base = Settings::models().mXbaseanim1st.get().value();
        }

        const std::string defaultSkeleton = Misc::ResourceHelpers::correctActorModelPath(
            getActorSkeleton(is1stPerson, isFemale, isBeast, isWerewolf), mResourceSystem->getVFS());

        std::string smodel = defaultSkeleton;
        bool isCustomModel = false;
        if (!is1stPerson && !isWerewolf && !mNpc->mModel.empty())
        {
            std::string model = Misc::ResourceHelpers::correctMeshPath(mNpc->mModel);
            isCustomModel = !isDefaultActorSkeleton(model);
            smodel = Misc::ResourceHelpers::correctActorModelPath(model, mResourceSystem->getVFS());
        }

        setObjectRoot(smodel, true, true, false);

        updateParts();

        if (!base.empty())
            addAnimSource(base, smodel);

        if (defaultSkeleton != base)
            addAnimSource(defaultSkeleton, smodel);

        if (isCustomModel)
            addAnimSource(smodel, smodel);

        const bool customArgonianSwim = !is1stPerson && !isWerewolf && isBeast && mNpc->mRace.contains("argonian");
        if (customArgonianSwim)
            addAnimSource(Settings::models().mXargonianswimkna.get().value(), smodel);

        if (is1stPerson)
        {
            mObjectRoot->setNodeMask(Mask_FirstPerson);
            mObjectRoot->addCullCallback(new OverrideFieldOfViewCallback(mFirstPersonFieldOfView));
        }

        mWeaponAnimationTime->updateStartTime();
    }

    std::string NpcAnimation::getSheathedShieldMesh(const MWWorld::ConstPtr& shield) const
    {
        std::string mesh = getShieldMesh(shield, !mNpc->isMale());

        if (mesh.empty())
            return std::string();

        const VFS::Path::Normalized holsteredName(addSuffixBeforeExtension(mesh, "_sh"));
        if (mResourceSystem->getVFS()->exists(holsteredName))
        {
            osg::ref_ptr<osg::Node> shieldTemplate = mResourceSystem->getSceneManager()->getInstance(holsteredName);
            SceneUtil::FindByNameVisitor findVisitor("Bip01 Sheath");
            shieldTemplate->accept(findVisitor);
            osg::ref_ptr<osg::Node> sheathNode = findVisitor.mFoundNode;
            if (!sheathNode)
                return std::string();
        }

        return mesh;
    }

    void NpcAnimation::updateParts()
    {
        if (!mObjectRoot.get())
            return;

        NpcType curType = getNpcType();
        if (curType != mNpcType)
        {
            mNpcType = curType;
            rebuild();
            return;
        }

        static const struct
        {
            int mSlot;
            int mBasePriority;
        } slotlist[] = { // FIXME: Priority is based on the number of reserved slots. There should be a better way.
            { MWWorld::InventoryStore::Slot_Robe, 11 }, { MWWorld::InventoryStore::Slot_Skirt, 3 },
            { MWWorld::InventoryStore::Slot_Helmet, 0 }, { MWWorld::InventoryStore::Slot_Cuirass, 0 },
            { MWWorld::InventoryStore::Slot_Greaves, 0 }, { MWWorld::InventoryStore::Slot_LeftPauldron, 0 },
            { MWWorld::InventoryStore::Slot_RightPauldron, 0 }, { MWWorld::InventoryStore::Slot_Boots, 0 },
            { MWWorld::InventoryStore::Slot_LeftGauntlet, 0 }, { MWWorld::InventoryStore::Slot_RightGauntlet, 0 },
            { MWWorld::InventoryStore::Slot_Shirt, 0 }, { MWWorld::InventoryStore::Slot_Pants, 0 },
            { MWWorld::InventoryStore::Slot_CarriedLeft, 0 }, { MWWorld::InventoryStore::Slot_CarriedRight, 0 }
        };
        static const size_t slotlistsize = sizeof(slotlist) / sizeof(slotlist[0]);

        bool wasArrowAttached = isArrowAttached();
        mAmmunition.reset();

        const MWWorld::InventoryStore& inv = mPtr.getClass().getInventoryStore(mPtr);
        for (size_t i = 0; i < slotlistsize && mViewMode != VM_HeadOnly; i++)
        {
            MWWorld::ConstContainerStoreIterator store = inv.getSlot(slotlist[i].mSlot);

            removePartGroup(slotlist[i].mSlot);

            if (store == inv.end())
                continue;

            if (slotlist[i].mSlot == MWWorld::InventoryStore::Slot_Helmet)
                removeIndividualPart(ESM::PRT_Hair);

            int prio = 1;
            bool enchantedGlow = !store->getClass().getEnchantment(*store).empty();
            osg::Vec4f glowColor = store->getClass().getEnchantmentColor(*store);
            if (store->getType() == ESM::Clothing::sRecordId)
            {
                prio = ((slotlist[i].mBasePriority + 1) << 1) + 0;
                const ESM::Clothing* clothes = store->get<ESM::Clothing>()->mBase;
                addPartGroup(slotlist[i].mSlot, prio, clothes->mParts.mParts, enchantedGlow, &glowColor);
            }
            else if (store->getType() == ESM::Armor::sRecordId)
            {
                prio = ((slotlist[i].mBasePriority + 1) << 1) + 1;
                const ESM::Armor* armor = store->get<ESM::Armor>()->mBase;
                addPartGroup(slotlist[i].mSlot, prio, armor->mParts.mParts, enchantedGlow, &glowColor);
            }

            if (slotlist[i].mSlot == MWWorld::InventoryStore::Slot_Robe)
            {
                ESM::PartReferenceType parts[] = { ESM::PRT_Groin, ESM::PRT_Skirt, ESM::PRT_RLeg, ESM::PRT_LLeg,
                    ESM::PRT_RUpperarm, ESM::PRT_LUpperarm, ESM::PRT_RKnee, ESM::PRT_LKnee, ESM::PRT_RForearm,
                    ESM::PRT_LForearm, ESM::PRT_Cuirass };
                size_t parts_size = sizeof(parts) / sizeof(parts[0]);
                for (size_t p = 0; p < parts_size; ++p)
                    reserveIndividualPart(parts[p], slotlist[i].mSlot, prio);
            }
            else if (slotlist[i].mSlot == MWWorld::InventoryStore::Slot_Skirt)
            {
                reserveIndividualPart(ESM::PRT_Groin, slotlist[i].mSlot, prio);
                reserveIndividualPart(ESM::PRT_RLeg, slotlist[i].mSlot, prio);
                reserveIndividualPart(ESM::PRT_LLeg, slotlist[i].mSlot, prio);
            }
        }

        if (mViewMode != VM_FirstPerson)
        {
            if (mPartPriorities[ESM::PRT_Head] < 1 && !mHeadModel.empty())
                addOrReplaceIndividualPart(ESM::PRT_Head, -1, 1, mHeadModel);
            if (mPartPriorities[ESM::PRT_Hair] < 1 && mPartPriorities[ESM::PRT_Head] <= 1 && !mHairModel.empty())
                addOrReplaceIndividualPart(ESM::PRT_Hair, -1, 1, mHairModel);
        }
        if (mViewMode == VM_HeadOnly)
            return;

        if (mPartPriorities[ESM::PRT_Shield] < 1)
        {
            MWWorld::ConstContainerStoreIterator store = inv.getSlot(MWWorld::InventoryStore::Slot_CarriedLeft);
            MWWorld::ConstPtr part;
            if (store != inv.end() && (part = *store).getType() == ESM::Light::sRecordId)
            {
                const ESM::Light* light = part.get<ESM::Light>()->mBase;
                addOrReplaceIndividualPart(ESM::PRT_Shield, MWWorld::InventoryStore::Slot_CarriedLeft, 1,
                    VFS::Path::toNormalized(Misc::ResourceHelpers::correctMeshPath(light->mModel)), false, nullptr,
                    true);
                if (mObjectParts[ESM::PRT_Shield])
                    addExtraLight(mObjectParts[ESM::PRT_Shield]->getNode()->asGroup(), SceneUtil::LightCommon(*light));
            }
        }

        showWeapons(mShowWeapons);
        showCarriedLeft(mShowCarriedLeft);

        bool isWerewolf = (getNpcType() == Type_Werewolf);
        ESM::RefId race = (isWerewolf ? ESM::RefId::stringRefId("werewolf") : mNpc->mRace);

        const std::vector<const ESM::BodyPart*>& parts
            = getBodyParts(race, !mNpc->isMale(), mViewMode == VM_FirstPerson, isWerewolf);
        for (int part = ESM::PRT_Neck; part < ESM::PRT_Count; ++part)
        {
            if (mPartPriorities[part] < 1)
            {
                if (const ESM::BodyPart* bodypart = parts[part])
                    addOrReplaceIndividualPart(static_cast<ESM::PartReferenceType>(part), -1, 1,
                        VFS::Path::toNormalized(Misc::ResourceHelpers::correctMeshPath(bodypart->mModel)));
            }
        }

        if (wasArrowAttached)
            attachArrow();
    }

    PartHolderPtr NpcAnimation::insertBoundedPart(VFS::Path::NormalizedView model, std::string_view bonename,
        std::string_view bonefilter, bool enchantedGlow, osg::Vec4f* glowColor, bool isLight)
    {
        osg::ref_ptr<osg::Node> attached = attach(model, bonename, bonefilter, isLight);
        if (enchantedGlow)
            mGlowUpdater = SceneUtil::addEnchantedGlow(attached, mResourceSystem, *glowColor);

        return std::make_unique<PartHolder>(std::move(attached));
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

            mFirstPersonNeckController->setRotate(
                osg::Quat(mPtr.getRefData().getPosition().rot[0] * rotateFactor, osg::Vec3f(-1, 0, 0)));
            mFirstPersonNeckController->setOffset(mFirstPersonOffset);
        }

        WeaponAnimation::configureControllers(mPtr.getRefData().getPosition().rot[0] + getBodyPitchRadians());

        return ret;
    }

    void NpcAnimation::removeIndividualPart(ESM::PartReferenceType type)
    {
        mPartPriorities[type] = 0;
        mPartslots[type] = -1;

        mObjectParts[type].reset();
        if (mSounds[type] != nullptr && !mSoundsDisabled)
        {
            MWBase::Environment::get().getSoundManager()->stopSound(mSounds[type]);
            mSounds[type] = nullptr;
        }
    }

    void NpcAnimation::reserveIndividualPart(ESM::PartReferenceType type, int group, int priority)
    {
        if (priority > mPartPriorities[type])
        {
            removeIndividualPart(type);
            mPartPriorities[type] = priority;
            mPartslots[type] = group;
        }
    }

    void NpcAnimation::removePartGroup(int group)
    {
        for (int i = 0; i < ESM::PRT_Count; i++)
        {
            if (mPartslots[i] == group)
                removeIndividualPart((ESM::PartReferenceType)i);
        }
    }

    bool NpcAnimation::isFemalePart(const ESM::BodyPart* bodypart)
    {
        return bodypart->mData.mFlags & ESM::BodyPart::BPF_Female;
    }

    bool NpcAnimation::addOrReplaceIndividualPart(ESM::PartReferenceType type, int group, int priority,
        VFS::Path::NormalizedView mesh, bool enchantedGlow, osg::Vec4f* glowColor, bool isLight)
    {
        if (priority <= mPartPriorities[type])
            return false;

        removeIndividualPart(type);
        mPartslots[type] = group;
        mPartPriorities[type] = priority;
        try
        {
            std::string_view bonename = sPartList.at(type);
            if (type == ESM::PRT_Weapon)
            {
                const MWWorld::InventoryStore& inv = mPtr.getClass().getInventoryStore(mPtr);
                MWWorld::ConstContainerStoreIterator weapon = inv.getSlot(MWWorld::InventoryStore::Slot_CarriedRight);
                if (weapon != inv.end() && weapon->getType() == ESM::Weapon::sRecordId)
                {
                    int weaponType = weapon->get<ESM::Weapon>()->mBase->mData.mType;
                    const std::string& weaponBonename = MWMechanics::getWeaponType(weaponType)->mAttachBone;

                    if (weaponBonename != bonename)
                    {
                        const NodeMap& nodeMap = getNodeMap();
                        NodeMap::const_iterator found = nodeMap.find(weaponBonename);
                        if (found != nodeMap.end())
                            bonename = weaponBonename;
                    }
                }
            }

            // PRT_Hair seems to be the only type that breaks consistency and uses a filter that's different from the
            // attachment bone
            const std::string_view bonefilter = (type == ESM::PRT_Hair) ? std::string_view{ "hair" } : bonename;
            mObjectParts[type] = insertBoundedPart(mesh, bonename, bonefilter, enchantedGlow, glowColor, isLight);
        }
        catch (std::exception& e)
        {
            Log(Debug::Error) << "Error adding NPC part: " << e.what();
            return false;
        }

        if (!mSoundsDisabled && group == MWWorld::InventoryStore::Slot_CarriedLeft)
        {
            const MWWorld::InventoryStore& inv = mPtr.getClass().getInventoryStore(mPtr);
            MWWorld::ConstContainerStoreIterator csi = inv.getSlot(group);
            if (csi != inv.end())
            {
                const auto soundId = csi->getClass().getSound(*csi);
                if (!soundId.empty())
                {
                    mSounds[type] = MWBase::Environment::get().getSoundManager()->playSound3D(
                        mPtr, soundId, 1.0f, 1.0f, MWSound::Type::Sfx, MWSound::PlayMode::Loop);
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
                    for (unsigned int i = 0; i < node->getUserDataContainer()->getNumUserObjects(); ++i)
                    {
                        osg::Object* obj = node->getUserDataContainer()->getUserObject(i);
                        if (SceneUtil::TextKeyMapHolder* keys = dynamic_cast<SceneUtil::TextKeyMapHolder*>(obj))
                        {
                            for (const auto& key : keys->mTextKeys)
                            {
                                if (Misc::StringUtils::ciEqual(key.second, "talk: start"))
                                    mHeadAnimationTime->setTalkStart(key.first);
                                if (Misc::StringUtils::ciEqual(key.second, "talk: stop"))
                                    mHeadAnimationTime->setTalkStop(key.first);
                                if (Misc::StringUtils::ciEqual(key.second, "blink: start"))
                                    mHeadAnimationTime->setBlinkStart(key.first);
                                if (Misc::StringUtils::ciEqual(key.second, "blink: stop"))
                                    mHeadAnimationTime->setBlinkStop(key.first);
                            }

                            break;
                        }
                    }
                }
                SceneUtil::ForceControllerSourcesVisitor assignVisitor(std::move(src));
                node->accept(assignVisitor);
            }
            else
            {
                if (type == ESM::PRT_Weapon)
                    src = mWeaponAnimationTime;
                else
                    src = mAnimationTimePtr[0];
                SceneUtil::AssignControllerSourcesVisitor assignVisitor(std::move(src));
                node->accept(assignVisitor);
            }
        }

        return true;
    }

    void NpcAnimation::addPartGroup(int group, int priority, const std::vector<ESM::PartReference>& parts,
        bool enchantedGlow, osg::Vec4f* glowColor)
    {
        const MWWorld::ESMStore& store = *MWBase::Environment::get().getESMStore();
        const MWWorld::Store<ESM::BodyPart>& partStore = store.get<ESM::BodyPart>();

        const char* ext = (mViewMode == VM_FirstPerson) ? ".1st" : "";
        for (const ESM::PartReference& part : parts)
        {
            const ESM::BodyPart* bodypart = nullptr;
            if (!mNpc->isMale() && !part.mFemale.empty())
            {
                bodypart = partStore.search(ESM::RefId::stringRefId(part.mFemale.getRefIdString() + ext));
                if (!bodypart && mViewMode == VM_FirstPerson)
                {
                    bodypart = partStore.search(part.mFemale);
                    if (bodypart
                        && !(bodypart->mData.mPart == ESM::BodyPart::MP_Hand
                            || bodypart->mData.mPart == ESM::BodyPart::MP_Wrist
                            || bodypart->mData.mPart == ESM::BodyPart::MP_Forearm
                            || bodypart->mData.mPart == ESM::BodyPart::MP_Upperarm))
                        bodypart = nullptr;
                }
                else if (!bodypart)
                    Log(Debug::Warning) << "Warning: Failed to find body part '" << part.mFemale << "'";
            }
            if (!bodypart && !part.mMale.empty())
            {
                bodypart = partStore.search(ESM::RefId::stringRefId(part.mMale.getRefIdString() + ext));
                if (!bodypart && mViewMode == VM_FirstPerson)
                {
                    bodypart = partStore.search(part.mMale);
                    if (bodypart
                        && !(bodypart->mData.mPart == ESM::BodyPart::MP_Hand
                            || bodypart->mData.mPart == ESM::BodyPart::MP_Wrist
                            || bodypart->mData.mPart == ESM::BodyPart::MP_Forearm
                            || bodypart->mData.mPart == ESM::BodyPart::MP_Upperarm))
                        bodypart = nullptr;
                }
                else if (!bodypart)
                    Log(Debug::Warning) << "Warning: Failed to find body part '" << part.mMale << "'";
            }

            if (bodypart)
                addOrReplaceIndividualPart(static_cast<ESM::PartReferenceType>(part.mPart), group, priority,
                    VFS::Path::toNormalized(Misc::ResourceHelpers::correctMeshPath(bodypart->mModel)), enchantedGlow,
                    glowColor);
            else
                reserveIndividualPart((ESM::PartReferenceType)part.mPart, group, priority);
        }
    }

    void NpcAnimation::addControllers()
    {
        Animation::addControllers();

        mFirstPersonNeckController = nullptr;
        WeaponAnimation::deleteControllers();

        if (mViewMode == VM_FirstPerson)
        {
            // If there is no active animation, then the bip01 neck node will not be updated each frame, and the
            // RotateController will accumulate rotations.
            if (mStates.size() > 0)
            {
                NodeMap::iterator found = mNodeMap.find("bip01 neck");
                if (found != mNodeMap.end())
                {
                    osg::MatrixTransform* node = found->second.get();
                    mFirstPersonNeckController = new RotateController(mObjectRoot.get());
                    node->addUpdateCallback(mFirstPersonNeckController);
                    mActiveControllers.emplace_back(node, mFirstPersonNeckController);
                }
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
        if (showWeapon)
        {
            const MWWorld::InventoryStore& inv = mPtr.getClass().getInventoryStore(mPtr);
            MWWorld::ConstContainerStoreIterator weapon = inv.getSlot(MWWorld::InventoryStore::Slot_CarriedRight);
            if (weapon != inv.end())
            {
                osg::Vec4f glowColor = weapon->getClass().getEnchantmentColor(*weapon);
                const VFS::Path::Normalized mesh = weapon->getClass().getCorrectedModel(*weapon);
                addOrReplaceIndividualPart(ESM::PRT_Weapon, MWWorld::InventoryStore::Slot_CarriedRight, 1, mesh,
                    !weapon->getClass().getEnchantment(*weapon).empty(), &glowColor);

                // Crossbows start out with a bolt attached
                if (weapon->getType() == ESM::Weapon::sRecordId
                    && weapon->get<ESM::Weapon>()->mBase->mData.mType == ESM::Weapon::MarksmanCrossbow)
                {
                    int ammotype = MWMechanics::getWeaponType(ESM::Weapon::MarksmanCrossbow)->mAmmoType;
                    MWWorld::ConstContainerStoreIterator ammo = inv.getSlot(MWWorld::InventoryStore::Slot_Ammunition);
                    if (ammo != inv.end() && ammo->get<ESM::Weapon>()->mBase->mData.mType == ammotype)
                        attachArrow();
                }
            }
        }
        else
        {
            removeIndividualPart(ESM::PRT_Weapon);
            // If we remove/hide weapon from player, we should reset attack animation as well
            if (mPtr == MWMechanics::getPlayer())
                mPtr.getClass().getCreatureStats(mPtr).setAttackingOrSpell(false);
        }

        updateHolsteredWeapon(!mShowWeapons);
        updateQuiver();
    }

    bool NpcAnimation::updateCarriedLeftVisible(const int weaptype) const
    {
        if (Settings::game().mShieldSheathing)
        {
            const MWWorld::Class& cls = mPtr.getClass();
            MWMechanics::CreatureStats& stats = cls.getCreatureStats(mPtr);
            if (stats.getDrawState() == MWMechanics::DrawState::Nothing)
            {
                SceneUtil::FindByNameVisitor findVisitor("Bip01 AttachShield");
                mObjectRoot->accept(findVisitor);
                if (findVisitor.mFoundNode || mViewMode == VM_FirstPerson)
                {
                    const MWWorld::InventoryStore& inv = cls.getInventoryStore(mPtr);
                    const MWWorld::ConstContainerStoreIterator shield
                        = inv.getSlot(MWWorld::InventoryStore::Slot_CarriedLeft);
                    if (shield != inv.end() && shield->getType() == ESM::Armor::sRecordId
                        && !getSheathedShieldMesh(*shield).empty())
                        return false;
                }
            }
        }

        return !(MWMechanics::getWeaponType(weaptype)->mFlags & ESM::WeaponType::TwoHanded);
    }

    void NpcAnimation::showCarriedLeft(bool show)
    {
        mShowCarriedLeft = show;
        const MWWorld::InventoryStore& inv = mPtr.getClass().getInventoryStore(mPtr);
        MWWorld::ConstContainerStoreIterator iter = inv.getSlot(MWWorld::InventoryStore::Slot_CarriedLeft);
        if (show && iter != inv.end())
        {
            osg::Vec4f glowColor = iter->getClass().getEnchantmentColor(*iter);
            VFS::Path::Normalized mesh = iter->getClass().getCorrectedModel(*iter);
            // For shields we must try to use the body part model
            if (iter->getType() == ESM::Armor::sRecordId)
            {
                mesh = getShieldMesh(*iter, !mNpc->isMale());
            }
            if (mesh.empty()
                || addOrReplaceIndividualPart(ESM::PRT_Shield, MWWorld::InventoryStore::Slot_CarriedLeft, 1, mesh,
                    !iter->getClass().getEnchantment(*iter).empty(), &glowColor,
                    iter->getType() == ESM::Light::sRecordId))
            {
                if (mesh.empty())
                    reserveIndividualPart(ESM::PRT_Shield, MWWorld::InventoryStore::Slot_CarriedLeft, 1);
                if (iter->getType() == ESM::Light::sRecordId && mObjectParts[ESM::PRT_Shield])
                    addExtraLight(mObjectParts[ESM::PRT_Shield]->getNode()->asGroup(),
                        SceneUtil::LightCommon(*iter->get<ESM::Light>()->mBase));
            }
        }
        else
            removeIndividualPart(ESM::PRT_Shield);

        updateHolsteredShield(mShowCarriedLeft);
    }

    void NpcAnimation::attachArrow()
    {
        WeaponAnimation::attachArrow(mPtr);

        const MWWorld::InventoryStore& inv = mPtr.getClass().getInventoryStore(mPtr);
        MWWorld::ConstContainerStoreIterator ammo = inv.getSlot(MWWorld::InventoryStore::Slot_Ammunition);
        if (ammo != inv.end() && !ammo->getClass().getEnchantment(*ammo).empty())
        {
            osg::Group* bone = getArrowBone();
            if (bone != nullptr && bone->getNumChildren())
                SceneUtil::addEnchantedGlow(
                    bone->getChild(0), mResourceSystem, ammo->getClass().getEnchantmentColor(*ammo));
        }

        updateQuiver();
    }

    void NpcAnimation::detachArrow()
    {
        WeaponAnimation::detachArrow(mPtr);
        updateQuiver();
    }

    void NpcAnimation::releaseArrow(float attackStrength)
    {
        WeaponAnimation::releaseArrow(mPtr, attackStrength);
        updateQuiver();
    }

    osg::Group* NpcAnimation::getArrowBone()
    {
        const PartHolder* const part = mObjectParts[ESM::PRT_Weapon].get();
        if (part == nullptr)
            return nullptr;

        const MWWorld::InventoryStore& inv = mPtr.getClass().getInventoryStore(mPtr);
        MWWorld::ConstContainerStoreIterator weapon = inv.getSlot(MWWorld::InventoryStore::Slot_CarriedRight);
        if (weapon == inv.end() || weapon->getType() != ESM::Weapon::sRecordId)
            return nullptr;

        int type = weapon->get<ESM::Weapon>()->mBase->mData.mType;
        int ammoType = MWMechanics::getWeaponType(type)->mAmmoType;
        if (ammoType == ESM::Weapon::None)
            return nullptr;

        // Try to find and attachment bone in actor's skeleton, otherwise fall back to the ArrowBone in weapon's mesh
        osg::Group* bone = getBoneByName(MWMechanics::getWeaponType(ammoType)->mAttachBone);
        if (bone == nullptr)
        {
            SceneUtil::FindByNameVisitor findVisitor("ArrowBone");
            part->getNode()->accept(findVisitor);
            bone = findVisitor.mFoundNode;
        }
        return bone;
    }

    osg::Node* NpcAnimation::getWeaponNode()
    {
        const PartHolder* const part = mObjectParts[ESM::PRT_Weapon].get();
        if (part == nullptr)
            return nullptr;
        return part->getNode();
    }

    Resource::ResourceSystem* NpcAnimation::getResourceSystem()
    {
        return mResourceSystem;
    }

    void NpcAnimation::enableHeadAnimation(bool enable)
    {
        mHeadAnimationTime->setEnabled(enable);
    }

    void NpcAnimation::setWeaponGroup(const std::string& group, bool relativeDuration)
    {
        mWeaponAnimationTime->setGroup(group, relativeDuration);
    }

    void NpcAnimation::equipmentChanged()
    {
        if (Settings::game().mShieldSheathing)
        {
            int weaptype = ESM::Weapon::None;
            MWMechanics::getActiveWeapon(mPtr, &weaptype);
            showCarriedLeft(updateCarriedLeftVisible(weaptype));
        }

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

    void NpcAnimation::setFirstPersonOffset(const osg::Vec3f& offset)
    {
        mFirstPersonOffset = offset;
    }

    void NpcAnimation::updatePtr(const MWWorld::Ptr& updated)
    {
        Animation::updatePtr(updated);
        mHeadAnimationTime->updatePtr(updated);
    }

    // Remember body parts so we only have to search through the store once for each race/gender/viewmode combination
    typedef std::map<std::pair<ESM::RefId, int>, std::vector<const ESM::BodyPart*>> RaceMapping;
    static RaceMapping sRaceMapping;

    const std::vector<const ESM::BodyPart*>& NpcAnimation::getBodyParts(
        const ESM::RefId& race, bool female, bool firstPerson, bool werewolf)
    {
        static const int Flag_FirstPerson = 1 << 1;
        static const int Flag_Female = 1 << 0;

        int flags = (werewolf ? -1 : 0);
        if (female)
            flags |= Flag_Female;
        if (firstPerson)
            flags |= Flag_FirstPerson;

        RaceMapping::iterator found = sRaceMapping.find(std::make_pair(race, flags));
        if (found != sRaceMapping.end())
            return found->second;
        else
        {
            std::vector<const ESM::BodyPart*>& parts = sRaceMapping[std::make_pair(race, flags)];

            typedef std::multimap<ESM::BodyPart::MeshPart, ESM::PartReferenceType> BodyPartMapType;
            static const BodyPartMapType sBodyPartMap = { { ESM::BodyPart::MP_Neck, ESM::PRT_Neck },
                { ESM::BodyPart::MP_Chest, ESM::PRT_Cuirass }, { ESM::BodyPart::MP_Groin, ESM::PRT_Groin },
                { ESM::BodyPart::MP_Hand, ESM::PRT_RHand }, { ESM::BodyPart::MP_Hand, ESM::PRT_LHand },
                { ESM::BodyPart::MP_Wrist, ESM::PRT_RWrist }, { ESM::BodyPart::MP_Wrist, ESM::PRT_LWrist },
                { ESM::BodyPart::MP_Forearm, ESM::PRT_RForearm }, { ESM::BodyPart::MP_Forearm, ESM::PRT_LForearm },
                { ESM::BodyPart::MP_Upperarm, ESM::PRT_RUpperarm }, { ESM::BodyPart::MP_Upperarm, ESM::PRT_LUpperarm },
                { ESM::BodyPart::MP_Foot, ESM::PRT_RFoot }, { ESM::BodyPart::MP_Foot, ESM::PRT_LFoot },
                { ESM::BodyPart::MP_Ankle, ESM::PRT_RAnkle }, { ESM::BodyPart::MP_Ankle, ESM::PRT_LAnkle },
                { ESM::BodyPart::MP_Knee, ESM::PRT_RKnee }, { ESM::BodyPart::MP_Knee, ESM::PRT_LKnee },
                { ESM::BodyPart::MP_Upperleg, ESM::PRT_RLeg }, { ESM::BodyPart::MP_Upperleg, ESM::PRT_LLeg },
                { ESM::BodyPart::MP_Tail, ESM::PRT_Tail } };

            parts.resize(ESM::PRT_Count, nullptr);

            if (werewolf)
                return parts;

            const MWWorld::ESMStore& store = *MWBase::Environment::get().getESMStore();

            for (const ESM::BodyPart& bodypart : store.get<ESM::BodyPart>())
            {
                if (bodypart.mData.mFlags & ESM::BodyPart::BPF_NotPlayable)
                    continue;
                if (bodypart.mData.mType != ESM::BodyPart::MT_Skin)
                    continue;

                if (!(bodypart.mRace == race))
                    continue;

                const bool partFirstPerson = ESM::isFirstPersonBodyPart(bodypart);

                bool isHand = bodypart.mData.mPart == ESM::BodyPart::MP_Hand
                    || bodypart.mData.mPart == ESM::BodyPart::MP_Wrist
                    || bodypart.mData.mPart == ESM::BodyPart::MP_Forearm
                    || bodypart.mData.mPart == ESM::BodyPart::MP_Upperarm;

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
                    BodyPartMapType::const_iterator bIt
                        = sBodyPartMap.lower_bound(BodyPartMapType::key_type(bodypart.mData.mPart));
                    while (bIt != sBodyPartMap.end() && bIt->first == bodypart.mData.mPart)
                    {
                        // If we have no fallback bodypart now and bodypart is for same gender (1)
                        if (!parts[bIt->second] && isSameGender)
                            parts[bIt->second] = &bodypart;

                        // If we have fallback bodypart for other gender and found fallback for current gender (1)
                        else if (isSameGender && isFemalePart(parts[bIt->second]) != female)
                            parts[bIt->second] = &bodypart;

                        // If we have no fallback bodypart and searching for female bodyparts (3)
                        else if (!parts[bIt->second] && female)
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
                    BodyPartMapType::const_iterator bIt
                        = sBodyPartMap.lower_bound(BodyPartMapType::key_type(bodypart.mData.mPart));
                    while (bIt != sBodyPartMap.end() && bIt->first == bodypart.mData.mPart)
                    {
                        // If we have no fallback bodypart now
                        if (!parts[bIt->second])
                            parts[bIt->second] = &bodypart;

                        // If we have 3d person fallback bodypart for hand and 1st person fallback found (2)
                        else if (isHand && !ESM::isFirstPersonBodyPart(*parts[bIt->second]) && partFirstPerson)
                            parts[bIt->second] = &bodypart;

                        ++bIt;
                    }

                    continue;
                }

                // Don't allow to use podyparts for another gender
                if (female != isFemalePart(&bodypart))
                    continue;

                // Use properly found bodypart, replacing fallbacks
                BodyPartMapType::const_iterator bIt
                    = sBodyPartMap.lower_bound(BodyPartMapType::key_type(bodypart.mData.mPart));
                while (bIt != sBodyPartMap.end() && bIt->first == bodypart.mData.mPart)
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

    bool NpcAnimation::isArrowAttached() const
    {
        return mAmmunition != nullptr;
    }

}

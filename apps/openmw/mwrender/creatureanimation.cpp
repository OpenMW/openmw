#include "creatureanimation.hpp"

#include <osg/MatrixTransform>

#include <components/debug/debuglog.hpp>
#include <components/esm3/loadcrea.hpp>
#include <components/resource/resourcesystem.hpp>
#include <components/sceneutil/lightcommon.hpp>
#include <components/sceneutil/positionattitudetransform.hpp>
#include <components/sceneutil/visitor.hpp>
#include <components/settings/values.hpp>

#include "../mwmechanics/weapontype.hpp"

#include "../mwworld/class.hpp"

namespace MWRender
{

    CreatureAnimation::CreatureAnimation(
        const MWWorld::Ptr& ptr, const std::string& model, Resource::ResourceSystem* resourceSystem, bool animated)
        : ActorAnimation(ptr, osg::ref_ptr<osg::Group>(ptr.getRefData().getBaseNode()), resourceSystem)
    {
        MWWorld::LiveCellRef<ESM::Creature>* ref = mPtr.get<ESM::Creature>();

        if (!model.empty())
        {
            setObjectRoot(model, false, false, true);

            if ((ref->mBase->mFlags & ESM::Creature::Bipedal))
                addAnimSource(Settings::models().mXbaseanim.get(), model);

            if (animated)
                addAnimSource(model, model);
        }
    }

    CreatureWeaponAnimation::CreatureWeaponAnimation(
        const MWWorld::Ptr& ptr, const std::string& model, Resource::ResourceSystem* resourceSystem, bool animated)
        : ActorAnimation(ptr, osg::ref_ptr<osg::Group>(ptr.getRefData().getBaseNode()), resourceSystem)
        , mShowWeapons(false)
        , mShowCarriedLeft(false)
    {
        MWWorld::LiveCellRef<ESM::Creature>* ref = mPtr.get<ESM::Creature>();

        if (!model.empty())
        {
            setObjectRoot(model, true, false, true);

            if ((ref->mBase->mFlags & ESM::Creature::Bipedal))
                addAnimSource(Settings::models().mXbaseanim.get(), model);

            if (animated)
                addAnimSource(model, model);

            mPtr.getClass().getInventoryStore(mPtr).setInvListener(this);

            updateParts();
        }

        mWeaponAnimationTime = std::make_shared<WeaponAnimationTime>(this);
    }

    void CreatureWeaponAnimation::showWeapons(bool showWeapon)
    {
        if (showWeapon != mShowWeapons)
        {
            mShowWeapons = showWeapon;
            updateParts();
        }
    }

    void CreatureWeaponAnimation::showCarriedLeft(bool show)
    {
        if (show != mShowCarriedLeft)
        {
            mShowCarriedLeft = show;
            updateParts();
        }
    }

    void CreatureWeaponAnimation::updateParts()
    {
        mAmmunition.reset();
        mWeapon.reset();
        mShield.reset();

        updateHolsteredWeapon(!mShowWeapons);
        updateQuiver();
        updateHolsteredShield(mShowCarriedLeft);

        if (mShowWeapons)
            updatePart(mWeapon, MWWorld::InventoryStore::Slot_CarriedRight);
        if (mShowCarriedLeft)
            updatePart(mShield, MWWorld::InventoryStore::Slot_CarriedLeft);
    }

    void CreatureWeaponAnimation::updatePart(PartHolderPtr& scene, int slot)
    {
        if (!mObjectRoot)
            return;

        const MWWorld::InventoryStore& inv = mPtr.getClass().getInventoryStore(mPtr);
        MWWorld::ConstContainerStoreIterator it = inv.getSlot(slot);

        if (it == inv.end())
        {
            scene.reset();
            return;
        }
        MWWorld::ConstPtr item = *it;

        std::string_view bonename;
        VFS::Path::Normalized itemModel = item.getClass().getCorrectedModel(item);
        if (slot == MWWorld::InventoryStore::Slot_CarriedRight)
        {
            if (item.getType() == ESM::Weapon::sRecordId)
            {
                int type = item.get<ESM::Weapon>()->mBase->mData.mType;
                bonename = MWMechanics::getWeaponType(type)->mAttachBone;
                if (bonename != "Weapon Bone")
                {
                    const NodeMap& nodeMap = getNodeMap();
                    NodeMap::const_iterator found = nodeMap.find(bonename);
                    if (found == nodeMap.end())
                        bonename = "Weapon Bone";
                }
            }
            else
                bonename = "Weapon Bone";
        }
        else
        {
            bonename = "Shield Bone";
            if (item.getType() == ESM::Armor::sRecordId)
            {
                itemModel = getShieldMesh(item, false);
            }
        }

        try
        {
            osg::ref_ptr<osg::Node> attached
                = attach(itemModel, bonename, bonename, item.getType() == ESM::Light::sRecordId);

            scene = std::make_unique<PartHolder>(attached);

            if (!item.getClass().getEnchantment(item).empty())
                mGlowUpdater
                    = SceneUtil::addEnchantedGlow(attached, mResourceSystem, item.getClass().getEnchantmentColor(item));

            // Crossbows start out with a bolt attached
            // FIXME: code duplicated from NpcAnimation
            if (slot == MWWorld::InventoryStore::Slot_CarriedRight && item.getType() == ESM::Weapon::sRecordId
                && item.get<ESM::Weapon>()->mBase->mData.mType == ESM::Weapon::MarksmanCrossbow)
            {
                const ESM::WeaponType* weaponInfo = MWMechanics::getWeaponType(ESM::Weapon::MarksmanCrossbow);
                MWWorld::ConstContainerStoreIterator ammo = inv.getSlot(MWWorld::InventoryStore::Slot_Ammunition);
                if (ammo != inv.end() && ammo->get<ESM::Weapon>()->mBase->mData.mType == weaponInfo->mAmmoType)
                    attachArrow();
                else
                    mAmmunition.reset();
            }
            else
                mAmmunition.reset();

            std::shared_ptr<SceneUtil::ControllerSource> source;

            if (slot == MWWorld::InventoryStore::Slot_CarriedRight)
                source = mWeaponAnimationTime;
            else
                source = mAnimationTimePtr[0];

            SceneUtil::AssignControllerSourcesVisitor assignVisitor(std::move(source));
            attached->accept(assignVisitor);

            if (item.getType() == ESM::Light::sRecordId)
                addExtraLight(scene->getNode()->asGroup(), SceneUtil::LightCommon(*item.get<ESM::Light>()->mBase));
        }
        catch (std::exception& e)
        {
            Log(Debug::Error) << "Can not add creature part: " << e.what();
        }
    }

    bool CreatureWeaponAnimation::isArrowAttached() const
    {
        return mAmmunition != nullptr;
    }

    void CreatureWeaponAnimation::detachArrow()
    {
        WeaponAnimation::detachArrow(mPtr);
        updateQuiver();
    }

    void CreatureWeaponAnimation::attachArrow()
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

    void CreatureWeaponAnimation::releaseArrow(float attackStrength)
    {
        WeaponAnimation::releaseArrow(mPtr, attackStrength);
        updateQuiver();
    }

    osg::Group* CreatureWeaponAnimation::getArrowBone()
    {
        if (!mWeapon)
            return nullptr;

        if (!mPtr.getClass().hasInventoryStore(mPtr))
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
            mWeapon->getNode()->accept(findVisitor);
            bone = findVisitor.mFoundNode;
        }
        return bone;
    }

    osg::Node* CreatureWeaponAnimation::getWeaponNode()
    {
        return mWeapon ? mWeapon->getNode().get() : nullptr;
    }

    Resource::ResourceSystem* CreatureWeaponAnimation::getResourceSystem()
    {
        return mResourceSystem;
    }

    void CreatureWeaponAnimation::addControllers()
    {
        Animation::addControllers();
        if (mObjectRoot)
            WeaponAnimation::addControllers(mNodeMap, mActiveControllers, mObjectRoot.get());
    }

    osg::Vec3f CreatureWeaponAnimation::runAnimation(float duration)
    {
        osg::Vec3f ret = Animation::runAnimation(duration);

        WeaponAnimation::configureControllers(mPtr.getRefData().getPosition().rot[0] + getBodyPitchRadians());

        return ret;
    }

}

#include "actoranimation.hpp"
#include <utility>

#include <osg/Group>
#include <osg/Node>
#include <osg/Vec4f>

#include <components/esm3/loadbody.hpp>
#include <components/esm3/loadcell.hpp>
#include <components/esm3/loadligh.hpp>

#include <components/resource/resourcesystem.hpp>
#include <components/resource/scenemanager.hpp>

#include <components/sceneutil/attach.hpp>
#include <components/sceneutil/lightcommon.hpp>
#include <components/sceneutil/lightmanager.hpp>
#include <components/sceneutil/lightutil.hpp>
#include <components/sceneutil/visitor.hpp>

#include <components/misc/resourcehelpers.hpp>

#include <components/settings/values.hpp>

#include <components/vfs/manager.hpp>

#include "../mwbase/environment.hpp"
#include "../mwmechanics/actorutil.hpp"
#include "../mwmechanics/creaturestats.hpp"
#include "../mwmechanics/drawstate.hpp"
#include "../mwmechanics/weapontype.hpp"
#include "../mwworld/cellstore.hpp"
#include "../mwworld/class.hpp"
#include "../mwworld/esmstore.hpp"
#include "../mwworld/inventorystore.hpp"
#include "../mwworld/ptr.hpp"

#include "actorutil.hpp"
#include "vismask.hpp"

namespace MWRender
{

    ActorAnimation::ActorAnimation(
        const MWWorld::Ptr& ptr, osg::ref_ptr<osg::Group> parentNode, Resource::ResourceSystem* resourceSystem)
        : Animation(ptr, std::move(parentNode), resourceSystem)
    {
        MWWorld::ContainerStore& store = mPtr.getClass().getContainerStore(mPtr);

        for (MWWorld::ConstContainerStoreIterator iter = store.cbegin(MWWorld::ContainerStore::Type_Light);
             iter != store.cend(); ++iter)
        {
            const ESM::Light* light = iter->get<ESM::Light>()->mBase;
            if (!(light->mData.mFlags & ESM::Light::Carry))
            {
                addHiddenItemLight(*iter, light);
            }
        }

        // Make sure we cleaned object from effects, just in cast if we re-use node
        removeEffects();
    }

    ActorAnimation::~ActorAnimation()
    {
        removeFromSceneImpl();
    }

    PartHolderPtr ActorAnimation::attachMesh(
        const std::string& model, std::string_view bonename, bool enchantedGlow, osg::Vec4f* glowColor)
    {
        osg::Group* parent = getBoneByName(bonename);
        if (!parent)
            return nullptr;

        osg::ref_ptr<osg::Node> instance
            = mResourceSystem->getSceneManager()->getInstance(VFS::Path::toNormalized(model), parent);

        const NodeMap& nodeMap = getNodeMap();
        NodeMap::const_iterator found = nodeMap.find(bonename);
        if (found == nodeMap.end())
            return {};

        if (enchantedGlow)
            mGlowUpdater = SceneUtil::addEnchantedGlow(instance, mResourceSystem, *glowColor);

        return std::make_unique<PartHolder>(instance);
    }

    osg::ref_ptr<osg::Node> ActorAnimation::attach(
        const std::string& model, std::string_view bonename, std::string_view bonefilter, bool isLight)
    {
        osg::ref_ptr<const osg::Node> templateNode
            = mResourceSystem->getSceneManager()->getTemplate(VFS::Path::toNormalized(model));

        const NodeMap& nodeMap = getNodeMap();
        auto found = nodeMap.find(bonename);
        if (found == nodeMap.end())
            throw std::runtime_error("Can't find attachment node " + std::string{ bonename });
        if (isLight)
        {
            osg::Quat rotation(osg::DegreesToRadians(-90.f), osg::Vec3f(1, 0, 0));
            return SceneUtil::attach(
                templateNode, mObjectRoot, bonefilter, found->second, mResourceSystem->getSceneManager(), &rotation);
        }
        return SceneUtil::attach(
            std::move(templateNode), mObjectRoot, bonefilter, found->second, mResourceSystem->getSceneManager());
    }

    std::string ActorAnimation::getShieldMesh(const MWWorld::ConstPtr& shield, bool female) const
    {
        const ESM::Armor* armor = shield.get<ESM::Armor>()->mBase;
        const std::vector<ESM::PartReference>& bodyparts = armor->mParts.mParts;
        // Try to recover the body part model, use ground model as a fallback otherwise.
        if (!bodyparts.empty())
        {
            const MWWorld::ESMStore& store = *MWBase::Environment::get().getESMStore();
            const MWWorld::Store<ESM::BodyPart>& partStore = store.get<ESM::BodyPart>();
            for (const auto& part : bodyparts)
            {
                if (part.mPart != ESM::PRT_Shield)
                    continue;

                const ESM::RefId* bodypartName = nullptr;
                if (female && !part.mFemale.empty())
                    bodypartName = &part.mFemale;
                else if (!part.mMale.empty())
                    bodypartName = &part.mMale;

                if (bodypartName && !bodypartName->empty())
                {
                    const ESM::BodyPart* bodypart = partStore.search(*bodypartName);
                    if (bodypart == nullptr || bodypart->mData.mType != ESM::BodyPart::MT_Armor)
                        return std::string();
                    if (!bodypart->mModel.empty())
                        return Misc::ResourceHelpers::correctMeshPath(bodypart->mModel);
                }
            }
        }
        return shield.getClass().getCorrectedModel(shield);
    }

    std::string ActorAnimation::getSheathedShieldMesh(const MWWorld::ConstPtr& shield) const
    {
        std::string mesh = getShieldMesh(shield, false);

        if (mesh.empty())
            return mesh;

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

    bool ActorAnimation::updateCarriedLeftVisible(const int weaptype) const
    {
        if (Settings::game().mShieldSheathing)
        {
            const MWWorld::Class& cls = mPtr.getClass();
            MWMechanics::CreatureStats& stats = cls.getCreatureStats(mPtr);
            if (cls.hasInventoryStore(mPtr) && stats.getDrawState() == MWMechanics::DrawState::Nothing)
            {
                SceneUtil::FindByNameVisitor findVisitor("Bip01 AttachShield");
                mObjectRoot->accept(findVisitor);
                if (findVisitor.mFoundNode)
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

    void ActorAnimation::updateHolsteredShield(bool showCarriedLeft)
    {
        if (!Settings::game().mShieldSheathing)
            return;

        if (!mPtr.getClass().hasInventoryStore(mPtr))
            return;

        mHolsteredShield.reset();

        if (showCarriedLeft)
            return;

        const MWWorld::InventoryStore& inv = mPtr.getClass().getInventoryStore(mPtr);
        MWWorld::ConstContainerStoreIterator shield = inv.getSlot(MWWorld::InventoryStore::Slot_CarriedLeft);
        if (shield == inv.end() || shield->getType() != ESM::Armor::sRecordId)
            return;

        // Can not show holdstered shields with two-handed weapons at all
        const MWWorld::ConstContainerStoreIterator weapon = inv.getSlot(MWWorld::InventoryStore::Slot_CarriedRight);
        if (weapon == inv.end())
            return;

        auto type = weapon->getType();
        if (type == ESM::Weapon::sRecordId)
        {
            const MWWorld::LiveCellRef<ESM::Weapon>* ref = weapon->get<ESM::Weapon>();
            ESM::Weapon::Type weaponType = (ESM::Weapon::Type)ref->mBase->mData.mType;
            if (MWMechanics::getWeaponType(weaponType)->mFlags & ESM::WeaponType::TwoHanded)
                return;
        }

        std::string mesh = getSheathedShieldMesh(*shield);
        if (mesh.empty())
            return;

        std::string_view boneName = "Bip01 AttachShield";
        osg::Vec4f glowColor = shield->getClass().getEnchantmentColor(*shield);
        const std::string holsteredName = addSuffixBeforeExtension(mesh, "_sh");
        bool isEnchanted = !shield->getClass().getEnchantment(*shield).empty();

        // If we have no dedicated sheath model, use basic shield model as fallback.
        if (!mResourceSystem->getVFS()->exists(holsteredName))
            mHolsteredShield = attachMesh(mesh, boneName, isEnchanted, &glowColor);
        else
            mHolsteredShield = attachMesh(holsteredName, boneName, isEnchanted, &glowColor);

        if (!mHolsteredShield)
            return;

        SceneUtil::FindByNameVisitor findVisitor("Bip01 Sheath");
        mHolsteredShield->getNode()->accept(findVisitor);
        osg::Group* shieldNode = findVisitor.mFoundNode;

        // If mesh author declared an empty sheath node, use transformation from this node, but use the common shield
        // mesh. This approach allows to tweak shield position without need to store the whole shield mesh in the _sh
        // file.
        if (shieldNode && !shieldNode->getNumChildren())
        {
            osg::ref_ptr<osg::Node> fallbackNode
                = mResourceSystem->getSceneManager()->getInstance(VFS::Path::toNormalized(mesh), shieldNode);
            if (isEnchanted)
                SceneUtil::addEnchantedGlow(shieldNode, mResourceSystem, glowColor);
        }
    }

    bool ActorAnimation::useShieldAnimations() const
    {
        if (!Settings::game().mShieldSheathing)
            return false;

        const MWWorld::Class& cls = mPtr.getClass();
        if (!cls.hasInventoryStore(mPtr))
            return false;

        if (getTextKeyTime("shield: equip attach") < 0 || getTextKeyTime("shield: unequip detach") < 0)
            return false;

        const MWWorld::InventoryStore& inv = cls.getInventoryStore(mPtr);
        const MWWorld::ConstContainerStoreIterator weapon = inv.getSlot(MWWorld::InventoryStore::Slot_CarriedRight);
        const MWWorld::ConstContainerStoreIterator shield = inv.getSlot(MWWorld::InventoryStore::Slot_CarriedLeft);
        if (weapon != inv.end() && shield != inv.end() && shield->getType() == ESM::Armor::sRecordId
            && !getSheathedShieldMesh(*shield).empty())
        {
            auto type = weapon->getType();
            if (type == ESM::Weapon::sRecordId)
            {
                const MWWorld::LiveCellRef<ESM::Weapon>* ref = weapon->get<ESM::Weapon>();
                ESM::Weapon::Type weaponType = (ESM::Weapon::Type)ref->mBase->mData.mType;
                return !(MWMechanics::getWeaponType(weaponType)->mFlags & ESM::WeaponType::TwoHanded);
            }
            else if (type == ESM::Lockpick::sRecordId || type == ESM::Probe::sRecordId)
                return true;
        }

        return false;
    }

    osg::Group* ActorAnimation::getBoneByName(std::string_view boneName) const
    {
        if (!mObjectRoot)
            return nullptr;

        SceneUtil::FindByNameVisitor findVisitor(boneName);
        mObjectRoot->accept(findVisitor);

        return findVisitor.mFoundNode;
    }

    std::string_view ActorAnimation::getHolsteredWeaponBoneName(const MWWorld::ConstPtr& weapon)
    {
        if (weapon.isEmpty())
            return {};

        auto type = weapon.getClass().getType();
        if (type == ESM::Weapon::sRecordId)
        {
            const MWWorld::LiveCellRef<ESM::Weapon>* ref = weapon.get<ESM::Weapon>();
            int weaponType = ref->mBase->mData.mType;
            return MWMechanics::getWeaponType(weaponType)->mSheathingBone;
        }

        return {};
    }

    void ActorAnimation::resetControllers(osg::Node* node)
    {
        if (node == nullptr)
            return;

        // This is used to avoid playing animations intended for equipped weapons on holstered weapons.
        SceneUtil::ForceControllerSourcesVisitor removeVisitor(std::make_shared<NullAnimationTime>());
        node->accept(removeVisitor);
    }

    void ActorAnimation::updateHolsteredWeapon(bool showHolsteredWeapons)
    {
        if (!Settings::game().mWeaponSheathing)
            return;

        if (!mPtr.getClass().hasInventoryStore(mPtr))
            return;

        mScabbard.reset();

        const MWWorld::InventoryStore& inv = mPtr.getClass().getInventoryStore(mPtr);
        MWWorld::ConstContainerStoreIterator weapon = inv.getSlot(MWWorld::InventoryStore::Slot_CarriedRight);
        if (weapon == inv.end() || weapon->getType() != ESM::Weapon::sRecordId)
            return;

        // Since throwing weapons stack themselves, do not show such weapon itself
        int type = weapon->get<ESM::Weapon>()->mBase->mData.mType;
        if (MWMechanics::getWeaponType(type)->mWeaponClass == ESM::WeaponType::Thrown)
            showHolsteredWeapons = false;

        std::string mesh = weapon->getClass().getCorrectedModel(*weapon);
        std::string_view boneName = getHolsteredWeaponBoneName(*weapon);
        if (mesh.empty() || boneName.empty())
            return;

        // If the scabbard is not found, use the weapon mesh as fallback.
        const std::string scabbardName = addSuffixBeforeExtension(mesh, "_sh");
        bool isEnchanted = !weapon->getClass().getEnchantment(*weapon).empty();
        if (!mResourceSystem->getVFS()->exists(scabbardName))
        {
            if (showHolsteredWeapons)
            {
                osg::Vec4f glowColor = weapon->getClass().getEnchantmentColor(*weapon);
                mScabbard = attachMesh(mesh, boneName, isEnchanted, &glowColor);
                if (mScabbard)
                    resetControllers(mScabbard->getNode());
            }

            return;
        }

        mScabbard = attachMesh(scabbardName, boneName);
        if (mScabbard)
            resetControllers(mScabbard->getNode());

        osg::Group* weaponNode = getBoneByName("Bip01 Weapon");
        if (!weaponNode)
            return;

        // When we draw weapon, hide the Weapon node from sheath model.
        // Otherwise add the enchanted glow to it.
        if (!showHolsteredWeapons)
        {
            weaponNode->setNodeMask(0);
        }
        else
        {
            // If mesh author declared empty weapon node, use transformation from this node, but use the common weapon
            // mesh. This approach allows to tweak weapon position without need to store the whole weapon mesh in the
            // _sh file.
            if (!weaponNode->getNumChildren())
            {
                osg::ref_ptr<osg::Node> fallbackNode
                    = mResourceSystem->getSceneManager()->getInstance(VFS::Path::toNormalized(mesh), weaponNode);
                resetControllers(fallbackNode);
            }

            if (isEnchanted)
            {
                osg::Vec4f glowColor = weapon->getClass().getEnchantmentColor(*weapon);
                mGlowUpdater = SceneUtil::addEnchantedGlow(weaponNode, mResourceSystem, glowColor);
            }
        }
    }

    void ActorAnimation::updateQuiver()
    {
        if (!Settings::game().mWeaponSheathing)
            return;

        if (!mPtr.getClass().hasInventoryStore(mPtr))
            return;

        const MWWorld::InventoryStore& inv = mPtr.getClass().getInventoryStore(mPtr);
        MWWorld::ConstContainerStoreIterator weapon = inv.getSlot(MWWorld::InventoryStore::Slot_CarriedRight);
        if (weapon == inv.end() || weapon->getType() != ESM::Weapon::sRecordId)
            return;

        std::string_view mesh = weapon->getClass().getModel(*weapon);
        std::string_view boneName = getHolsteredWeaponBoneName(*weapon);
        if (mesh.empty() || boneName.empty())
            return;

        osg::Group* ammoNode = getBoneByName("Bip01 Ammo");
        if (!ammoNode)
            return;

        // Special case for throwing weapons - they do not use ammo, but they stack themselves
        bool suitableAmmo = false;
        MWWorld::ConstContainerStoreIterator ammo = weapon;
        unsigned int ammoCount = 0;
        int type = weapon->get<ESM::Weapon>()->mBase->mData.mType;
        const auto& weaponType = MWMechanics::getWeaponType(type);
        if (weaponType->mWeaponClass == ESM::WeaponType::Thrown)
        {
            ammoCount = ammo->getCellRef().getCount();
            osg::Group* throwingWeaponNode = getBoneByName(weaponType->mAttachBone);
            if (throwingWeaponNode && throwingWeaponNode->getNumChildren())
                ammoCount--;

            suitableAmmo = true;
        }
        else
        {
            ammo = inv.getSlot(MWWorld::InventoryStore::Slot_Ammunition);
            if (ammo == inv.end())
                return;

            ammoCount = ammo->getCellRef().getCount();
            bool arrowAttached = isArrowAttached();
            if (arrowAttached)
                ammoCount--;

            suitableAmmo = ammo->get<ESM::Weapon>()->mBase->mData.mType == weaponType->mAmmoType;
        }

        if (!suitableAmmo)
            return;

        // We should not show more ammo than equipped and more than quiver mesh has
        ammoCount = std::min(ammoCount, ammoNode->getNumChildren());

        // Remove existing ammo nodes
        for (unsigned int i = 0; i < ammoNode->getNumChildren(); ++i)
        {
            osg::ref_ptr<osg::Group> arrowNode = ammoNode->getChild(i)->asGroup();
            if (!arrowNode->getNumChildren())
                continue;

            osg::ref_ptr<osg::Node> arrowChildNode = arrowNode->getChild(0);
            arrowNode->removeChild(arrowChildNode);
        }

        // Add new ones
        osg::Vec4f glowColor = ammo->getClass().getEnchantmentColor(*ammo);
        const VFS::Path::Normalized model(ammo->getClass().getCorrectedModel(*ammo));
        for (unsigned int i = 0; i < ammoCount; ++i)
        {
            osg::ref_ptr<osg::Group> arrowNode = ammoNode->getChild(i)->asGroup();
            osg::ref_ptr<osg::Node> arrow = mResourceSystem->getSceneManager()->getInstance(model, arrowNode);
            if (!ammo->getClass().getEnchantment(*ammo).empty())
                mGlowUpdater = SceneUtil::addEnchantedGlow(std::move(arrow), mResourceSystem, glowColor);
        }
    }

    void ActorAnimation::itemAdded(const MWWorld::ConstPtr& item, int /*count*/)
    {
        if (item.getType() == ESM::Light::sRecordId)
        {
            const ESM::Light* light = item.get<ESM::Light>()->mBase;
            if (!(light->mData.mFlags & ESM::Light::Carry))
            {
                addHiddenItemLight(item, light);
            }
        }

        if (!mPtr.getClass().hasInventoryStore(mPtr))
            return;

        // If the count of equipped ammo or throwing weapon was changed, we should update quiver
        const MWWorld::InventoryStore& inv = mPtr.getClass().getInventoryStore(mPtr);
        MWWorld::ConstContainerStoreIterator weapon = inv.getSlot(MWWorld::InventoryStore::Slot_CarriedRight);
        if (weapon == inv.end() || weapon->getType() != ESM::Weapon::sRecordId)
            return;

        MWWorld::ConstContainerStoreIterator ammo = inv.end();
        int type = weapon->get<ESM::Weapon>()->mBase->mData.mType;
        if (MWMechanics::getWeaponType(type)->mWeaponClass == ESM::WeaponType::Thrown)
            ammo = weapon;
        else
            ammo = inv.getSlot(MWWorld::InventoryStore::Slot_Ammunition);

        if (ammo != inv.end() && item.getCellRef().getRefId() == ammo->getCellRef().getRefId())
            updateQuiver();
    }

    void ActorAnimation::itemRemoved(const MWWorld::ConstPtr& item, int /*count*/)
    {
        if (item.getType() == ESM::Light::sRecordId)
        {
            ItemLightMap::iterator iter = mItemLights.find(item);
            if (iter != mItemLights.end())
            {
                if (!item.getCellRef().getCount())
                {
                    removeHiddenItemLight(item);
                }
            }
        }

        if (!mPtr.getClass().hasInventoryStore(mPtr))
            return;

        // If the count of equipped ammo or throwing weapon was changed, we should update quiver
        const MWWorld::InventoryStore& inv = mPtr.getClass().getInventoryStore(mPtr);
        MWWorld::ConstContainerStoreIterator weapon = inv.getSlot(MWWorld::InventoryStore::Slot_CarriedRight);
        if (weapon == inv.end() || weapon->getType() != ESM::Weapon::sRecordId)
            return;

        MWWorld::ConstContainerStoreIterator ammo = inv.end();
        int type = weapon->get<ESM::Weapon>()->mBase->mData.mType;
        if (MWMechanics::getWeaponType(type)->mWeaponClass == ESM::WeaponType::Thrown)
            ammo = weapon;
        else
            ammo = inv.getSlot(MWWorld::InventoryStore::Slot_Ammunition);

        if (ammo != inv.end() && item.getCellRef().getRefId() == ammo->getCellRef().getRefId())
            updateQuiver();
    }

    void ActorAnimation::addHiddenItemLight(const MWWorld::ConstPtr& item, const ESM::Light* esmLight)
    {
        if (mItemLights.find(item) != mItemLights.end())
            return;

        bool exterior = mPtr.isInCell() && mPtr.getCell()->getCell()->isExterior();

        osg::Vec4f ambient(1, 1, 1, 1);
        osg::ref_ptr<SceneUtil::LightSource> lightSource
            = SceneUtil::createLightSource(SceneUtil::LightCommon(*esmLight), Mask_Lighting, exterior, ambient);

        mInsert->addChild(lightSource);

        if (mLightListCallback && mPtr == MWMechanics::getPlayer())
            mLightListCallback->getIgnoredLightSources().insert(lightSource.get());

        mItemLights.insert(std::make_pair(item, lightSource));
    }

    void ActorAnimation::removeHiddenItemLight(const MWWorld::ConstPtr& item)
    {
        ItemLightMap::iterator iter = mItemLights.find(item);
        if (iter == mItemLights.end())
            return;

        if (mLightListCallback && mPtr == MWMechanics::getPlayer())
        {
            std::set<SceneUtil::LightSource*>::iterator ignoredIter
                = mLightListCallback->getIgnoredLightSources().find(iter->second.get());
            if (ignoredIter != mLightListCallback->getIgnoredLightSources().end())
                mLightListCallback->getIgnoredLightSources().erase(ignoredIter);
        }

        mInsert->removeChild(iter->second);
        mItemLights.erase(iter);
    }

    void ActorAnimation::removeFromScene()
    {
        removeFromSceneImpl();
        Animation::removeFromScene();
    }

    void ActorAnimation::removeFromSceneImpl()
    {
        for (const auto& [k, v] : mItemLights)
            mInsert->removeChild(v);
    }
}

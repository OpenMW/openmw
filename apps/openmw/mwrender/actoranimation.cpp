#include "actoranimation.hpp"
#include <utility>

#include <osg/Node>
#include <osg/Group>
#include <osg/Vec4f>

#include <components/esm/loadligh.hpp>
#include <components/esm/loadcell.hpp>

#include <components/resource/resourcesystem.hpp>
#include <components/resource/scenemanager.hpp>

#include <components/sceneutil/attach.hpp>
#include <components/sceneutil/lightmanager.hpp>
#include <components/sceneutil/lightutil.hpp>
#include <components/sceneutil/visitor.hpp>

#include <components/misc/stringops.hpp>

#include <components/settings/settings.hpp>

#include <components/vfs/manager.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwworld/ptr.hpp"
#include "../mwworld/class.hpp"
#include "../mwworld/cellstore.hpp"
#include "../mwworld/esmstore.hpp"
#include "../mwmechanics/actorutil.hpp"
#include "../mwmechanics/weapontype.hpp"

#include "vismask.hpp"

namespace MWRender
{

ActorAnimation::ActorAnimation(const MWWorld::Ptr& ptr, osg::ref_ptr<osg::Group> parentNode, Resource::ResourceSystem* resourceSystem)
    : Animation(ptr, parentNode, resourceSystem)
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
    for (ItemLightMap::iterator iter = mItemLights.begin(); iter != mItemLights.end(); ++iter)
    {
        mInsert->removeChild(iter->second);
    }

    mScabbard.reset();
    mHolsteredShield.reset();
}

PartHolderPtr ActorAnimation::attachMesh(const std::string& model, const std::string& bonename, bool enchantedGlow, osg::Vec4f* glowColor)
{
    osg::Group* parent = getBoneByName(bonename);
    if (!parent)
        return nullptr;

    osg::ref_ptr<osg::Node> instance = mResourceSystem->getSceneManager()->getInstance(model, parent);

    const NodeMap& nodeMap = getNodeMap();
    NodeMap::const_iterator found = nodeMap.find(Misc::StringUtils::lowerCase(bonename));
    if (found == nodeMap.end())
        return PartHolderPtr();

    if (enchantedGlow)
        mGlowUpdater = SceneUtil::addEnchantedGlow(instance, mResourceSystem, *glowColor);

    return PartHolderPtr(new PartHolder(instance));
}

std::string ActorAnimation::getShieldMesh(MWWorld::ConstPtr shield) const
{
    std::string mesh = shield.getClass().getModel(shield);
    const ESM::Armor *armor = shield.get<ESM::Armor>()->mBase;
    const std::vector<ESM::PartReference>& bodyparts = armor->mParts.mParts;
    if (!bodyparts.empty())
    {
        const MWWorld::ESMStore &store = MWBase::Environment::get().getWorld()->getStore();
        const MWWorld::Store<ESM::BodyPart> &partStore = store.get<ESM::BodyPart>();

        // Try to get shield model from bodyparts first, with ground model as fallback
        for (const auto& part : bodyparts)
        {
            // Assume all creatures use the male mesh.
            if (part.mPart != ESM::PRT_Shield || part.mMale.empty())
                continue;
            const ESM::BodyPart *bodypart = partStore.search(part.mMale);
            if (bodypart && bodypart->mData.mType == ESM::BodyPart::MT_Armor && !bodypart->mModel.empty())
            {
                mesh = "meshes\\" + bodypart->mModel;
                break;
            }
        }
    }

    if (mesh.empty())
        return mesh;

    std::string holsteredName = mesh;
    holsteredName = holsteredName.replace(holsteredName.size()-4, 4, "_sh.nif");
    if(mResourceSystem->getVFS()->exists(holsteredName))
    {
        osg::ref_ptr<osg::Node> shieldTemplate = mResourceSystem->getSceneManager()->getInstance(holsteredName);
        SceneUtil::FindByNameVisitor findVisitor ("Bip01 Sheath");
        shieldTemplate->accept(findVisitor);
        osg::ref_ptr<osg::Node> sheathNode = findVisitor.mFoundNode;
        if(!sheathNode)
            return std::string();
    }

    return mesh;
}

bool ActorAnimation::updateCarriedLeftVisible(const int weaptype) const
{
    static const bool shieldSheathing = Settings::Manager::getBool("shield sheathing", "Game");
    if (shieldSheathing)
    {
        const MWWorld::Class &cls = mPtr.getClass();
        MWMechanics::CreatureStats &stats = cls.getCreatureStats(mPtr);
        if (cls.hasInventoryStore(mPtr) && weaptype != ESM::Weapon::Spell)
        {
            SceneUtil::FindByNameVisitor findVisitor ("Bip01 AttachShield");
            mObjectRoot->accept(findVisitor);
            if (findVisitor.mFoundNode || (mPtr == MWMechanics::getPlayer() && mPtr.isInCell() && MWBase::Environment::get().getWorld()->isFirstPerson()))
            {
                const MWWorld::InventoryStore& inv = cls.getInventoryStore(mPtr);
                const MWWorld::ConstContainerStoreIterator weapon = inv.getSlot(MWWorld::InventoryStore::Slot_CarriedRight);
                const MWWorld::ConstContainerStoreIterator shield = inv.getSlot(MWWorld::InventoryStore::Slot_CarriedLeft);
                if (shield != inv.end() && shield->getTypeName() == typeid(ESM::Armor).name() && !getShieldMesh(*shield).empty())
                {
                    if(stats.getDrawState() != MWMechanics::DrawState_Weapon)
                        return false;

                    if (weapon != inv.end())
                    {
                        const std::string &type = weapon->getTypeName();
                        if(type == typeid(ESM::Weapon).name())
                        {
                            const MWWorld::LiveCellRef<ESM::Weapon> *ref = weapon->get<ESM::Weapon>();
                            ESM::Weapon::Type weaponType = (ESM::Weapon::Type)ref->mBase->mData.mType;
                            return !(MWMechanics::getWeaponType(weaponType)->mFlags & ESM::WeaponType::TwoHanded);
                        }
                        else if (type == typeid(ESM::Lockpick).name() || type == typeid(ESM::Probe).name())
                            return true;
                    }
                }
            }
        }
    }

    return !(MWMechanics::getWeaponType(weaptype)->mFlags & ESM::WeaponType::TwoHanded);
}

void ActorAnimation::updateHolsteredShield(bool showCarriedLeft)
{
    static const bool shieldSheathing = Settings::Manager::getBool("shield sheathing", "Game");
    if (!shieldSheathing)
        return;

    if (!mPtr.getClass().hasInventoryStore(mPtr))
        return;

    mHolsteredShield.reset();

    if (showCarriedLeft)
        return;

    const MWWorld::InventoryStore& inv = mPtr.getClass().getInventoryStore(mPtr);
    MWWorld::ConstContainerStoreIterator shield = inv.getSlot(MWWorld::InventoryStore::Slot_CarriedLeft);
    if (shield == inv.end() || shield->getTypeName() != typeid(ESM::Armor).name())
        return;

    // Can not show holdstered shields with two-handed weapons at all
    const MWWorld::ConstContainerStoreIterator weapon = inv.getSlot(MWWorld::InventoryStore::Slot_CarriedRight);
    if(weapon == inv.end())
        return;

    const std::string &type = weapon->getTypeName();
    if(type == typeid(ESM::Weapon).name())
    {
        const MWWorld::LiveCellRef<ESM::Weapon> *ref = weapon->get<ESM::Weapon>();
        ESM::Weapon::Type weaponType = (ESM::Weapon::Type)ref->mBase->mData.mType;
        if (MWMechanics::getWeaponType(weaponType)->mFlags & ESM::WeaponType::TwoHanded)
            return;
    }

    std::string mesh = getShieldMesh(*shield);
    if (mesh.empty())
        return;

    std::string boneName = "Bip01 AttachShield";
    osg::Vec4f glowColor = shield->getClass().getEnchantmentColor(*shield);
    std::string holsteredName = mesh;
    holsteredName = holsteredName.replace(holsteredName.size()-4, 4, "_sh.nif");
    bool isEnchanted = !shield->getClass().getEnchantment(*shield).empty();

    // If we have no dedicated sheath model, use basic shield model as fallback.
    if (!mResourceSystem->getVFS()->exists(holsteredName))
        mHolsteredShield = attachMesh(mesh, boneName, isEnchanted, &glowColor);
    else
        mHolsteredShield = attachMesh(holsteredName, boneName, isEnchanted, &glowColor);

    if (!mHolsteredShield)
        return;

    SceneUtil::FindByNameVisitor findVisitor ("Bip01 Sheath");
    mHolsteredShield->getNode()->accept(findVisitor);
    osg::Group* shieldNode = findVisitor.mFoundNode;

    // If mesh author declared an empty sheath node, use transformation from this node, but use the common shield mesh.
    // This approach allows to tweak shield position without need to store the whole shield mesh in the _sh file.
    if (shieldNode && !shieldNode->getNumChildren())
    {
        osg::ref_ptr<osg::Node> fallbackNode = mResourceSystem->getSceneManager()->getInstance(mesh, shieldNode);
        if (isEnchanted)
            SceneUtil::addEnchantedGlow(shieldNode, mResourceSystem, glowColor);
    }

    if (mAlpha != 1.f)
        mResourceSystem->getSceneManager()->recreateShaders(mHolsteredShield->getNode());
}

bool ActorAnimation::useShieldAnimations() const
{
    static const bool shieldSheathing = Settings::Manager::getBool("shield sheathing", "Game");
    if (!shieldSheathing)
        return false;

    const MWWorld::Class &cls = mPtr.getClass();
    if (!cls.hasInventoryStore(mPtr))
        return false;

    if (getTextKeyTime("shield: equip attach") < 0 || getTextKeyTime("shield: unequip detach") < 0)
        return false;

    const MWWorld::InventoryStore& inv = cls.getInventoryStore(mPtr);
    const MWWorld::ConstContainerStoreIterator weapon = inv.getSlot(MWWorld::InventoryStore::Slot_CarriedRight);
    const MWWorld::ConstContainerStoreIterator shield = inv.getSlot(MWWorld::InventoryStore::Slot_CarriedLeft);
    if (weapon != inv.end() && shield != inv.end() &&
        shield->getTypeName() == typeid(ESM::Armor).name() &&
        !getShieldMesh(*shield).empty())
    {
        const std::string &type = weapon->getTypeName();
        if(type == typeid(ESM::Weapon).name())
        {
            const MWWorld::LiveCellRef<ESM::Weapon> *ref = weapon->get<ESM::Weapon>();
            ESM::Weapon::Type weaponType = (ESM::Weapon::Type)ref->mBase->mData.mType;
            return !(MWMechanics::getWeaponType(weaponType)->mFlags & ESM::WeaponType::TwoHanded);
        }
        else if (type == typeid(ESM::Lockpick).name() || type == typeid(ESM::Probe).name())
            return true;
    }

    return false;
}

osg::Group* ActorAnimation::getBoneByName(const std::string& boneName)
{
    if (!mObjectRoot)
        return nullptr;

    SceneUtil::FindByNameVisitor findVisitor (boneName);
    mObjectRoot->accept(findVisitor);

    return findVisitor.mFoundNode;
}

std::string ActorAnimation::getHolsteredWeaponBoneName(const MWWorld::ConstPtr& weapon)
{
    std::string boneName;
    if(weapon.isEmpty())
        return boneName;

    const std::string &type = weapon.getClass().getTypeName();
    if(type == typeid(ESM::Weapon).name())
    {
        const MWWorld::LiveCellRef<ESM::Weapon> *ref = weapon.get<ESM::Weapon>();
        int weaponType = ref->mBase->mData.mType;
        return MWMechanics::getWeaponType(weaponType)->mSheathingBone;
    }

    return boneName;
}

void ActorAnimation::resetControllers(osg::Node* node)
{
    if (node == nullptr)
        return;

    std::shared_ptr<SceneUtil::ControllerSource> src;
    src.reset(new NullAnimationTime);
    SceneUtil::AssignControllerSourcesVisitor removeVisitor(src);
    node->accept(removeVisitor);
}

void ActorAnimation::updateHolsteredWeapon(bool showHolsteredWeapons)
{
    static const bool weaponSheathing = Settings::Manager::getBool("weapon sheathing", "Game");
    if (!weaponSheathing)
        return;

    if (!mPtr.getClass().hasInventoryStore(mPtr))
        return;

    mScabbard.reset();

    const MWWorld::InventoryStore& inv = mPtr.getClass().getInventoryStore(mPtr);
    MWWorld::ConstContainerStoreIterator weapon = inv.getSlot(MWWorld::InventoryStore::Slot_CarriedRight);
    if (weapon == inv.end() || weapon->getTypeName() != typeid(ESM::Weapon).name())
        return;

    // Since throwing weapons stack themselves, do not show such weapon itself
    int type = weapon->get<ESM::Weapon>()->mBase->mData.mType;
    if (MWMechanics::getWeaponType(type)->mWeaponClass == ESM::WeaponType::Thrown)
        showHolsteredWeapons = false;

    std::string mesh = weapon->getClass().getModel(*weapon);
    std::string scabbardName = mesh;

    std::string boneName = getHolsteredWeaponBoneName(*weapon);
    if (mesh.empty() || boneName.empty())
        return;

    // If the scabbard is not found, use a weapon mesh as fallback.
    // Note: it is unclear how to handle time for controllers attached to bodyparts, so disable them for now.
    // We use the similar approach for other bodyparts.
    scabbardName = scabbardName.replace(scabbardName.size()-4, 4, "_sh.nif");
    bool isEnchanted = !weapon->getClass().getEnchantment(*weapon).empty();
    if(!mResourceSystem->getVFS()->exists(scabbardName))
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
        // If mesh author declared empty weapon node, use transformation from this node, but use the common weapon mesh.
        // This approach allows to tweak weapon position without need to store the whole weapon mesh in the _sh file.
        if (!weaponNode->getNumChildren())
        {
            osg::ref_ptr<osg::Node> fallbackNode = mResourceSystem->getSceneManager()->getInstance(mesh, weaponNode);
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
    static const bool weaponSheathing = Settings::Manager::getBool("weapon sheathing", "Game");
    if (!weaponSheathing)
        return;

    if (!mPtr.getClass().hasInventoryStore(mPtr))
        return;

    const MWWorld::InventoryStore& inv = mPtr.getClass().getInventoryStore(mPtr);
    MWWorld::ConstContainerStoreIterator weapon = inv.getSlot(MWWorld::InventoryStore::Slot_CarriedRight);
    if(weapon == inv.end() || weapon->getTypeName() != typeid(ESM::Weapon).name())
        return;

    std::string mesh = weapon->getClass().getModel(*weapon);
    std::string boneName = getHolsteredWeaponBoneName(*weapon);
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
        ammoCount = ammo->getRefData().getCount();
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

        ammoCount = ammo->getRefData().getCount();
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
    for (unsigned int i=0; i<ammoNode->getNumChildren(); ++i)
    {
        osg::ref_ptr<osg::Group> arrowNode = ammoNode->getChild(i)->asGroup();
        if (!arrowNode->getNumChildren())
            continue;

        osg::ref_ptr<osg::Node> arrowChildNode = arrowNode->getChild(0);
        arrowNode->removeChild(arrowChildNode);
    }

    // Add new ones
    osg::Vec4f glowColor = ammo->getClass().getEnchantmentColor(*ammo);
    std::string model = ammo->getClass().getModel(*ammo);
    for (unsigned int i=0; i<ammoCount; ++i)
    {
        osg::ref_ptr<osg::Group> arrowNode = ammoNode->getChild(i)->asGroup();
        osg::ref_ptr<osg::Node> arrow = mResourceSystem->getSceneManager()->getInstance(model, arrowNode);
        if (!ammo->getClass().getEnchantment(*ammo).empty())
            mGlowUpdater = SceneUtil::addEnchantedGlow(arrow, mResourceSystem, glowColor);
    }
}

void ActorAnimation::itemAdded(const MWWorld::ConstPtr& item, int /*count*/)
{
    if (item.getTypeName() == typeid(ESM::Light).name())
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
    if(weapon == inv.end() || weapon->getTypeName() != typeid(ESM::Weapon).name())
        return;

    MWWorld::ConstContainerStoreIterator ammo = inv.end();
    int type = weapon->get<ESM::Weapon>()->mBase->mData.mType;
    if (MWMechanics::getWeaponType(type)->mWeaponClass == ESM::WeaponType::Thrown)
        ammo = weapon;
    else
        ammo = inv.getSlot(MWWorld::InventoryStore::Slot_Ammunition);

    if(ammo != inv.end() && item.getCellRef().getRefId() == ammo->getCellRef().getRefId())
        updateQuiver();
}

void ActorAnimation::itemRemoved(const MWWorld::ConstPtr& item, int /*count*/)
{
    if (item.getTypeName() == typeid(ESM::Light).name())
    {
        ItemLightMap::iterator iter = mItemLights.find(item);
        if (iter != mItemLights.end())
        {
            if (!item.getRefData().getCount())
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
    if(weapon == inv.end() || weapon->getTypeName() != typeid(ESM::Weapon).name())
        return;

    MWWorld::ConstContainerStoreIterator ammo = inv.end();
    int type = weapon->get<ESM::Weapon>()->mBase->mData.mType;
    if (MWMechanics::getWeaponType(type)->mWeaponClass == ESM::WeaponType::Thrown)
        ammo = weapon;
    else
        ammo = inv.getSlot(MWWorld::InventoryStore::Slot_Ammunition);

    if(ammo != inv.end() && item.getCellRef().getRefId() == ammo->getCellRef().getRefId())
        updateQuiver();
}

void ActorAnimation::addHiddenItemLight(const MWWorld::ConstPtr& item, const ESM::Light* esmLight)
{
    if (mItemLights.find(item) != mItemLights.end())
        return;

    bool exterior = mPtr.isInCell() && mPtr.getCell()->getCell()->isExterior();

    osg::Vec4f ambient(1,1,1,1);
    osg::ref_ptr<SceneUtil::LightSource> lightSource = SceneUtil::createLightSource(esmLight, Mask_Lighting, exterior, ambient);

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
        std::set<SceneUtil::LightSource*>::iterator ignoredIter = mLightListCallback->getIgnoredLightSources().find(iter->second.get());
        if (ignoredIter != mLightListCallback->getIgnoredLightSources().end())
            mLightListCallback->getIgnoredLightSources().erase(ignoredIter);
    }

    mInsert->removeChild(iter->second);
    mItemLights.erase(iter);
}

}

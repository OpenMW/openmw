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

#include <components/fallback/fallback.hpp>

#include <components/misc/stringops.hpp>

#include <components/settings/settings.hpp>

#include <components/vfs/manager.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwworld/ptr.hpp"
#include "../mwworld/class.hpp"
#include "../mwworld/cellstore.hpp"
#include "../mwmechanics/actorutil.hpp"

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

    mWeaponSheathing = Settings::Manager::getBool("weapon sheathing", "Game");
}

ActorAnimation::~ActorAnimation()
{
    for (ItemLightMap::iterator iter = mItemLights.begin(); iter != mItemLights.end(); ++iter)
    {
        mInsert->removeChild(iter->second);
    }

    mScabbard.reset();
}

PartHolderPtr ActorAnimation::getWeaponPart(const std::string& model, const std::string& bonename, bool enchantedGlow, osg::Vec4f* glowColor)
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
        addGlow(instance, *glowColor);

    return PartHolderPtr(new PartHolder(instance));
}

osg::Group* ActorAnimation::getBoneByName(std::string boneName)
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
        ESM::Weapon::Type weaponType = (ESM::Weapon::Type)ref->mBase->mData.mType;
        return getHolsteredWeaponBoneName(weaponType);
    }

    return boneName;
}

std::string ActorAnimation::getHolsteredWeaponBoneName(const unsigned int weaponType)
{
    std::string boneName;

    switch(weaponType)
    {
        case ESM::Weapon::ShortBladeOneHand:
            boneName = "Bip01 ShortBladeOneHand";
            break;
        case ESM::Weapon::LongBladeOneHand:
            boneName = "Bip01 LongBladeOneHand";
            break;
        case ESM::Weapon::BluntOneHand:
            boneName = "Bip01 BluntOneHand";
            break;
        case ESM::Weapon::AxeOneHand:
            boneName = "Bip01 LongBladeOneHand";
            break;
        case ESM::Weapon::LongBladeTwoHand:
            boneName = "Bip01 LongBladeTwoClose";
            break;
        case ESM::Weapon::BluntTwoClose:
            boneName = "Bip01 BluntTwoClose";
            break;
        case ESM::Weapon::AxeTwoHand:
            boneName = "Bip01 AxeTwoClose";
            break;
        case ESM::Weapon::BluntTwoWide:
            boneName = "Bip01 BluntTwoWide";
            break;
        case ESM::Weapon::SpearTwoWide:
            boneName = "Bip01 SpearTwoWide";
            break;
        case ESM::Weapon::MarksmanBow:
            boneName = "Bip01 MarksmanBow";
            break;
        case ESM::Weapon::MarksmanCrossbow:
            boneName = "Bip01 MarksmanCrossbow";
            break;
        case ESM::Weapon::MarksmanThrown:
            boneName = "Bip01 MarksmanThrown";
            break;
        default:
            break;
    }

    return boneName;
}

void ActorAnimation::injectWeaponBones()
{
    if (!mResourceSystem->getVFS()->exists("meshes\\xbase_anim_sh.nif"))
    {
        mWeaponSheathing = false;
        return;
    }

    osg::ref_ptr<osg::Node> sheathSkeleton = mResourceSystem->getSceneManager()->getInstance("meshes\\xbase_anim_sh.nif");

    for (unsigned int type=0; type<=ESM::Weapon::MarksmanThrown; ++type)
    {
        const std::string holsteredBoneName = getHolsteredWeaponBoneName(type);

        SceneUtil::FindByNameVisitor findVisitor (holsteredBoneName);
        sheathSkeleton->accept(findVisitor);
        osg::ref_ptr<osg::Node> sheathNode = findVisitor.mFoundNode;

        if (sheathNode && sheathNode.get()->getNumParents())
        {
            osg::Group* sheathParent = getBoneByName(sheathNode.get()->getParent(0)->getName());

            if (sheathParent)
            {
                sheathNode.get()->getParent(0)->removeChild(sheathNode);
                sheathParent->addChild(sheathNode);
            }
        }
    }
}

// To make sure we do not run morph controllers for weapons, i.e. bows
class EmptyCallback : public osg::NodeCallback
{
    public:

        virtual void operator()(osg::Node* node, osg::NodeVisitor* nv)
        {
        }
};

void ActorAnimation::updateHolsteredWeapon(bool showHolsteredWeapons)
{
    if (!mWeaponSheathing)
        return;

    if (!mPtr.getClass().hasInventoryStore(mPtr))
        return;

    mScabbard.reset();

    const MWWorld::InventoryStore& inv = mPtr.getClass().getInventoryStore(mPtr);
    MWWorld::ConstContainerStoreIterator weapon = inv.getSlot(MWWorld::InventoryStore::Slot_CarriedRight);
    if (weapon == inv.end() || weapon->getTypeName() != typeid(ESM::Weapon).name())
        return;

    // Since throwing weapons stack themselves, do not show such weapon itself
    if (weapon->get<ESM::Weapon>()->mBase->mData.mType == ESM::Weapon::MarksmanThrown)
        showHolsteredWeapons = false;

    std::string mesh = weapon->getClass().getModel(*weapon);
    std::string scabbardName = mesh;

    std::string boneName = getHolsteredWeaponBoneName(*weapon);
    if (mesh.empty() || boneName.empty())
        return;

    // If the scabbard is not found, use a weapon mesh as fallback
    scabbardName = scabbardName.replace(scabbardName.size()-4, 4, "_sh.nif");
    bool isEnchanted = !weapon->getClass().getEnchantment(*weapon).empty();
    if(!mResourceSystem->getVFS()->exists(scabbardName))
    {
        if (showHolsteredWeapons)
        {
            osg::Vec4f glowColor = getEnchantmentColor(*weapon);
            mScabbard = getWeaponPart(mesh, boneName, isEnchanted, &glowColor);
            if (mScabbard)
                mScabbard->getNode()->setUpdateCallback(new EmptyCallback);
        }

        return;
    }

    mScabbard = getWeaponPart(scabbardName, boneName);

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
            fallbackNode->setUpdateCallback(new EmptyCallback);
        }

        if (isEnchanted)
        {
            osg::Vec4f glowColor = getEnchantmentColor(*weapon);
            addGlow(weaponNode, glowColor);
        }
    }
}

void ActorAnimation::updateQuiver()
{
    if (!mWeaponSheathing)
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
    if (weapon->get<ESM::Weapon>()->mBase->mData.mType == ESM::Weapon::MarksmanThrown)
    {
        ammoCount = ammo->getRefData().getCount();
        osg::Group* throwingWeaponNode = getBoneByName("Weapon Bone");
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

        if (weapon->get<ESM::Weapon>()->mBase->mData.mType == ESM::Weapon::MarksmanCrossbow)
            suitableAmmo = ammo->get<ESM::Weapon>()->mBase->mData.mType == ESM::Weapon::Bolt;
        else if (weapon->get<ESM::Weapon>()->mBase->mData.mType == ESM::Weapon::MarksmanBow)
            suitableAmmo = ammo->get<ESM::Weapon>()->mBase->mData.mType == ESM::Weapon::Arrow;
    }

    if (ammoNode && suitableAmmo)
    {
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
        osg::Vec4f glowColor = getEnchantmentColor(*ammo);
        std::string model = ammo->getClass().getModel(*ammo);
        for (unsigned int i=0; i<ammoCount; ++i)
        {
            osg::ref_ptr<osg::Group> arrowNode = ammoNode->getChild(i)->asGroup();
            osg::ref_ptr<osg::Node> arrow = mResourceSystem->getSceneManager()->getInstance(model, arrowNode);
            if (!ammo->getClass().getEnchantment(*ammo).empty())
                addGlow(arrow, glowColor);
        }
    }

    // recreate shaders for invisible actors, otherwise new nodes will be visible
    if (mAlpha != 1.f)
        mResourceSystem->getSceneManager()->recreateShaders(mObjectRoot);
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
    if (weapon->get<ESM::Weapon>()->mBase->mData.mType == ESM::Weapon::MarksmanThrown)
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
    if (weapon->get<ESM::Weapon>()->mBase->mData.mType == ESM::Weapon::MarksmanThrown)
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

    const Fallback::Map* fallback = MWBase::Environment::get().getWorld()->getFallback();
    static bool outQuadInLin = fallback->getFallbackBool("LightAttenuation_OutQuadInLin");
    static bool useQuadratic = fallback->getFallbackBool("LightAttenuation_UseQuadratic");
    static float quadraticValue = fallback->getFallbackFloat("LightAttenuation_QuadraticValue");
    static float quadraticRadiusMult = fallback->getFallbackFloat("LightAttenuation_QuadraticRadiusMult");
    static bool useLinear = fallback->getFallbackBool("LightAttenuation_UseLinear");
    static float linearRadiusMult = fallback->getFallbackFloat("LightAttenuation_LinearRadiusMult");
    static float linearValue = fallback->getFallbackFloat("LightAttenuation_LinearValue");
    bool exterior = mPtr.isInCell() && mPtr.getCell()->getCell()->isExterior();

    osg::Vec4f ambient(1,1,1,1);
    osg::ref_ptr<SceneUtil::LightSource> lightSource = SceneUtil::createLightSource(esmLight, Mask_Lighting, exterior, outQuadInLin,
                                 useQuadratic, quadraticValue, quadraticRadiusMult, useLinear, linearRadiusMult, linearValue, ambient);

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

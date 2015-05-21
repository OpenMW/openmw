#include "creatureanimation.hpp"

#include <components/esm/loadcrea.hpp>

#include <osg/PositionAttitudeTransform>

#include <components/resource/resourcesystem.hpp>
#include <components/resource/scenemanager.hpp>
#include <components/sceneutil/attach.hpp>

#include "../mwbase/world.hpp"

#include "../mwworld/class.hpp"

namespace MWRender
{

CreatureAnimation::CreatureAnimation(const MWWorld::Ptr &ptr,
                                     const std::string& model, Resource::ResourceSystem* resourceSystem)
  : Animation(ptr, osg::ref_ptr<osg::Group>(ptr.getRefData().getBaseNode()), resourceSystem)
{
    MWWorld::LiveCellRef<ESM::Creature> *ref = mPtr.get<ESM::Creature>();

    if(!model.empty())
    {
        setObjectRoot(model, false, false);
        //setRenderProperties(mObjectRoot, RV_Actors, RQG_Main, RQG_Alpha);

        if((ref->mBase->mFlags&ESM::Creature::Bipedal))
            addAnimSource("meshes\\xbase_anim.nif");
        addAnimSource(model);
    }
}


CreatureWeaponAnimation::CreatureWeaponAnimation(const MWWorld::Ptr &ptr, const std::string& model, Resource::ResourceSystem* resourceSystem)
    : Animation(ptr, osg::ref_ptr<osg::Group>(ptr.getRefData().getBaseNode()), resourceSystem)
    , mShowWeapons(false)
    , mShowCarriedLeft(false)
{
    MWWorld::LiveCellRef<ESM::Creature> *ref = mPtr.get<ESM::Creature>();

    if(!model.empty())
    {
        setObjectRoot(model, true, false);
        //setRenderProperties(mObjectRoot, RV_Actors, RQG_Main, RQG_Alpha);

        if((ref->mBase->mFlags&ESM::Creature::Bipedal))
            addAnimSource("meshes\\xbase_anim.nif");
        addAnimSource(model);

        mPtr.getClass().getInventoryStore(mPtr).setListener(this, mPtr);

        updateParts();
    }

    //mWeaponAnimationTime = Ogre::SharedPtr<WeaponAnimationTime>(new WeaponAnimationTime(this));
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
    mWeapon.reset();
    mShield.reset();

    if (mShowWeapons)
        updatePart(mWeapon, MWWorld::InventoryStore::Slot_CarriedRight);
    if (mShowCarriedLeft)
        updatePart(mShield, MWWorld::InventoryStore::Slot_CarriedLeft);
}

void CreatureWeaponAnimation::updatePart(PartHolderPtr& scene, int slot)
{
    if (!mObjectRoot)
        return;

    MWWorld::InventoryStore& inv = mPtr.getClass().getInventoryStore(mPtr);
    MWWorld::ContainerStoreIterator it = inv.getSlot(slot);

    if (it == inv.end())
    {
        scene.reset();
        return;
    }
    MWWorld::Ptr item = *it;

    std::string bonename;
    if (slot == MWWorld::InventoryStore::Slot_CarriedRight)
        bonename = "Weapon Bone";
    else
        bonename = "Shield Bone";

    osg::ref_ptr<osg::Node> node = mResourceSystem->getSceneManager()->createInstance(item.getClass().getModel(item));
    SceneUtil::attach(node, mObjectRoot, bonename, bonename);

    scene.reset(new PartHolder(node));

    if (!item.getClass().getEnchantment(item).empty())
        addGlow(node, getEnchantmentColor(item));

    // Crossbows start out with a bolt attached
    // FIXME: code duplicated from NpcAnimation
    if (slot == MWWorld::InventoryStore::Slot_CarriedRight &&
            item.getTypeName() == typeid(ESM::Weapon).name() &&
            item.get<ESM::Weapon>()->mBase->mData.mType == ESM::Weapon::MarksmanCrossbow)
    {
        MWWorld::ContainerStoreIterator ammo = inv.getSlot(MWWorld::InventoryStore::Slot_Ammunition);
        if (ammo != inv.end() && ammo->get<ESM::Weapon>()->mBase->mData.mType == ESM::Weapon::Bolt)
            attachArrow();
        //else
        //    mAmmunition.setNull();
    }
    //else
        //mAmmunition.setNull();

    /*
    std::vector<Ogre::Controller<Ogre::Real> >::iterator ctrl(scene->mControllers.begin());
    for(;ctrl != scene->mControllers.end();++ctrl)
    {
        if(ctrl->getSource().isNull())
        {
            if (slot == MWWorld::InventoryStore::Slot_CarriedRight)
                ctrl->setSource(mWeaponAnimationTime);
            else
                ctrl->setSource(Ogre::SharedPtr<NullAnimationTime>(new NullAnimationTime()));
        }
    }
    */
}

/*
void CreatureWeaponAnimation::configureAddedObject(NifOgre::ObjectScenePtr object, MWWorld::Ptr ptr, int slot)
{
    //Ogre::Vector3 glowColor = getEnchantmentColor(ptr);

    //setRenderProperties(object, RV_Actors, RQG_Main, RQG_Alpha, 0,
    //                    !ptr.getClass().getEnchantment(ptr).empty(), &glowColor);
}
*/

void CreatureWeaponAnimation::attachArrow()
{
    //WeaponAnimation::attachArrow(mPtr);
}

void CreatureWeaponAnimation::releaseArrow()
{
    //WeaponAnimation::releaseArrow(mPtr);
}

osg::Vec3f CreatureWeaponAnimation::runAnimation(float duration)
{
    /*
    Ogre::Vector3 ret = Animation::runAnimation(duration);

    if (mSkelBase)
        pitchSkeleton(mPtr.getRefData().getPosition().rot[0], mSkelBase->getSkeleton());
    */

    return osg::Vec3f();
}

}

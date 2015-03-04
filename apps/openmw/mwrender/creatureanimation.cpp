#include "creatureanimation.hpp"

#include <OgreEntity.h>
#include <OgreSkeletonInstance.h>
#include <OgreBone.h>

#include <components/esm/loadcrea.hpp>

#include "../mwbase/world.hpp"

#include "../mwworld/class.hpp"

#include "renderconst.hpp"

namespace MWRender
{


CreatureAnimation::CreatureAnimation(const MWWorld::Ptr &ptr, const std::string& model)
  : Animation(ptr, ptr.getRefData().getBaseNode())
{
    MWWorld::LiveCellRef<ESM::Creature> *ref = mPtr.get<ESM::Creature>();

    if(!model.empty())
    {
        setObjectRoot(model, false);
        setRenderProperties(mObjectRoot, RV_Actors, RQG_Main, RQG_Alpha);

        if((ref->mBase->mFlags&ESM::Creature::Bipedal))
            addAnimSource("meshes\\xbase_anim.nif");
        addAnimSource(model);
    }
}


CreatureWeaponAnimation::CreatureWeaponAnimation(const MWWorld::Ptr &ptr, const std::string& model)
    : Animation(ptr, ptr.getRefData().getBaseNode())
    , mShowWeapons(false)
    , mShowCarriedLeft(false)
{
    MWWorld::LiveCellRef<ESM::Creature> *ref = mPtr.get<ESM::Creature>();

    if(!model.empty())
    {
        setObjectRoot(model, false);
        setRenderProperties(mObjectRoot, RV_Actors, RQG_Main, RQG_Alpha);

        if((ref->mBase->mFlags&ESM::Creature::Bipedal))
            addAnimSource("meshes\\xbase_anim.nif");
        addAnimSource(model);

        mPtr.getClass().getInventoryStore(mPtr).setListener(this, mPtr);

        updateParts();
    }

    mWeaponAnimationTime = Ogre::SharedPtr<WeaponAnimationTime>(new WeaponAnimationTime(this));
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
    mWeapon.setNull();
    mShield.setNull();

    if (mShowWeapons)
        updatePart(mWeapon, MWWorld::InventoryStore::Slot_CarriedRight);
    if (mShowCarriedLeft)
        updatePart(mShield, MWWorld::InventoryStore::Slot_CarriedLeft);
}

void CreatureWeaponAnimation::updatePart(NifOgre::ObjectScenePtr& scene, int slot)
{
    if (!mSkelBase)
        return;

    MWWorld::InventoryStore& inv = mPtr.getClass().getInventoryStore(mPtr);
    MWWorld::ContainerStoreIterator it = inv.getSlot(slot);

    if (it == inv.end())
    {
        scene.setNull();
        return;
    }
    MWWorld::Ptr item = *it;

    std::string bonename;
    if (slot == MWWorld::InventoryStore::Slot_CarriedRight)
        bonename = "Weapon Bone";
    else
        bonename = "Shield Bone";

    scene = NifOgre::Loader::createObjects(mSkelBase, bonename, bonename, mInsert, item.getClass().getModel(item));
    Ogre::Vector3 glowColor = getEnchantmentColor(item);

    setRenderProperties(scene, RV_Actors, RQG_Main, RQG_Alpha, 0,
                        !item.getClass().getEnchantment(item).empty(), &glowColor);

    // Crossbows start out with a bolt attached
    if (slot == MWWorld::InventoryStore::Slot_CarriedRight &&
            item.getTypeName() == typeid(ESM::Weapon).name() &&
            item.get<ESM::Weapon>()->mBase->mData.mType == ESM::Weapon::MarksmanCrossbow)
    {
        MWWorld::ContainerStoreIterator ammo = inv.getSlot(MWWorld::InventoryStore::Slot_Ammunition);
        if (ammo != inv.end() && ammo->get<ESM::Weapon>()->mBase->mData.mType == ESM::Weapon::Bolt)
            attachArrow();
        else
            mAmmunition.setNull();
    }
    else
        mAmmunition.setNull();

    if(scene->mSkelBase)
    {
        Ogre::SkeletonInstance *skel = scene->mSkelBase->getSkeleton();
        if(scene->mSkelBase->isParentTagPoint())
        {
            Ogre::Node *root = scene->mSkelBase->getParentNode();
            if(skel->hasBone("BoneOffset"))
            {
                Ogre::Bone *offset = skel->getBone("BoneOffset");

                root->translate(offset->getPosition());

                // It appears that the BoneOffset rotation is completely bogus, at least for light models.
                //root->rotate(offset->getOrientation());
                root->pitch(Ogre::Degree(-90.0f));

                root->scale(offset->getScale());
                root->setInitialState();
            }
        }
        updateSkeletonInstance(mSkelBase->getSkeleton(), skel);
    }

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
}

void CreatureWeaponAnimation::configureAddedObject(NifOgre::ObjectScenePtr object, MWWorld::Ptr ptr, int slot)
{
    Ogre::Vector3 glowColor = getEnchantmentColor(ptr);

    setRenderProperties(object, RV_Actors, RQG_Main, RQG_Alpha, 0,
                        !ptr.getClass().getEnchantment(ptr).empty(), &glowColor);
}

void CreatureWeaponAnimation::attachArrow()
{
    WeaponAnimation::attachArrow(mPtr);
}

void CreatureWeaponAnimation::releaseArrow()
{
    WeaponAnimation::releaseArrow(mPtr);
}

Ogre::Vector3 CreatureWeaponAnimation::runAnimation(float duration)
{
    Ogre::Vector3 ret = Animation::runAnimation(duration);

    if (mSkelBase)
        pitchSkeleton(mPtr.getRefData().getPosition().rot[0], mSkelBase->getSkeleton());

    if (!mWeapon.isNull())
    {
        for (unsigned int i=0; i<mWeapon->mControllers.size(); ++i)
            mWeapon->mControllers[i].update();
    }
    if (!mShield.isNull())
    {
        for (unsigned int i=0; i<mShield->mControllers.size(); ++i)
            mShield->mControllers[i].update();
    }

    return ret;
}

}

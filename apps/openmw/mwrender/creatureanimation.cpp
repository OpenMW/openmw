#include "creatureanimation.hpp"

#include <OgreEntity.h>
#include <OgreSkeletonInstance.h>
#include <OgreBone.h>

#include "renderconst.hpp"

#include "../mwbase/world.hpp"

#include "../mwworld/class.hpp"

namespace MWRender
{


CreatureAnimation::CreatureAnimation(const MWWorld::Ptr &ptr)
  : Animation(ptr, ptr.getRefData().getBaseNode())
{
    MWWorld::LiveCellRef<ESM::Creature> *ref = mPtr.get<ESM::Creature>();

    std::string model = ptr.getClass().getModel(ptr);
    if(!model.empty())
    {
        setObjectRoot(model, false);
        setRenderProperties(mObjectRoot, RV_Actors, RQG_Main, RQG_Alpha);

        if((ref->mBase->mFlags&ESM::Creature::Bipedal))
            addAnimSource("meshes\\base_anim.nif");
        addAnimSource(model);
    }
}


CreatureWeaponAnimation::CreatureWeaponAnimation(const MWWorld::Ptr &ptr)
    : Animation(ptr, ptr.getRefData().getBaseNode())
    , mShowWeapons(false)
    , mShowCarriedLeft(false)
{
    MWWorld::LiveCellRef<ESM::Creature> *ref = mPtr.get<ESM::Creature>();

    std::string model = ptr.getClass().getModel(ptr);
    if(!model.empty())
    {
        setObjectRoot(model, false);
        setRenderProperties(mObjectRoot, RV_Actors, RQG_Main, RQG_Alpha);

        if((ref->mBase->mFlags&ESM::Creature::Bipedal))
            addAnimSource("meshes\\base_anim.nif");
        addAnimSource(model);

        mPtr.getClass().getInventoryStore(mPtr).setListener(this, mPtr);

        updateParts();
    }
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

    scene = NifOgre::Loader::createObjects(mSkelBase, bonename, mInsert, item.getClass().getModel(item));
    Ogre::Vector3 glowColor = getEnchantmentColor(item);

    setRenderProperties(scene, RV_Actors, RQG_Main, RQG_Alpha, 0,
                        !item.getClass().getEnchantment(item).empty(), &glowColor);

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

    // TODO:
    // type == ESM::PRT_Weapon should get an animation source based on the current offset
    // of the weapon attack animation (from its beginning, or start marker?)
    std::vector<Ogre::Controller<Ogre::Real> >::iterator ctrl(scene->mControllers.begin());
    for(;ctrl != scene->mControllers.end();ctrl++)
    {
        if(ctrl->getSource().isNull())
            ctrl->setSource(Ogre::SharedPtr<NullAnimationTime>(new NullAnimationTime()));
    }
}

}

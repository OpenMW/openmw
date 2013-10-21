#ifndef _GAME_RENDER_NPCANIMATION_H
#define _GAME_RENDER_NPCANIMATION_H

#include "animation.hpp"

#include "../mwworld/containerstore.hpp"

namespace ESM
{
    struct NPC;
}

namespace MWWorld
{
    class InventoryStore;
}

namespace MWRender
{

class NpcAnimation : public Animation
{
public:
    typedef std::map<ESM::PartReferenceType,std::string> PartBoneMap;

    enum ViewMode {
        VM_Normal,
        VM_FirstPerson,
        VM_HeadOnly
    };

private:
    static const PartBoneMap sPartList;

    int mStateID;

    // Bounded Parts
    NifOgre::ObjectList mObjectParts[ESM::PRT_Count];

    const ESM::NPC *mNpc;
    std::string    mHeadModel;
    std::string    mHairModel;
    ViewMode       mViewMode;
    bool mShowWeapons;

    float mTimeToChange;
    MWWorld::ContainerStoreIterator mRobe;
    MWWorld::ContainerStoreIterator mHelmet;
    MWWorld::ContainerStoreIterator mShirt;
    MWWorld::ContainerStoreIterator mCuirass;
    MWWorld::ContainerStoreIterator mGreaves;
    MWWorld::ContainerStoreIterator mPauldronL;
    MWWorld::ContainerStoreIterator mPauldronR;
    MWWorld::ContainerStoreIterator mBoots;
    MWWorld::ContainerStoreIterator mPants;
    MWWorld::ContainerStoreIterator mGloveL;
    MWWorld::ContainerStoreIterator mGloveR;
    MWWorld::ContainerStoreIterator mSkirtIter;
    MWWorld::ContainerStoreIterator mWeapon;
    MWWorld::ContainerStoreIterator mShield;

    int mVisibilityFlags;

    int mPartslots[ESM::PRT_Count];  //Each part slot is taken by clothing, armor, or is empty
    int mPartPriorities[ESM::PRT_Count];

    Ogre::Vector3 mFirstPersonOffset;

    void updateNpcBase();

    NifOgre::ObjectList insertBoundedPart(const std::string &model, int group, const std::string &bonename);

    void removeIndividualPart(ESM::PartReferenceType type);
    void reserveIndividualPart(ESM::PartReferenceType type, int group, int priority);

    bool addOrReplaceIndividualPart(ESM::PartReferenceType type, int group, int priority, const std::string &mesh);
    void removePartGroup(int group);
    void addPartGroup(int group, int priority, const std::vector<ESM::PartReference> &parts);

public:
    NpcAnimation(const MWWorld::Ptr& ptr, Ogre::SceneNode* node,
                 MWWorld::InventoryStore& inv, int visibilityFlags,
                 ViewMode viewMode=VM_Normal);
    virtual ~NpcAnimation();

    virtual Ogre::Vector3 runAnimation(float timepassed);

    virtual void showWeapons(bool showWeapon);

    void setViewMode(ViewMode viewMode);

    void updateParts(bool forceupdate = false);

    /// \brief Applies a translation to the arms and hands.
    /// This may be called multiple times before the animation
    /// is updated to add additional offsets.
    void addFirstPersonOffset(const Ogre::Vector3 &offset);

    /// Rebuilds the NPC, updating their root model, animation sources, and equipment.
    void rebuild();
};

}

#endif

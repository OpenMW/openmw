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
struct PartInfo {
    ESM::PartReferenceType type;
    const char name[32];
};

enum ViewMode {
    VM_Normal,
    VM_FirstPerson,
    VM_HeadOnly
};

private:
    static const size_t sPartListSize = 27;
    static const PartInfo sPartList[sPartListSize];

    int mStateID;

    // Bounded Parts
    NifOgre::ObjectList mObjectParts[sPartListSize];

    const ESM::NPC  *mNpc;
    std::string     mHeadModel;
    std::string     mHairModel;
    std::string     mBodyPrefix;
    ViewMode        mViewMode;
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

    int mPartslots[sPartListSize];  //Each part slot is taken by clothing, armor, or is empty
    int mPartPriorities[sPartListSize];

    NifOgre::ObjectList insertBoundedPart(const std::string &model, int group, const std::string &bonename);

    void updateParts(bool forceupdate = false);

    void removeIndividualPart(int type);
    void reserveIndividualPart(int type, int group, int priority);

    bool addOrReplaceIndividualPart(int type, int group, int priority, const std::string &mesh);
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

    void forceUpdate()
    { updateParts(true); }
};

}

#endif

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

private:
    static const size_t sPartListSize = 27;
    static const PartInfo sPartList[sPartListSize];

    int mStateID;

    // Bounded Parts
    NifOgre::EntityList mEntityParts[sPartListSize];

    const ESM::NPC  *mNpc;
    std::string     mHeadModel;
    std::string     mHairModel;
    std::string     mBodyPrefix;
    bool            mHeadOnly;

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

    int mVisibilityFlags;

    int mPartslots[sPartListSize];  //Each part slot is taken by clothing, armor, or is empty
    int mPartPriorities[sPartListSize];

    NifOgre::EntityList insertBoundedPart(const std::string &mesh, int group, const std::string &bonename);

    void updateParts(bool forceupdate = false);

    void removeEntities(NifOgre::EntityList &entities);
    void removeIndividualPart(int type);
    void reserveIndividualPart(int type, int group, int priority);

    bool addOrReplaceIndividualPart(int type, int group, int priority, const std::string &mesh);
    void removePartGroup(int group);
    void addPartGroup(int group, int priority, const std::vector<ESM::PartReference> &parts);

public:
    NpcAnimation(const MWWorld::Ptr& ptr, Ogre::SceneNode* node,
                 MWWorld::InventoryStore& inv, int visibilityFlags, bool headOnly=false);
    virtual ~NpcAnimation();

    virtual Ogre::Vector3 runAnimation(float timepassed);

    Ogre::Node* getHeadNode();

    void forceUpdate()
    { updateParts(true); }
};

}

#endif

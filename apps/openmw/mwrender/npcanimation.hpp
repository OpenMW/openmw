#ifndef _GAME_RENDER_NPCANIMATION_H
#define _GAME_RENDER_NPCANIMATION_H

#include "animation.hpp"

#include "components/nifogre/ogre_nif_loader.hpp"
#include "../mwworld/inventorystore.hpp"
#include "../mwclass/npc.hpp"
#include "../mwworld/containerstore.hpp"

namespace MWRender{

class NpcAnimation: public Animation{
private:
    MWWorld::InventoryStore& mInv;
    int mStateID;
    //Free Parts
        std::pair<Ogre::Entity*, std::vector<Nif::NiTriShapeCopy>*> mChest;
        std::pair<Ogre::Entity*, std::vector<Nif::NiTriShapeCopy>*> mSkirt;
        std::pair<Ogre::Entity*, std::vector<Nif::NiTriShapeCopy>*> mLhand;
        std::pair<Ogre::Entity*, std::vector<Nif::NiTriShapeCopy>*> mRhand;
        std::pair<Ogre::Entity*, std::vector<Nif::NiTriShapeCopy>*> mTail;
        std::pair<Ogre::Entity*, std::vector<Nif::NiTriShapeCopy>*> mLFreeFoot;
        std::pair<Ogre::Entity*, std::vector<Nif::NiTriShapeCopy>*> mRFreeFoot;

        int mPartslots[27];  //Each part slot is taken by clothing, armor, or is empty
        int mPartPriorities[27];
        std::pair<Ogre::Entity*, std::vector<Nif::NiTriShapeCopy>*> mZero;

    //Bounded Parts
    Ogre::Entity* lclavicle;
    Ogre::Entity* rclavicle;
    Ogre::Entity* rupperArm;
    Ogre::Entity* lupperArm;
    Ogre::Entity* rUpperLeg;
    Ogre::Entity* lUpperLeg;
    Ogre::Entity* lForearm;
    Ogre::Entity* rForearm;
    Ogre::Entity* lWrist;
    Ogre::Entity* rWrist;
    Ogre::Entity* rKnee;
    Ogre::Entity* lKnee;
    Ogre::Entity* neck;
    Ogre::Entity* rAnkle;
    Ogre::Entity* lAnkle;
    Ogre::Entity* groin;
    Ogre::Entity* lfoot;
    Ogre::Entity* rfoot;
    Ogre::Entity* hair;
    Ogre::Entity* head;

    Ogre::SceneNode* insert;
    bool isBeast;
    bool isFemale;
    std::string headModel;
    std::string hairModel;
    std::string npcName;
    std::string bodyRaceID;
    float timeToChange;
    MWWorld::ContainerStoreIterator robe;
    MWWorld::ContainerStoreIterator helmet;
    MWWorld::ContainerStoreIterator shirt;
    MWWorld::ContainerStoreIterator cuirass;
    MWWorld::ContainerStoreIterator greaves;
    MWWorld::ContainerStoreIterator leftpauldron;
    MWWorld::ContainerStoreIterator rightpauldron;
    MWWorld::ContainerStoreIterator boots;
    MWWorld::ContainerStoreIterator pants;
    MWWorld::ContainerStoreIterator leftglove;
    MWWorld::ContainerStoreIterator rightglove;
    MWWorld::ContainerStoreIterator skirtiter;

public:
    NpcAnimation(const MWWorld::Ptr& ptr, OEngine::Render::OgreRenderer& _rend, MWWorld::InventoryStore& _inv);
    virtual ~NpcAnimation();
    Ogre::Entity* insertBoundedPart(const std::string &mesh, std::string bonename);
    std::pair<Ogre::Entity*, std::vector<Nif::NiTriShapeCopy>*> insertFreePart(const std::string &mesh, const std::string& suffix);
    void insertFootPart(int type, const std::string &mesh);
    virtual void runAnimation(float timepassed);
    void updateParts();
    void removeIndividualPart(int type);
    void reserveIndividualPart(int type, int group, int priority);

    bool addOrReplaceIndividualPart(int type, int group, int priority, const std::string &mesh);
    void removePartGroup(int group);
    void addPartGroup(int group, int priority, std::vector<ESM::PartReference>& parts);

};
}
#endif

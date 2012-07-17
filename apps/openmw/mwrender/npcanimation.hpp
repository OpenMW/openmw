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

    int mPartslots[27];  //Each part slot is taken by clothing, armor, or is empty
    int mPartPriorities[27];

    //Bounded Parts
    std::vector<Ogre::Entity*> lclavicle;
    std::vector<Ogre::Entity*> rclavicle;
    std::vector<Ogre::Entity*> rupperArm;
    std::vector<Ogre::Entity*> lupperArm;
    std::vector<Ogre::Entity*> rUpperLeg;
    std::vector<Ogre::Entity*> lUpperLeg;
    std::vector<Ogre::Entity*> lForearm;
    std::vector<Ogre::Entity*> rForearm;
    std::vector<Ogre::Entity*> lWrist;
    std::vector<Ogre::Entity*> rWrist;
    std::vector<Ogre::Entity*> rKnee;
    std::vector<Ogre::Entity*> lKnee;
    std::vector<Ogre::Entity*> neck;
    std::vector<Ogre::Entity*> rAnkle;
    std::vector<Ogre::Entity*> lAnkle;
    std::vector<Ogre::Entity*> groin;
    std::vector<Ogre::Entity*> lfoot;
    std::vector<Ogre::Entity*> rfoot;
    std::vector<Ogre::Entity*> hair;
    std::vector<Ogre::Entity*> head;

    Ogre::SceneNode* mInsert;
    Ogre::Entity *mSkelBase; // Entity with the base skeleton (temporary)
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
    std::vector<Ogre::Entity*> insertBoundedPart(const std::string &mesh, const std::string &bonename);
    virtual void runAnimation(float timepassed);
    void updateParts();
    void removeEntities(std::vector<Ogre::Entity*> &entities);
    void removeIndividualPart(int type);
    void reserveIndividualPart(int type, int group, int priority);

    bool addOrReplaceIndividualPart(int type, int group, int priority, const std::string &mesh);
    void removePartGroup(int group);
    void addPartGroup(int group, int priority, std::vector<ESM::PartReference>& parts);
};

}
#endif

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
    NifOgre::EntityList lclavicle;
    NifOgre::EntityList rclavicle;
    NifOgre::EntityList rupperArm;
    NifOgre::EntityList lupperArm;
    NifOgre::EntityList rUpperLeg;
    NifOgre::EntityList lUpperLeg;
    NifOgre::EntityList lForearm;
    NifOgre::EntityList rForearm;
    NifOgre::EntityList lWrist;
    NifOgre::EntityList rWrist;
    NifOgre::EntityList rKnee;
    NifOgre::EntityList lKnee;
    NifOgre::EntityList neck;
    NifOgre::EntityList rAnkle;
    NifOgre::EntityList lAnkle;
    NifOgre::EntityList groin;
    NifOgre::EntityList skirt;
    NifOgre::EntityList lfoot;
    NifOgre::EntityList rfoot;
    NifOgre::EntityList hair;
    NifOgre::EntityList rHand;
    NifOgre::EntityList lHand;
    NifOgre::EntityList head;
    NifOgre::EntityList chest;
    NifOgre::EntityList tail;

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
    NifOgre::EntityList insertBoundedPart(const std::string &mesh, const std::string &bonename);
    virtual void runAnimation(float timepassed);
    void updateParts();
    void removeEntities(NifOgre::EntityList &entities);
    void removeIndividualPart(int type);
    void reserveIndividualPart(int type, int group, int priority);

    bool addOrReplaceIndividualPart(int type, int group, int priority, const std::string &mesh);
    void removePartGroup(int group);
    void addPartGroup(int group, int priority, std::vector<ESM::PartReference>& parts);
};

}
#endif

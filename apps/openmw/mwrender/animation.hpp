#ifndef _GAME_RENDER_ANIMATION_H
#define _GAME_RENDER_ANIMATION_H

#include <components/nifogre/ogre_nif_loader.hpp>

#include "../mwworld/ptr.hpp"

namespace MWMechanics
{
    class CharacterController;
}

namespace MWRender
{

class Animation
{
protected:
    MWWorld::Ptr mPtr;
    MWMechanics::CharacterController *mController;

    Ogre::SceneNode* mInsert;
    NifOgre::EntityList mEntityList;
    std::map<std::string,NifOgre::TextKeyMap> mTextKeys;
    Ogre::Bone *mAccumRoot;
    Ogre::Bone *mNonAccumRoot;
    Ogre::Vector3 mAccumulate;
    Ogre::Vector3 mStartPosition;
    Ogre::Vector3 mLastPosition;

    NifOgre::TextKeyMap *mCurrentKeys;
    NifOgre::TextKeyMap::const_iterator mNextKey;
    Ogre::AnimationState *mAnimState;

    /* Updates the animation to the specified time, and moves the mPtr object
     * based on the change since the last update or reset. */
    void updatePosition(float time);
    /* Updates the animation to the specified time, without moving the mPtr
     * object. */
    void resetPosition(float time);

    float findStart(const std::string &groupname, const std::string &start);

    void createEntityList(Ogre::SceneNode *node, const std::string &model);

public:
    Animation(const MWWorld::Ptr &ptr);
    virtual ~Animation();

    void setController(MWMechanics::CharacterController *controller);
    std::vector<std::string> getAnimationNames();

    // Specifies the axis' to accumulate on. Non-accumulated axis will just
    // move visually, but not affect the actual movement. Each x/y/z value
    // should be on the scale of 0 to 1.
    void setAccumulation(const Ogre::Vector3 &accum);

    void play(const std::string &groupname, const std::string &start);
    virtual void runAnimation(float timepassed);
};

}
#endif

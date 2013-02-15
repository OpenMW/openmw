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
    Ogre::Node *mAccumRoot;
    Ogre::Bone *mNonAccumRoot;
    Ogre::Vector3 mAccumulate;
    Ogre::Vector3 mLastPosition;

    std::vector<Ogre::SkeletonPtr> mSkeletonSources;

    NifOgre::TextKeyMap *mCurrentKeys;
    NifOgre::TextKeyMap::const_iterator mNextKey;
    Ogre::Animation *mCurrentAnim;
    float mCurrentTime;
    bool mPlaying;
    bool mLooping;

    float mAnimVelocity;
    float mAnimSpeedMult;

    /* Applies the given animation to the given skeleton instance, using the specified time. */
    void applyAnimation(const Ogre::Animation *anim, float time, Ogre::SkeletonInstance *skel);

    /* Updates a skeleton instance so that all bones matching the source skeleton (based on
     * bone names) are positioned identically. */
    void updateSkeletonInstance(const Ogre::SkeletonInstance *skelsrc, Ogre::SkeletonInstance *skel);


    /* Updates the animation to the specified time, and returns the movement
     * vector since the last update or reset. */
    Ogre::Vector3 updatePosition(float time);

    /* Resets the animation to the time of the specified marker, without moving
     * anything. If the marker is not found, it resets to the beginning. */
    void reset(const std::string &marker);

    /* Inserts an additional skeleton into the animation source chain. Returns
     * the bone representing the non-accum root from the base skeleton. */
    Ogre::Bone *insertSkeletonSource(const std::string &name);

    void createEntityList(Ogre::SceneNode *node, const std::string &model);

public:
    Animation(const MWWorld::Ptr &ptr);
    virtual ~Animation();

    void setController(MWMechanics::CharacterController *controller);

    bool hasAnimation(const std::string &anim);

    // Specifies the axis' to accumulate on. Non-accumulated axis will just
    // move visually, but not affect the actual movement. Each x/y/z value
    // should be on the scale of 0 to 1.
    void setAccumulation(const Ogre::Vector3 &accum);

    void setSpeed(float speed);

    void play(const std::string &groupname, const std::string &start, bool loop);
    virtual Ogre::Vector3 runAnimation(float timepassed);
};

}
#endif

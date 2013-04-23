#ifndef _GAME_RENDER_ANIMATION_H
#define _GAME_RENDER_ANIMATION_H

#include <OgreController.h>
#include <OgreVector3.h>

#include <components/nifogre/ogrenifloader.hpp>

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
    class AnimationValue : public Ogre::ControllerValue<Ogre::Real>
    {
    private:
        Animation *mAnimation;

    public:
        AnimationValue(Animation *anim) : mAnimation(anim)
        { }

        virtual Ogre::Real getValue() const
        {
            return mAnimation->mCurrentTime;
        }

        virtual void setValue(Ogre::Real value)
        {
            mAnimation->mCurrentTime = value;
        }
    };
    Ogre::SharedPtr<Ogre::ControllerValue<Ogre::Real> > mAnimationBaseValuePtr;

    MWWorld::Ptr mPtr;
    MWMechanics::CharacterController *mController;

    Ogre::SceneNode *mInsert;
    Ogre::Entity *mSkelBase;
    std::vector<NifOgre::ObjectList> mObjectLists;
    Ogre::Node *mAccumRoot;
    Ogre::Bone *mNonAccumRoot;
    Ogre::Vector3 mAccumulate;
    Ogre::Vector3 mLastPosition;

    std::string mCurrentGroup;
    std::vector<Ogre::Controller<Ogre::Real> > *mCurrentControllers;
    const NifOgre::TextKeyMap *mCurrentKeys;
    NifOgre::TextKeyMap::const_iterator mStartKey;
    NifOgre::TextKeyMap::const_iterator mLoopStartKey;
    NifOgre::TextKeyMap::const_iterator mStopKey;
    NifOgre::TextKeyMap::const_iterator mNextKey;
    float mCurrentTime;
    bool mPlaying;
    bool mLooping;

    NifOgre::NodeTargetValue<Ogre::Real> *mNonAccumCtrl;
    float mAnimVelocity;
    float mAnimSpeedMult;

    static float calcAnimVelocity(const NifOgre::TextKeyMap &keys,
                                  NifOgre::NodeTargetValue<Ogre::Real> *nonaccumctrl,
                                  const std::string &groupname);

    /* Updates a skeleton instance so that all bones matching the source skeleton (based on
     * bone names) are positioned identically. */
    void updateSkeletonInstance(const Ogre::SkeletonInstance *skelsrc, Ogre::SkeletonInstance *skel);

    /* Updates the position of the accum root node for the current time, and
     * returns the wanted movement vector from the previous update. */
    Ogre::Vector3 updatePosition();

    /* Resets the animation to the time of the specified start marker, without
     * moving anything, and set the end time to the specified stop marker. If
     * the marker is not found, or if the markers are the same, it returns
     * false.
     */
    bool reset(const NifOgre::TextKeyMap &keys, NifOgre::NodeTargetValue<Ogre::Real> *nonaccumctrl, const std::string &groupname, const std::string &start, const std::string &stop);

    void doLoop();

    bool handleTextKey(const NifOgre::TextKeyMap::const_iterator &key);

    void addObjectList(Ogre::SceneNode *node, const std::string &model, bool baseonly);
    static void destroyObjectList(Ogre::SceneManager *sceneMgr, NifOgre::ObjectList &objects);

    static void setRenderProperties(const NifOgre::ObjectList &objlist, Ogre::uint32 visflags, Ogre::uint8 solidqueue, Ogre::uint8 transqueue);

public:
    Animation(const MWWorld::Ptr &ptr);
    virtual ~Animation();

    void setController(MWMechanics::CharacterController *controller);

    void updatePtr(const MWWorld::Ptr &ptr);

    bool hasAnimation(const std::string &anim);

    // Specifies the axis' to accumulate on. Non-accumulated axis will just
    // move visually, but not affect the actual movement. Each x/y/z value
    // should be on the scale of 0 to 1.
    void setAccumulation(const Ogre::Vector3 &accum);

    void setSpeed(float speed);

    void setLooping(bool loop);

    void play(const std::string &groupname, const std::string &start, const std::string &stop, bool loop);
    virtual Ogre::Vector3 runAnimation(float timepassed);

    Ogre::Node *getNode(const std::string &name);
};

}
#endif

#ifndef _GAME_RENDER_ANIMATION_H
#define _GAME_RENDER_ANIMATION_H

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

    MWWorld::Ptr mPtr;
    MWMechanics::CharacterController *mController;

    Ogre::SceneNode* mInsert;
    NifOgre::ObjectList mObjectList;
    std::map<std::string,NifOgre::TextKeyMap> mTextKeys;
    Ogre::Node *mAccumRoot;
    Ogre::Bone *mNonAccumRoot;
    Ogre::Vector3 mAccumulate;
    Ogre::Vector3 mLastPosition;

    std::vector<NifOgre::ObjectList> mAnimationSources;

    std::vector<Ogre::Controller<Ogre::Real> > *mCurrentControllers;
    NifOgre::TextKeyMap *mCurrentKeys;
    NifOgre::TextKeyMap::const_iterator mNextKey;
    Ogre::Animation *mCurrentAnim;
    float mCurrentTime;
    float mStopTime;
    bool mPlaying;
    bool mLooping;

    float mAnimVelocity;
    float mAnimSpeedMult;

    void calcAnimVelocity();

    /* Updates a skeleton instance so that all bones matching the source skeleton (based on
     * bone names) are positioned identically. */
    void updateSkeletonInstance(const Ogre::SkeletonInstance *skelsrc, Ogre::SkeletonInstance *skel);

    /* Updates the position of the accum root node for the current time, and
     * returns the wanted movement vector from the previous update. */
    Ogre::Vector3 updatePosition();

    /* Resets the animation to the time of the specified start marker, without
     * moving anything, and set the end time to the specified stop marker. If
     * the marker is not found, it resets to the beginning or end respectively.
     */
    void reset(const std::string &start, const std::string &stop);

    bool handleEvent(float time, const std::string &evt);

    /* Specifies a list of skeleton names to use as animation sources. */
    void setAnimationSources(const std::vector<std::string> &names);

    /* Specifies a single skeleton name to use as an animation source. */
    void setAnimationSource(const std::string &name)
    {
        std::vector<std::string> names(1, name);
        setAnimationSources(names);
    }

    void createObjectList(Ogre::SceneNode *node, const std::string &model);
    static void destroyObjectList(Ogre::SceneManager *sceneMgr, NifOgre::ObjectList &objects);

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

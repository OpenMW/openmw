#ifndef _GAME_RENDER_ANIMATION_H
#define _GAME_RENDER_ANIMATION_H

#include <OgreController.h>
#include <OgreVector3.h>

#include <components/nifogre/ogrenifloader.hpp>

#include "../mwworld/ptr.hpp"


namespace MWRender
{

class Animation
{
public:
    enum Group {
        Group_LowerBody = 1<<0,

        Group_Torso = 1<<1,
        Group_LeftArm = 1<<2,
        Group_RightArm = 1<<3,

        Group_UpperBody = Group_Torso | Group_LeftArm | Group_RightArm,

        Group_All = Group_LowerBody | Group_UpperBody
    };

protected:
    /* This is the number of *discrete* groups. */
    static const size_t sNumGroups = 4;

    class AnimationValue : public Ogre::ControllerValue<Ogre::Real>
    {
    private:
        Animation *mAnimation;
        std::string mAnimationName;

    public:
        AnimationValue(Animation *anim)
          : mAnimation(anim)
        { }

        void setAnimName(const std::string &name)
        { mAnimationName = name; }
        const std::string &getAnimName() const
        { return mAnimationName; }

        virtual Ogre::Real getValue() const;
        virtual void setValue(Ogre::Real value);
    };

    struct AnimSource : public Ogre::AnimationAlloc {
        NifOgre::TextKeyMap mTextKeys;
        std::vector<Ogre::Controller<Ogre::Real> > mControllers[sNumGroups];
    };
    typedef std::vector< Ogre::SharedPtr<AnimSource> > AnimSourceList;

    struct AnimState {
        Ogre::SharedPtr<AnimSource> mSource;
        NifOgre::TextKeyMap::const_iterator mStartKey;
        NifOgre::TextKeyMap::const_iterator mLoopStartKey;
        NifOgre::TextKeyMap::const_iterator mStopKey;
        NifOgre::TextKeyMap::const_iterator mNextKey;

        float mTime;

        bool mPlaying;
        size_t mLoopCount;

        int mPriority;
        int mGroups;
        bool mAutoDisable;

        AnimState() : mTime(0.0f), mPlaying(false), mLoopCount(0),
                      mPriority(0), mGroups(0), mAutoDisable(true)
        { }
    };
    typedef std::map<std::string,AnimState> AnimStateMap;

    MWWorld::Ptr mPtr;

    Ogre::SceneNode *mInsert;
    Ogre::Entity *mSkelBase;
    NifOgre::ObjectList mObjectRoot;
    AnimSourceList mAnimSources;
    Ogre::Node *mAccumRoot;
    Ogre::Node *mNonAccumRoot;
    NifOgre::NodeTargetValue<Ogre::Real> *mNonAccumCtrl;
    Ogre::Vector3 mAccumulate;

    AnimStateMap mStates;

    Ogre::SharedPtr<AnimationValue> mAnimationValuePtr[sNumGroups];

    float mAnimVelocity;
    float mAnimSpeedMult;

    /* Sets the appropriate animations on the bone groups based on priority.
     */
    void resetActiveGroups();

    static size_t detectAnimGroup(const Ogre::Node *node);

    static float calcAnimVelocity(const NifOgre::TextKeyMap &keys,
                                  NifOgre::NodeTargetValue<Ogre::Real> *nonaccumctrl,
                                  const Ogre::Vector3 &accum,
                                  const std::string &groupname);

    /* Updates a skeleton instance so that all bones matching the source skeleton (based on
     * bone names) are positioned identically. */
    void updateSkeletonInstance(const Ogre::SkeletonInstance *skelsrc, Ogre::SkeletonInstance *skel);

    /* Updates the position of the accum root node for the given time, and
     * returns the wanted movement vector from the previous time. */
    void updatePosition(float oldtime, float newtime, Ogre::Vector3 &position);

    static NifOgre::TextKeyMap::const_iterator findGroupStart(const NifOgre::TextKeyMap &keys, const std::string &groupname);

    /* Resets the animation to the time of the specified start marker, without
     * moving anything, and set the end time to the specified stop marker. If
     * the marker is not found, or if the markers are the same, it returns
     * false.
     */
    bool reset(AnimState &state, const NifOgre::TextKeyMap &keys,
               const std::string &groupname, const std::string &start, const std::string &stop,
               float startpoint);

    bool doLoop(AnimState &state);

    bool handleTextKey(AnimState &state, const std::string &groupname, const NifOgre::TextKeyMap::const_iterator &key);

    void setObjectRoot(Ogre::SceneNode *node, const std::string &model, bool baseonly);
    void addAnimSource(const std::string &model);

    static void destroyObjectList(Ogre::SceneManager *sceneMgr, NifOgre::ObjectList &objects);

    static void setRenderProperties(const NifOgre::ObjectList &objlist, Ogre::uint32 visflags, Ogre::uint8 solidqueue, Ogre::uint8 transqueue);

    void clearAnimSources();

public:
    Animation(const MWWorld::Ptr &ptr);
    virtual ~Animation();

    void updatePtr(const MWWorld::Ptr &ptr);

    bool hasAnimation(const std::string &anim);

    // Specifies the axis' to accumulate on. Non-accumulated axis will just
    // move visually, but not affect the actual movement. Each x/y/z value
    // should be on the scale of 0 to 1.
    void setAccumulation(const Ogre::Vector3 &accum);

    void setSpeed(float speed);

    /** Plays an animation.
     * \param groupname Name of the animation group to play.
     * \param priority Priority of the animation. The animation will play on
     *                 bone groups that don't have another animation set of a
     *                 higher priority.
     * \param groups Bone groups to play the animation on.
     * \param autodisable Automatically disable the animation when it stops
     *                    playing.
     * \param start Key marker from which to start.
     * \param stop Key marker to stop at.
     * \param startpoint How far in between the two markers to start. 0 starts
     *                   at the start marker, 1 starts at the stop marker.
     * \param loops How many times to loop the animation. This will use the
     *              "loop start" and "loop stop" markers if they exist,
     *              otherwise it will use "start" and "stop".
     */
    void play(const std::string &groupname, int priority, int groups, bool autodisable,
              const std::string &start, const std::string &stop,
              float startpoint, size_t loops);

    /** Returns true if the named animation group is playing. */
    bool isPlaying(const std::string &groupname) const;

    /** Gets info about the given animation group.
     * \param groupname Animation group to check.
     * \param complete Stores completion amount (0 = at start key, 0.5 = half way between start and stop keys), etc.
     * \param start Stores the start key
     * \param stop Stores the stop key
     * \return True if the animation is active, false otherwise.
     */
    bool getInfo(const std::string &groupname, float *complete=NULL, std::string *start=NULL, std::string *stop=NULL) const;

    /** Disables the specified animation group;
     * \param groupname Animation group to disable.
     */
    void disable(const std::string &groupname);

    virtual Ogre::Vector3 runAnimation(float duration);

    virtual void showWeapons(bool showWeapon);

    Ogre::Node *getNode(const std::string &name);
};

}
#endif

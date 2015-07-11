#ifndef GAME_RENDER_ANIMATION_H
#define GAME_RENDER_ANIMATION_H

#include "../mwworld/ptr.hpp"

#include <components/sceneutil/controller.hpp>

namespace ESM
{
    struct Light;
}

namespace Resource
{
    class ResourceSystem;
}

namespace NifOsg
{
    class KeyframeHolder;
    class KeyframeController;
}

namespace SceneUtil
{
    class LightSource;
}

namespace MWRender
{

class ResetAccumRootCallback;
class RotateController;

class EffectAnimationTime : public SceneUtil::ControllerSource
{
private:
    float mTime;
public:
    virtual float getValue(osg::NodeVisitor* nv);

    void addTime(float duration);
    void resetTime(float time);
    float getTime() const;

    EffectAnimationTime() : mTime(0) {  }
};

/// @brief Detaches the node from its parent when the object goes out of scope.
class PartHolder
{
public:
    PartHolder(osg::ref_ptr<osg::Node> node);

    ~PartHolder();

    osg::ref_ptr<osg::Node> getNode()
    {
        return mNode;
    }

private:
    osg::ref_ptr<osg::Node> mNode;
};
typedef std::shared_ptr<PartHolder> PartHolderPtr;

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

    class TextKeyListener
    {
    public:
        virtual void handleTextKey(const std::string &groupname, const std::multimap<float, std::string>::const_iterator &key,
                           const std::multimap<float, std::string>& map) = 0;
    };

    void setTextKeyListener(TextKeyListener* listener);

protected:
    /* This is the number of *discrete* groups. */
    static const size_t sNumGroups = 4;

    class AnimationTime : public SceneUtil::ControllerSource
    {
    private:
        std::shared_ptr<float> mTimePtr;

    public:

        void setTimePtr(std::shared_ptr<float> time)
        { mTimePtr = time; }
        std::shared_ptr<float> getTimePtr() const
        { return mTimePtr; }

        virtual float getValue(osg::NodeVisitor* nv);
    };

    class NullAnimationTime : public SceneUtil::ControllerSource
    {
    public:
        virtual float getValue(osg::NodeVisitor *nv)
        {
            return 0.f;
        }
    };

    struct AnimSource;

    struct AnimState {
        std::shared_ptr<AnimSource> mSource;
        float mStartTime;
        float mLoopStartTime;
        float mLoopStopTime;
        float mStopTime;

        typedef std::shared_ptr<float> TimePtr;
        TimePtr mTime;
        float mSpeedMult;

        bool mPlaying;
        size_t mLoopCount;

        int mPriority;
        int mGroups;
        bool mAutoDisable;

        AnimState() : mStartTime(0.0f), mLoopStartTime(0.0f), mLoopStopTime(0.0f), mStopTime(0.0f),
                      mTime(new float), mSpeedMult(1.0f), mPlaying(false), mLoopCount(0),
                      mPriority(0), mGroups(0), mAutoDisable(true)
        {
        }
        ~AnimState();

        float getTime() const
        {
            return *mTime;
        }
        void setTime(float time)
        {
            *mTime = time;
        }
    };
    typedef std::map<std::string,AnimState> AnimStateMap;
    AnimStateMap mStates;

    typedef std::vector<std::shared_ptr<AnimSource> > AnimSourceList;
    AnimSourceList mAnimSources;

    osg::ref_ptr<osg::Group> mInsert;

    osg::ref_ptr<osg::Node> mObjectRoot;

    // The node expected to accumulate movement during movement animations.
    osg::ref_ptr<osg::Node> mAccumRoot;

    // The controller animating that node.
    osg::ref_ptr<NifOsg::KeyframeController> mAccumCtrl;

    // Used to reset the position of the accumulation root every frame - the movement should be applied to the physics system
    osg::ref_ptr<ResetAccumRootCallback> mResetAccumRootCallback;

    // Keep track of controllers that we added to our scene graph.
    // We may need to rebuild these controllers when the active animation groups / sources change.
    typedef std::multimap<osg::ref_ptr<osg::Node>, osg::ref_ptr<osg::NodeCallback> > ControllerMap;
    ControllerMap mActiveControllers;

    std::shared_ptr<AnimationTime> mAnimationTimePtr[sNumGroups];

    // Stored in all lowercase for a case-insensitive lookup
    typedef std::map<std::string, osg::ref_ptr<osg::MatrixTransform> > NodeMap;
    NodeMap mNodeMap;

    MWWorld::Ptr mPtr;

    Resource::ResourceSystem* mResourceSystem;

    osg::Vec3f mAccumulate;

    struct EffectParams
    {
        std::string mModelName; // Just here so we don't add the same effect twice
        PartHolderPtr mObjects;
        std::shared_ptr<EffectAnimationTime> mAnimTime;
        float mMaxControllerLength;
        int mEffectId;
        bool mLoop;
        std::string mBoneName;
    };

    std::vector<EffectParams> mEffects;

    TextKeyListener* mTextKeyListener;

    osg::ref_ptr<RotateController> mHeadController;
    float mHeadYawRadians;
    float mHeadPitchRadians;

    osg::ref_ptr<SceneUtil::LightSource> mGlowLight;

    /* Sets the appropriate animations on the bone groups based on priority.
     */
    void resetActiveGroups();

    size_t detectAnimGroup(osg::Node* node);

    /* Updates the position of the accum root node for the given time, and
     * returns the wanted movement vector from the previous time. */
    void updatePosition(float oldtime, float newtime, osg::Vec3f& position);

    /* Resets the animation to the time of the specified start marker, without
     * moving anything, and set the end time to the specified stop marker. If
     * the marker is not found, or if the markers are the same, it returns
     * false.
     */
    bool reset(AnimState &state, const std::multimap<float, std::string> &keys,
               const std::string &groupname, const std::string &start, const std::string &stop,
               float startpoint, bool loopfallback);

    void handleTextKey(AnimState &state, const std::string &groupname, const std::multimap<float, std::string>::const_iterator &key,
                       const std::multimap<float, std::string>& map);

    /** Sets the root model of the object.
     *
     * Note that you must make sure all animation sources are cleared before reseting the object
     * root. All nodes previously retrieved with getNode will also become invalidated.
     * @param forceskeleton Wrap the object root in a Skeleton, even if it contains no skinned parts. Use this if you intend to add skinned parts manually.
     * @param baseonly If true, then any meshes or particle systems in the model are ignored
     *      (useful for NPCs, where only the skeleton is needed for the root, and the actual NPC parts are then assembled from separate files).
     */
    void setObjectRoot(const std::string &model, bool forceskeleton, bool baseonly);

    /* Adds the keyframe controllers in the specified model as a new animation source. Note that
     * the filename portion of the provided model name will be prepended with 'x', and the .nif
     * extension will be replaced with .kf. */
    void addAnimSource(const std::string &model);

    /** Adds an additional light to the given node using the specified ESM record. */
    void addExtraLight(osg::ref_ptr<osg::Group> parent, const ESM::Light *light);

    void clearAnimSources();

    /**
     * Provided to allow derived classes adding their own controllers. Note, the controllers must be added to mActiveControllers
     * so they get cleaned up properly on the next controller rebuild. A controller rebuild may be necessary to ensure correct ordering.
     */
    virtual void addControllers();

    osg::Vec4f getEnchantmentColor(MWWorld::Ptr item);

    void addGlow(osg::ref_ptr<osg::Node> node, osg::Vec4f glowColor);

public:

    Animation(const MWWorld::Ptr &ptr, osg::ref_ptr<osg::Group> parentNode, Resource::ResourceSystem* resourceSystem);
    virtual ~Animation();

    MWWorld::Ptr getPtr();

    /// Set active flag on the object skeleton, if one exists.
    /// @see SceneUtil::Skeleton::setActive
    void setActive(bool active);

    osg::Group* getOrCreateObjectRoot();

    osg::Group* getObjectRoot();

    /**
     * @brief Add an effect mesh attached to a bone or the insert scene node
     * @param model
     * @param effectId An ID for this effect by which you can identify it later. If this is not wanted, set to -1.
     * @param loop Loop the effect. If false, it is removed automatically after it finishes playing. If true,
     *              you need to remove it manually using removeEffect when the effect should end.
     * @param bonename Bone to attach to, or empty string to use the scene node instead
     * @param texture override the texture specified in the model's materials - if empty, do not override
     * @note Will not add an effect twice.
     */
    void addEffect (const std::string& model, int effectId, bool loop = false, const std::string& bonename = "", std::string texture = "");
    void removeEffect (int effectId);
    void getLoopingEffects (std::vector<int>& out);

    virtual void updatePtr(const MWWorld::Ptr &ptr);

    bool hasAnimation(const std::string &anim);

    // Specifies the axis' to accumulate on. Non-accumulated axis will just
    // move visually, but not affect the actual movement. Each x/y/z value
    // should be on the scale of 0 to 1.
    void setAccumulation(const osg::Vec3f& accum);

    /** Plays an animation.
     * \param groupname Name of the animation group to play.
     * \param priority Priority of the animation. The animation will play on
     *                 bone groups that don't have another animation set of a
     *                 higher priority.
     * \param groups Bone groups to play the animation on.
     * \param autodisable Automatically disable the animation when it stops
     *                    playing.
     * \param speedmult Speed multiplier for the animation.
     * \param start Key marker from which to start.
     * \param stop Key marker to stop at.
     * \param startpoint How far in between the two markers to start. 0 starts
     *                   at the start marker, 1 starts at the stop marker.
     * \param loops How many times to loop the animation. This will use the
     *              "loop start" and "loop stop" markers if they exist,
     *              otherwise it may fall back to "start" and "stop", but only if
     *              the \a loopFallback parameter is true.
     * \param loopFallback Allow looping an animation that has no loop keys, i.e. fall back to use
     *                     the "start" and "stop" keys for looping?
     */
    void play(const std::string &groupname, int priority, int groups, bool autodisable,
              float speedmult, const std::string &start, const std::string &stop,
              float startpoint, size_t loops, bool loopfallback=false);

    /** If the given animation group is currently playing, set its remaining loop count to '0'.
     */
    void stopLooping(const std::string& groupName);

    /** Adjust the speed multiplier of an already playing animation.
     */
    void adjustSpeedMult (const std::string& groupname, float speedmult);

    /** Returns true if the named animation group is playing. */
    bool isPlaying(const std::string &groupname) const;

    /// Returns true if no important animations are currently playing on the upper body.
    bool upperBodyReady() const;

    /** Gets info about the given animation group.
     * \param groupname Animation group to check.
     * \param complete Stores completion amount (0 = at start key, 0.5 = half way between start and stop keys), etc.
     * \param speedmult Stores the animation speed multiplier
     * \return True if the animation is active, false otherwise.
     */
    bool getInfo(const std::string &groupname, float *complete=nullptr, float *speedmult=nullptr) const;

    /// Get the absolute position in the animation track of the first text key with the given group.
    float getStartTime(const std::string &groupname) const;

    /// Get the absolute position in the animation track of the text key
    float getTextKeyTime(const std::string &textKey) const;

    /// Get the current absolute position in the animation track for the animation that is currently playing from the given group.
    float getCurrentTime(const std::string& groupname) const;

    /** Disables the specified animation group;
     * \param groupname Animation group to disable.
     */
    void disable(const std::string &groupname);
    void changeGroups(const std::string &groupname, int group);

    /** Retrieves the velocity (in units per second) that the animation will move. */
    float getVelocity(const std::string &groupname) const;

    virtual osg::Vec3f runAnimation(float duration);

    /// This is typically called as part of runAnimation, but may be called manually if needed.
    void updateEffects(float duration);

    /// Return a node with the specified name, or nullptr if not existing.
    /// @note The matching is case-insensitive.
    const osg::Node* getNode(const std::string& name) const;

    virtual void showWeapons(bool showWeapon) {}
    virtual void showCarriedLeft(bool show) {}
    virtual void setWeaponGroup(const std::string& group) {}
    virtual void setVampire(bool vampire) {}
    virtual void setAlpha(float alpha) {}
    virtual void setPitchFactor(float factor) {}
    virtual void attachArrow() {}
    virtual void releaseArrow(float attackStrength) {}
    virtual void enableHeadAnimation(bool enable) {}
    // TODO: move outside of this class
    /// Makes this object glow, by placing a Light in its center.
    /// @param effect Controls the radius and intensity of the light.
    virtual void setLightEffect(float effect);

    virtual void setHeadPitch(float pitchRadians);
    virtual void setHeadYaw(float yawRadians);
    virtual float getHeadPitch() const;
    virtual float getHeadYaw() const;

private:
    Animation(const Animation&);
    void operator=(Animation&);
};

class ObjectAnimation : public Animation {
public:
    ObjectAnimation(const MWWorld::Ptr& ptr, const std::string &model, Resource::ResourceSystem* resourceSystem, bool animated, bool allowLight);
};

}
#endif

#ifndef GAME_RENDER_ANIMATION_H
#define GAME_RENDER_ANIMATION_H

#include "../mwworld/ptr.hpp"

#include <components/sceneutil/controller.hpp>
#include <components/sceneutil/util.hpp>

namespace ESM
{
    struct Light;
    struct MagicEffect;
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
    class LightListCallback;
    class Skeleton;
}

namespace MWRender
{

class ResetAccumRootCallback;
class RotateController;
class TransparencyUpdater;

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

    void operator= (const PartHolder&);
    PartHolder(const PartHolder&);
};
typedef std::shared_ptr<PartHolder> PartHolderPtr;

struct EffectParams
{
    std::string mModelName; // Just here so we don't add the same effect twice
    std::shared_ptr<EffectAnimationTime> mAnimTime;
    float mMaxControllerLength;
    int mEffectId;
    bool mLoop;
    std::string mBoneName;
};

class Animation : public osg::Referenced
{
public:
    enum BoneGroup {
        BoneGroup_LowerBody = 0,
        BoneGroup_Torso,
        BoneGroup_LeftArm,
        BoneGroup_RightArm
    };

    enum BlendMask {
        BlendMask_LowerBody = 1<<0,
        BlendMask_Torso = 1<<1,
        BlendMask_LeftArm = 1<<2,
        BlendMask_RightArm = 1<<3,

        BlendMask_UpperBody = BlendMask_Torso | BlendMask_LeftArm | BlendMask_RightArm,

        BlendMask_All = BlendMask_LowerBody | BlendMask_UpperBody
    };
    /* This is the number of *discrete* blend masks. */
    static const size_t sNumBlendMasks = 4;

    /// Holds an animation priority value for each BoneGroup.
    struct AnimPriority
    {
        /// Convenience constructor, initialises all priorities to the same value.
        AnimPriority(int priority)
        {
            for (unsigned int i=0; i<sNumBlendMasks; ++i)
                mPriority[i] = priority;
        }

        bool operator == (const AnimPriority& other) const
        {
            for (unsigned int i=0; i<sNumBlendMasks; ++i)
                if (other.mPriority[i] != mPriority[i])
                    return false;
            return true;
        }

        int& operator[] (BoneGroup n)
        {
            return mPriority[n];
        }

        const int& operator[] (BoneGroup n) const
        {
            return mPriority[n];
        }

        bool contains(int priority) const
        {
            for (unsigned int i=0; i<sNumBlendMasks; ++i)
                if (priority == mPriority[i])
                    return true;
            return false;
        }

        int mPriority[sNumBlendMasks];
    };

    class TextKeyListener
    {
    public:
        virtual void handleTextKey(const std::string &groupname, const std::multimap<float, std::string>::const_iterator &key,
                           const std::multimap<float, std::string>& map) = 0;

        virtual ~TextKeyListener() = default;
    };

    void setTextKeyListener(TextKeyListener* listener);

    virtual bool updateCarriedLeftVisible(const int weaptype) const { return false; };

protected:
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
        bool mLoopingEnabled;
        size_t mLoopCount;

        AnimPriority mPriority;
        int mBlendMask;
        bool mAutoDisable;

        AnimState() : mStartTime(0.0f), mLoopStartTime(0.0f), mLoopStopTime(0.0f), mStopTime(0.0f),
                      mTime(new float), mSpeedMult(1.0f), mPlaying(false), mLoopingEnabled(true),
                      mLoopCount(0), mPriority(0), mBlendMask(0), mAutoDisable(true)
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

        bool shouldLoop() const
        {
            return getTime() >= mLoopStopTime && mLoopingEnabled && mLoopCount > 0;
        }
    };
    typedef std::map<std::string,AnimState> AnimStateMap;
    AnimStateMap mStates;

    typedef std::vector<std::shared_ptr<AnimSource> > AnimSourceList;
    AnimSourceList mAnimSources;

    osg::ref_ptr<osg::Group> mInsert;

    osg::ref_ptr<osg::Group> mObjectRoot;
    SceneUtil::Skeleton* mSkeleton;

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

    std::shared_ptr<AnimationTime> mAnimationTimePtr[sNumBlendMasks];

    // Stored in all lowercase for a case-insensitive lookup
    typedef std::map<std::string, osg::ref_ptr<osg::MatrixTransform> > NodeMap;
    mutable NodeMap mNodeMap;
    mutable bool mNodeMapCreated;

    MWWorld::Ptr mPtr;

    Resource::ResourceSystem* mResourceSystem;

    osg::Vec3f mAccumulate;

    TextKeyListener* mTextKeyListener;

    osg::ref_ptr<RotateController> mHeadController;
    float mHeadYawRadians;
    float mHeadPitchRadians;
    bool mHasMagicEffects;

    osg::ref_ptr<SceneUtil::LightSource> mGlowLight;
    osg::ref_ptr<SceneUtil::GlowUpdater> mGlowUpdater;
    osg::ref_ptr<TransparencyUpdater> mTransparencyUpdater;

    float mAlpha;

    mutable std::map<std::string, float> mAnimVelocities;

    osg::ref_ptr<SceneUtil::LightListCallback> mLightListCallback;

    const NodeMap& getNodeMap() const;

    /* Sets the appropriate animations on the bone groups based on priority.
     */
    void resetActiveGroups();

    size_t detectBlendMask(const osg::Node* node) const;

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
     * Note that you must make sure all animation sources are cleared before resetting the object
     * root. All nodes previously retrieved with getNode will also become invalidated.
     * @param forceskeleton Wrap the object root in a Skeleton, even if it contains no skinned parts. Use this if you intend to add skinned parts manually.
     * @param baseonly If true, then any meshes or particle systems in the model are ignored
     *      (useful for NPCs, where only the skeleton is needed for the root, and the actual NPC parts are then assembled from separate files).
     */
    void setObjectRoot(const std::string &model, bool forceskeleton, bool baseonly, bool isCreature);

    void loadAllAnimationsInFolder(const std::string &model, const std::string &baseModel);

    /** Adds the keyframe controllers in the specified model as a new animation source.
     * @note Later added animation sources have the highest priority when it comes to finding a particular animation.
     * @param model The file to add the keyframes for. Note that the .nif file extension will be replaced with .kf.
     * @param baseModel The filename of the mObjectRoot, only used for error messages.
    */
    void addAnimSource(const std::string &model, const std::string& baseModel);
    void addSingleAnimSource(const std::string &model, const std::string& baseModel);

    /** Adds an additional light to the given node using the specified ESM record. */
    void addExtraLight(osg::ref_ptr<osg::Group> parent, const ESM::Light *light);

    void clearAnimSources();

    /**
     * Provided to allow derived classes adding their own controllers. Note, the controllers must be added to mActiveControllers
     * so they get cleaned up properly on the next controller rebuild. A controller rebuild may be necessary to ensure correct ordering.
     */
    virtual void addControllers();

    /// Set the render bin for this animation's object root. May be customized by subclasses.
    virtual void setRenderBin();

public:

    Animation(const MWWorld::Ptr &ptr, osg::ref_ptr<osg::Group> parentNode, Resource::ResourceSystem* resourceSystem);

    /// Must be thread safe
    virtual ~Animation();

    MWWorld::ConstPtr getPtr() const;

    MWWorld::Ptr getPtr();

    /// Set active flag on the object skeleton, if one exists.
    /// @see SceneUtil::Skeleton::setActive
    /// 0 = Inactive, 1 = Active in place, 2 = Active
    void setActive(int active);

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
    void addEffect (const std::string& model, int effectId, bool loop = false, const std::string& bonename = "", const std::string& texture = "");
    void removeEffect (int effectId);
    void removeEffects ();
    void getLoopingEffects (std::vector<int>& out) const;

    // Add a spell casting glow to an object. From measuring video taken from the original engine,
    // the glow seems to be about 1.5 seconds except for telekinesis, which is 1 second.
    void addSpellCastGlow(const ESM::MagicEffect *effect, float glowDuration = 1.5);

    virtual void updatePtr(const MWWorld::Ptr &ptr);

    bool hasAnimation(const std::string &anim) const;

    // Specifies the axis' to accumulate on. Non-accumulated axis will just
    // move visually, but not affect the actual movement. Each x/y/z value
    // should be on the scale of 0 to 1.
    void setAccumulation(const osg::Vec3f& accum);

    /** Plays an animation.
     * \param groupname Name of the animation group to play.
     * \param priority Priority of the animation. The animation will play on
     *                 bone groups that don't have another animation set of a
     *                 higher priority.
     * \param blendMask Bone groups to play the animation on.
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
    void play(const std::string &groupname, const AnimPriority& priority, int blendMask, bool autodisable,
              float speedmult, const std::string &start, const std::string &stop,
              float startpoint, size_t loops, bool loopfallback=false);

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

    size_t getCurrentLoopCount(const std::string& groupname) const;

    /** Disables the specified animation group;
     * \param groupname Animation group to disable.
     */
    void disable(const std::string &groupname);

    /** Retrieves the velocity (in units per second) that the animation will move. */
    float getVelocity(const std::string &groupname) const;

    virtual osg::Vec3f runAnimation(float duration);

    void setLoopingEnabled(const std::string &groupname, bool enabled);

    /// This is typically called as part of runAnimation, but may be called manually if needed.
    void updateEffects();

    /// Return a node with the specified name, or nullptr if not existing.
    /// @note The matching is case-insensitive.
    const osg::Node* getNode(const std::string& name) const;

    virtual bool useShieldAnimations() const { return false; }
    virtual void showWeapons(bool showWeapon) {}
    virtual bool getCarriedLeftShown() const { return false; }
    virtual void showCarriedLeft(bool show) {}
    virtual void setWeaponGroup(const std::string& group, bool relativeDuration) {}
    virtual void setVampire(bool vampire) {}
    /// A value < 1 makes the animation translucent, 1.f = fully opaque
    void setAlpha(float alpha);
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
    virtual void setAccurateAiming(bool enabled) {}
    virtual bool canBeHarvested() const { return false; }

private:
    Animation(const Animation&);
    void operator=(Animation&);
};

class ObjectAnimation : public Animation {
public:
    ObjectAnimation(const MWWorld::Ptr& ptr, const std::string &model, Resource::ResourceSystem* resourceSystem, bool animated, bool allowLight);

    bool canBeHarvested() const;
};

class UpdateVfxCallback : public osg::NodeCallback
{
public:
    UpdateVfxCallback(EffectParams& params)
        : mFinished(false)
        , mParams(params)
        , mStartingTime(0)
    {
    }

    bool mFinished;
    EffectParams mParams;

    virtual void operator()(osg::Node* node, osg::NodeVisitor* nv);

private:
    double mStartingTime;
};

}
#endif

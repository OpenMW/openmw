#ifndef GAME_RENDER_ANIMATION_H
#define GAME_RENDER_ANIMATION_H

#include "animationpriority.hpp"
#include "animblendcontroller.hpp"
#include "blendmask.hpp"
#include "bonegroup.hpp"

#include "../mwworld/movementdirection.hpp"
#include "../mwworld/ptr.hpp"

#include <components/misc/strings/algorithm.hpp>
#include <components/sceneutil/animblendrules.hpp>
#include <components/sceneutil/controller.hpp>
#include <components/sceneutil/nodecallback.hpp>
#include <components/sceneutil/textkeymap.hpp>
#include <components/sceneutil/util.hpp>
#include <components/vfs/pathutil.hpp>

#include <map>
#include <span>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace ESM
{
    struct Light;
    struct MagicEffect;
}

namespace Resource
{
    class ResourceSystem;
}

namespace SceneUtil
{
    class KeyframeHolder;
    class KeyframeController;
    class LightSource;
    class LightListCallback;
    class Skeleton;
    struct LightCommon;
}

namespace MWRender
{

    class ResetAccumRootCallback;
    class RotateController;
    class TransparencyUpdater;

    using ActiveControllersVector = std::vector<std::pair<osg::ref_ptr<osg::Node>, osg::ref_ptr<osg::Callback>>>;

    class EffectAnimationTime : public SceneUtil::ControllerSource
    {
    private:
        float mTime;

    public:
        float getValue(osg::NodeVisitor* nv) override;

        void addTime(float duration);
        void resetTime(float time);
        float getTime() const;

        EffectAnimationTime()
            : mTime(0)
        {
        }
    };

    /// @brief Detaches the node from its parent when the object goes out of scope.
    class PartHolder
    {
    public:
        PartHolder(osg::ref_ptr<osg::Node> node);

        ~PartHolder();

        const osg::ref_ptr<osg::Node>& getNode() const { return mNode; }

    private:
        osg::ref_ptr<osg::Node> mNode;

        void operator=(const PartHolder&);
        PartHolder(const PartHolder&);
    };
    using PartHolderPtr = std::unique_ptr<PartHolder>;

    struct EffectParams
    {
        std::string mModelName; // Just here so we don't add the same effect twice
        std::shared_ptr<EffectAnimationTime> mAnimTime;
        float mMaxControllerLength;
        std::string mEffectId;
        bool mLoop;
        std::string mBoneName;
    };

    class Animation : public osg::Referenced
    {
    public:
        using BlendMask = MWRender::BlendMask;
        using BoneGroup = MWRender::BoneGroup;
        using AnimPriority = MWRender::AnimPriority;

        class TextKeyListener
        {
        public:
            virtual void handleTextKey(
                std::string_view groupname, SceneUtil::TextKeyMap::ConstIterator key, const SceneUtil::TextKeyMap& map)
                = 0;

            virtual ~TextKeyListener() = default;
        };

        void setTextKeyListener(TextKeyListener* listener);

        virtual bool updateCarriedLeftVisible(const int weaptype) const { return false; }

        typedef std::unordered_map<std::string, osg::ref_ptr<osg::MatrixTransform>, Misc::StringUtils::CiHash,
            Misc::StringUtils::CiEqual>
            NodeMap;

    protected:
        class AnimationTime : public SceneUtil::ControllerSource
        {
        private:
            std::shared_ptr<float> mTimePtr;

        public:
            void setTimePtr(std::shared_ptr<float> time) { mTimePtr = std::move(time); }
            std::shared_ptr<float> getTimePtr() const { return mTimePtr; }

            float getValue(osg::NodeVisitor* nv) override;
        };

        class NullAnimationTime : public SceneUtil::ControllerSource
        {
        public:
            float getValue(osg::NodeVisitor* nv) override { return 0.f; }
        };

        struct AnimSource;

        struct AnimState
        {
            std::shared_ptr<AnimSource> mSource;
            float mStartTime = 0;
            float mLoopStartTime = 0;
            float mLoopStopTime = 0;
            float mStopTime = 0;

            std::shared_ptr<float> mTime = std::make_shared<float>(0);
            float mSpeedMult = 1;

            bool mPlaying = false;
            bool mLoopingEnabled = true;
            uint32_t mLoopCount = 0;

            AnimPriority mPriority{ 0 };
            int mBlendMask = 0;
            bool mAutoDisable = true;

            std::string mGroupname;
            std::string mStartKey;

            float getTime() const { return *mTime; }
            void setTime(float time) { *mTime = time; }
            bool blendMaskContains(size_t blendMask) const { return (mBlendMask & (1 << blendMask)); }
            bool shouldLoop() const { return getTime() >= mLoopStopTime && mLoopingEnabled && mLoopCount > 0; }
        };

        typedef std::map<std::string, AnimState, std::less<>> AnimStateMap;
        AnimStateMap mStates;

        typedef std::vector<std::shared_ptr<AnimSource>> AnimSourceList;
        AnimSourceList mAnimSources;

        std::unordered_set<std::string_view> mSupportedAnimations;

        osg::ref_ptr<osg::Group> mInsert;

        osg::ref_ptr<osg::Group> mObjectRoot;
        SceneUtil::Skeleton* mSkeleton;

        // The node expected to accumulate movement during movement animations.
        osg::ref_ptr<osg::Node> mAccumRoot;

        // The controller animating that node.
        osg::ref_ptr<SceneUtil::KeyframeController> mAccumCtrl;

        // Used to reset the position of the accumulation root every frame - the movement should be applied to the
        // physics system
        osg::ref_ptr<ResetAccumRootCallback> mResetAccumRootCallback;

        // Keep track of controllers that we added to our scene graph.
        // We may need to rebuild these controllers when the active animation groups / sources change.
        ActiveControllersVector mActiveControllers;

        // Keep track of the animation controllers for easy access
        std::map<osg::ref_ptr<osg::Node>, osg::ref_ptr<NifAnimBlendController>> mAnimBlendControllers;
        std::map<osg::ref_ptr<osg::Node>, osg::ref_ptr<BoneAnimBlendController>> mBoneAnimBlendControllers;

        std::shared_ptr<AnimationTime> mAnimationTimePtr[sNumBlendMasks];

        mutable NodeMap mNodeMap;
        mutable bool mNodeMapCreated;

        MWWorld::Ptr mPtr;

        Resource::ResourceSystem* mResourceSystem;

        osg::Vec3f mAccumulate;

        TextKeyListener* mTextKeyListener;

        osg::ref_ptr<RotateController> mHeadController;
        osg::ref_ptr<RotateController> mSpineController;
        osg::ref_ptr<RotateController> mRootController;
        float mHeadYawRadians;
        float mHeadPitchRadians;
        float mUpperBodyYawRadians;
        float mLegsYawRadians;
        float mBodyPitchRadians;

        osg::ref_ptr<RotateController> addRotateController(std::string_view bone);

        bool mHasMagicEffects;

        osg::ref_ptr<SceneUtil::LightSource> mGlowLight;
        osg::ref_ptr<SceneUtil::GlowUpdater> mGlowUpdater;
        osg::ref_ptr<TransparencyUpdater> mTransparencyUpdater;
        osg::ref_ptr<SceneUtil::LightSource> mExtraLightSource;

        float mAlpha;

        mutable std::map<std::string, float, std::less<>> mAnimVelocities;

        osg::ref_ptr<SceneUtil::LightListCallback> mLightListCallback;

        bool mPlayScriptedOnly;
        bool mRequiresBoneMap;

        const NodeMap& getNodeMap() const;

        /* Sets the appropriate animations on the bone groups based on priority by finding
         * the highest priority AnimationStates and linking the appropriate controllers stored
         * in the AnimationState to the corresponding nodes.
         */
        void resetActiveGroups();

        size_t detectBlendMask(const osg::Node* node, const std::string& controllerName) const;

        /* Updates the position of the accum root node for the given time, and
         * returns the wanted movement vector from the previous time. */
        void updatePosition(float oldtime, float newtime, osg::Vec3f& position);

        /* Resets the animation to the time of the specified start marker, without
         * moving anything, and set the end time to the specified stop marker. If
         * the marker is not found, or if the markers are the same, it returns
         * false.
         */
        bool reset(AnimState& state, const SceneUtil::TextKeyMap& keys, std::string_view groupname,
            std::string_view start, std::string_view stop, float startpoint, bool loopfallback);

        void handleTextKey(AnimState& state, std::string_view groupname, SceneUtil::TextKeyMap::ConstIterator key,
            const SceneUtil::TextKeyMap& map);

        /** Sets the root model of the object.
         *
         * Note that you must make sure all animation sources are cleared before resetting the object
         * root. All nodes previously retrieved with getNode will also become invalidated.
         * @param forceskeleton Wrap the object root in a Skeleton, even if it contains no skinned parts. Use this if
         * you intend to add skinned parts manually.
         * @param baseonly If true, then any meshes or particle systems in the model are ignored
         *      (useful for NPCs, where only the skeleton is needed for the root, and the actual NPC parts are then
         * assembled from separate files).
         */
        void setObjectRoot(const std::string& model, bool forceskeleton, bool baseonly, bool isCreature);

        void loadAdditionalAnimations(VFS::Path::NormalizedView model, const std::string& baseModel);

        /** Adds the keyframe controllers in the specified model as a new animation source.
         * @note Later added animation sources have the highest priority when it comes to finding a particular
         * animation.
         * @param model The file to add the keyframes for. Note that the .nif file extension will be replaced with .kf.
         * @param baseModel The filename of the mObjectRoot, only used for error messages.
         */
        void addAnimSource(std::string_view model, const std::string& baseModel);
        std::shared_ptr<AnimSource> addSingleAnimSource(const std::string& model, const std::string& baseModel);

        /** Adds an additional light to the given node using the specified ESM record. */
        void addExtraLight(osg::ref_ptr<osg::Group> parent, const SceneUtil::LightCommon& light);

        void clearAnimSources();

        /**
         * Provided to allow derived classes adding their own controllers. Note, the controllers must be added to
         * mActiveControllers so they get cleaned up properly on the next controller rebuild. A controller rebuild may
         * be necessary to ensure correct ordering.
         */
        virtual void addControllers();

        void removeFromSceneImpl();

        template <typename ControllerType>
        inline osg::Callback* handleBlendTransform(const osg::ref_ptr<osg::Node>& node,
            osg::ref_ptr<SceneUtil::KeyframeController> keyframeController,
            std::map<osg::ref_ptr<osg::Node>, osg::ref_ptr<ControllerType>>& blendControllers,
            const AnimBlendStateData& stateData, const osg::ref_ptr<const SceneUtil::AnimBlendRules>& blendRules,
            const AnimState& active);

    public:
        Animation(
            const MWWorld::Ptr& ptr, osg::ref_ptr<osg::Group> parentNode, Resource::ResourceSystem* resourceSystem);

        /// Must be thread safe
        virtual ~Animation();

        MWWorld::ConstPtr getPtr() const { return mPtr; }

        MWWorld::Ptr getPtr() { return mPtr; }

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
         * @param useAmbientLight attach white ambient light to the root VFX node of the scenegraph (Morrowind
         * default)
         * @note Will not add an effect twice.
         */
        void addEffect(std::string_view model, std::string_view effectId, bool loop = false,
            std::string_view bonename = {}, std::string_view texture = {}, bool useAmbientLight = true);

        void removeEffect(std::string_view effectId);
        void removeEffects();
        std::vector<std::string_view> getLoopingEffects() const;

        // Add a spell casting glow to an object. From measuring video taken from the original engine,
        // the glow seems to be about 1.5 seconds except for telekinesis, which is 1 second.
        void addSpellCastGlow(const osg::Vec4f& color, float glowDuration = 1.5);

        virtual void updatePtr(const MWWorld::Ptr& ptr);

        bool hasAnimation(std::string_view anim) const;

        bool isLoopingAnimation(std::string_view group) const;

        // Specifies the axis' to accumulate on. Non-accumulated axis will just
        // move visually, but not affect the actual movement. Each x/y/z value
        // should be on the scale of 0 to 1.
        void setAccumulation(const osg::Vec3f& accum);

        /** Plays an animation.
         * Creates or updates AnimationStates to represent and manage animation playback.
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
        void play(std::string_view groupname, const AnimPriority& priority, int blendMask, bool autodisable,
            float speedmult, std::string_view start, std::string_view stop, float startpoint, uint32_t loops,
            bool loopfallback = false);

        /** Adjust the speed multiplier of an already playing animation.
         */
        void adjustSpeedMult(const std::string& groupname, float speedmult);

        /** Returns true if the named animation group is playing. */
        bool isPlaying(std::string_view groupname) const;

        /// Returns true if no important animations are currently playing on the upper body.
        bool upperBodyReady() const;

        /** Gets info about the given animation group.
         * \param groupname Animation group to check.
         * \param complete Stores completion amount (0 = at start key, 0.5 = half way between start and stop keys), etc.
         * \param speedmult Stores the animation speed multiplier
         * \return True if the animation is active, false otherwise.
         */
        bool getInfo(std::string_view groupname, float* complete = nullptr, float* speedmult = nullptr,
            size_t* loopcount = nullptr) const;

        /// Returns the group name of the animation currently active on that bone group.
        std::string_view getActiveGroup(BoneGroup boneGroup) const;

        /// Get the absolute position in the animation track of the first text key with the given group.
        float getStartTime(const std::string& groupname) const;

        /// Get the absolute position in the animation track of the text key
        float getTextKeyTime(std::string_view textKey) const;

        /// Get the current absolute position in the animation track for the animation that is currently playing from
        /// the given group.
        float getCurrentTime(std::string_view groupname) const;

        /** Disables the specified animation group;
         * \param groupname Animation group to disable.
         */
        void disable(std::string_view groupname);

        /** Retrieves the velocity (in units per second) that the animation will move. */
        float getVelocity(std::string_view groupname) const;

        virtual osg::Vec3f runAnimation(float duration);

        void setLoopingEnabled(std::string_view groupname, bool enabled);

        /// This is typically called as part of runAnimation, but may be called manually if needed.
        void updateEffects();

        /// Return a node with the specified name, or nullptr if not existing.
        /// @note The matching is case-insensitive.
        const osg::Node* getNode(std::string_view name) const;

        MWWorld::MovementDirectionFlags getSupportedMovementDirections(
            std::span<const std::string_view> prefixes) const;

        bool getPlayScriptedOnly() const { return mPlayScriptedOnly; }
        void setPlayScriptedOnly(bool playScriptedOnly) { mPlayScriptedOnly = playScriptedOnly; }

        virtual bool useShieldAnimations() const { return false; }
        virtual bool getWeaponsShown() const { return false; }
        virtual void showWeapons(bool showWeapon) {}
        virtual bool getCarriedLeftShown() const { return false; }
        virtual void showCarriedLeft(bool show) {}
        virtual void setWeaponGroup(const std::string& group, bool relativeDuration) {}
        virtual void setVampire(bool vampire) {}
        /// A value < 1 makes the animation translucent, 1.f = fully opaque
        void setAlpha(float alpha);
        virtual void setPitchFactor(float factor) {}
        virtual void attachArrow() {}
        virtual void detachArrow() {}
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

        virtual void setUpperBodyYawRadians(float v) { mUpperBodyYawRadians = v; }
        virtual void setLegsYawRadians(float v) { mLegsYawRadians = v; }
        virtual float getUpperBodyYawRadians() const { return mUpperBodyYawRadians; }
        virtual float getLegsYawRadians() const { return mLegsYawRadians; }
        virtual void setBodyPitchRadians(float v) { mBodyPitchRadians = v; }
        virtual float getBodyPitchRadians() const { return mBodyPitchRadians; }

        virtual void setAccurateAiming(bool enabled) {}
        virtual bool canBeHarvested() const { return false; }
        virtual void harvest(const MWWorld::Ptr& ptr) {}

        virtual void removeFromScene();

    private:
        Animation(const Animation&);
        void operator=(Animation&);
    };

    class ObjectAnimation : public Animation
    {
    public:
        ObjectAnimation(const MWWorld::Ptr& ptr, const std::string& model, Resource::ResourceSystem* resourceSystem,
            bool animated, bool allowLight);

        bool canBeHarvested() const override;
        void harvest(const MWWorld::Ptr& ptr) override;
    };

    class UpdateVfxCallback : public SceneUtil::NodeCallback<UpdateVfxCallback>
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

        void operator()(osg::Node* node, osg::NodeVisitor* nv);

    private:
        double mStartingTime;
    };
}
#endif

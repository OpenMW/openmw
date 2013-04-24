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
        size_t mIndex;

    public:
        AnimationValue(Animation *anim, size_t layeridx)
          : mAnimation(anim), mIndex(layeridx)
        { }

        virtual Ogre::Real getValue() const;
        virtual void setValue(Ogre::Real value);
    };

    struct ObjectInfo {
        NifOgre::ObjectList mObjectList;
        /* Bit-field specifying which animation layers this object list is
         * explicitly animating on (1 = layer 0, 2 = layer 1, 4 = layer 2.
         * etc).
         */
        int mActiveLayers;
    };

    struct AnimLayer {
        std::string mGroupName;
        std::vector<Ogre::Controller<Ogre::Real> > *mControllers;
        const NifOgre::TextKeyMap *mTextKeys;
        NifOgre::TextKeyMap::const_iterator mStartKey;
        NifOgre::TextKeyMap::const_iterator mLoopStartKey;
        NifOgre::TextKeyMap::const_iterator mStopKey;
        NifOgre::TextKeyMap::const_iterator mNextKey;

        float mTime;

        bool mPlaying;
        size_t mLoopCount;

        AnimLayer();
    };

    MWWorld::Ptr mPtr;
    MWMechanics::CharacterController *mController;

    Ogre::SceneNode *mInsert;
    Ogre::Entity *mSkelBase;
    std::vector<ObjectInfo> mObjects;
    Ogre::Node *mAccumRoot;
    Ogre::Bone *mNonAccumRoot;
    Ogre::Vector3 mAccumulate;
    Ogre::Vector3 mLastPosition;

    NifOgre::NodeTargetValue<Ogre::Real> *mNonAccumCtrl;
    float mAnimVelocity;
    float mAnimSpeedMult;
    std::vector<Ogre::Controller<Ogre::Real> > mActiveCtrls;

    static const size_t sMaxLayers = 1;
    AnimLayer mLayer[sMaxLayers];
    Ogre::SharedPtr<Ogre::ControllerValue<Ogre::Real> > mAnimationValuePtr[sMaxLayers];

    static float calcAnimVelocity(const NifOgre::TextKeyMap &keys,
                                  NifOgre::NodeTargetValue<Ogre::Real> *nonaccumctrl,
                                  const std::string &groupname);

    /* Updates a skeleton instance so that all bones matching the source skeleton (based on
     * bone names) are positioned identically. */
    void updateSkeletonInstance(const Ogre::SkeletonInstance *skelsrc, Ogre::SkeletonInstance *skel);

    /* Updates the position of the accum root node for the current time, and
     * returns the wanted movement vector from the previous update. */
    void updatePosition(Ogre::Vector3 &position);

    static NifOgre::TextKeyMap::const_iterator findGroupStart(const NifOgre::TextKeyMap &keys, const std::string &groupname);

    /* Resets the animation to the time of the specified start marker, without
     * moving anything, and set the end time to the specified stop marker. If
     * the marker is not found, or if the markers are the same, it returns
     * false.
     */
    bool reset(size_t layeridx, const NifOgre::TextKeyMap &keys, NifOgre::NodeTargetValue<Ogre::Real> *nonaccumctrl, const std::string &groupname, const std::string &start, const std::string &stop);

    bool doLoop(size_t layeridx);

    bool handleTextKey(size_t layeridx, const NifOgre::TextKeyMap::const_iterator &key);

    void addObjectList(Ogre::SceneNode *node, const std::string &model, bool baseonly);
    static void destroyObjectList(Ogre::SceneManager *sceneMgr, NifOgre::ObjectList &objects);

    static void setRenderProperties(const NifOgre::ObjectList &objlist, Ogre::uint32 visflags, Ogre::uint8 solidqueue, Ogre::uint8 transqueue);

    void updateActiveControllers();

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

    void play(const std::string &groupname, const std::string &start, const std::string &stop, size_t loops);
    virtual Ogre::Vector3 runAnimation(float timepassed);

    bool isPlaying(size_t layeridx) const
    { return mLayer[layeridx].mPlaying; }

    Ogre::Node *getNode(const std::string &name);
};

}
#endif

#ifndef _GAME_RENDER_ANIMATION_H
#define _GAME_RENDER_ANIMATION_H

#include <components/nifogre/ogre_nif_loader.hpp>

#include "../mwworld/ptr.hpp"

namespace MWRender
{

class Animation
{
    struct GroupTimes {
        NifOgre::TextKeyMap::const_iterator mStart;
        NifOgre::TextKeyMap::const_iterator mStop;
        NifOgre::TextKeyMap::const_iterator mLoopStart;
        NifOgre::TextKeyMap::const_iterator mLoopStop;

        Ogre::AnimationState *mAnimState;
        size_t mLoops;

        GroupTimes()
          : mAnimState(NULL), mLoops(0)
        { }
    };

protected:
    MWWorld::Ptr mPtr;
    Ogre::SceneNode* mInsert;

    NifOgre::EntityList mEntityList;
    std::map<std::string,NifOgre::TextKeyMap> mTextKeys;
    Ogre::Bone *mAccumRoot;
    Ogre::Bone *mNonAccumRoot;
    Ogre::Vector3 mStartPosition;
    Ogre::Vector3 mLastPosition;

    float mTime;
    GroupTimes mCurGroup;
    GroupTimes mNextGroup;

    bool mSkipFrame;

    /* Updates the animation to the specified time, and moves the mPtr object
     * based on the change since the last update or reset. */
    void updatePosition(float time);
    /* Updates the animation to the specified time, without moving the mPtr
     * object. */
    void resetPosition(float time);

    bool findGroupTimes(const std::string &groupname, GroupTimes *times);

    void createEntityList(Ogre::SceneNode *node, const std::string &model);

public:
    Animation(const MWWorld::Ptr &ptr);
    virtual ~Animation();

    void playGroup(std::string groupname, int mode, int loops);
    void skipAnim();
    virtual void runAnimation(float timepassed);
};

}
#endif

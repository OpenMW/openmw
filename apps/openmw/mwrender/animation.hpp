#ifndef _GAME_RENDER_ANIMATION_H
#define _GAME_RENDER_ANIMATION_H

#include <components/nifogre/ogre_nif_loader.hpp>

#include "../mwworld/ptr.hpp"

namespace MWRender
{

class Animation
{
    struct GroupTimes {
        float mStart;
        float mStop;
        float mLoopStart;
        float mLoopStop;

        size_t mLoops;

        GroupTimes()
          : mStart(-1.0f), mStop(-1.0f), mLoopStart(-1.0f), mLoopStop(-1.0f),
            mLoops(0)
        { }
    };

protected:
    MWWorld::Ptr mPtr;
    Ogre::SceneNode* mInsert;

    float mTime;
    GroupTimes mCurGroup;
    GroupTimes mNextGroup;
    Ogre::AnimationState *mAnimState;

    bool mSkipFrame;

    NifOgre::EntityList mEntityList;
    NifOgre::TextKeyMap mTextKeys;
    Ogre::Node *mNonAccumRoot;

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

#ifndef _GAME_RENDER_ANIMATION_H
#define _GAME_RENDER_ANIMATION_H

#include <vector>

#include <components/nifogre/ogre_nif_loader.hpp>
#include <openengine/ogre/renderer.hpp>
#include "../mwworld/actiontalk.hpp"
#include <components/nif/node.hpp>
#include <openengine/bullet/physic.hpp>




namespace MWRender {

class Animation {
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
    Ogre::SceneNode* mInsert;
    OEngine::Render::OgreRenderer &mRend;

    float mTime;
    GroupTimes mCurGroup;
    GroupTimes mNextGroup;

    bool mSkipFrame;

    NifOgre::EntityList mEntityList;
    NifOgre::TextKeyMap mTextKeys;

    bool findGroupTimes(const std::string &groupname, GroupTimes *times);

public:
    Animation(OEngine::Render::OgreRenderer& _rend);
    virtual ~Animation();

    void playGroup(std::string groupname, int mode, int loops);
    void skipAnim();
    virtual void runAnimation(float timepassed);
};

}
#endif

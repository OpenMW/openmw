#include "animation.hpp"

#include <OgreHardwarePixelBuffer.h>
#include <OgreSkeletonInstance.h>
#include <OgreEntity.h>
#include <OgreBone.h>
#include <OgreSubMesh.h>
#include <OgreSceneManager.h>

namespace MWRender
{
std::map<std::string, int> Animation::sUniqueIDs;

Animation::Animation(OEngine::Render::OgreRenderer& _rend)
    : mInsert(NULL)
    , mRend(_rend)
    , mTime(0.0f)
    , mAnimate(0)
{
}

Animation::~Animation()
{
    Ogre::SceneManager *sceneMgr = mInsert->getCreator();
    for(size_t i = 0;i < mEntityList.mEntities.size();i++)
        sceneMgr->destroyEntity(mEntityList.mEntities[i]);
    mEntityList.mEntities.clear();
}

void Animation::startScript(std::string groupname, int mode, int loops)
{
    if(groupname == "all")
    {
        mAnimate = loops;
        mTime = 0.0f;
    }
}


void Animation::stopScript()
{
    mAnimate = 0;
}

}

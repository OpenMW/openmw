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

void Animation::playGroup(std::string groupname, int mode, int loops)
{
    if(groupname == "all")
    {
        mAnimate = loops;
        mTime = 0.0f;
    }
}


void Animation::skipAnim()
{
    mAnimate = 0;
}

void Animation::runAnimation(float timepassed)
{
    if(mAnimate != 0)
    {
        mTime += timepassed;

        if(mEntityList.mSkelBase)
        {
            Ogre::AnimationStateSet *aset = mEntityList.mSkelBase->getAllAnimationStates();
            Ogre::AnimationStateIterator as = aset->getAnimationStateIterator();
            while(as.hasMoreElements())
            {
                Ogre::AnimationState *state = as.getNext();
                state->setTimePosition(mTime);
                if(mTime >= state->getLength())
                {
                    if(mAnimate != -1)
                        mAnimate--;
                    //std::cout << "Stopping the animation\n";
                    if(mAnimate == 0)
                        mTime = state->getLength();
                    else
                        mTime = mTime - state->getLength();
                }
            }
        }
    }
}

}

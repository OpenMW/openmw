#include "animation.hpp"

#include <OgreSkeletonManager.h>
#include <OgreSkeletonInstance.h>
#include <OgreEntity.h>
#include <OgreBone.h>
#include <OgreSubMesh.h>
#include <OgreSceneManager.h>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"

#include "../mwmechanics/character.hpp"

namespace MWRender
{

Animation::Animation(const MWWorld::Ptr &ptr)
    : mPtr(ptr)
    , mController(NULL)
    , mInsert(NULL)
    , mAccumRoot(NULL)
    , mNonAccumRoot(NULL)
    , mAccumulate(Ogre::Vector3::ZERO)
    , mLastPosition(0.0f)
    , mCurrentKeys(NULL)
    , mCurrentAnim(NULL)
    , mCurrentTime(0.0f)
    , mPlaying(false)
    , mLooping(false)
    , mAnimVelocity(0.0f)
    , mAnimSpeedMult(1.0f)
{
}

Animation::~Animation()
{
    if(mInsert)
    {
        Ogre::SceneManager *sceneMgr = mInsert->getCreator();
        for(size_t i = 0;i < mEntityList.mEntities.size();i++)
            sceneMgr->destroyEntity(mEntityList.mEntities[i]);
    }
    mEntityList.mEntities.clear();
    mEntityList.mSkelBase = NULL;
}


Ogre::Bone *Animation::insertSkeletonSource(const std::string &name)
{
    Ogre::SkeletonManager &skelMgr = Ogre::SkeletonManager::getSingleton();
    Ogre::SkeletonPtr skel = skelMgr.getByName(name);
    if(skel.isNull())
    {
        NifOgre::Loader::createSkeleton(name);
        skel = skelMgr.getByName(name);
        if(skel.isNull())
        {
            std::cerr<< "Failed to get skeleton source "<<name <<std::endl;
            return NULL;
        }
    }
    skel->touch();
    mSkeletonSources.push_back(skel);

    Ogre::Skeleton::BoneIterator boneiter = skel->getBoneIterator();
    while(boneiter.hasMoreElements())
    {
        Ogre::Bone *bone = boneiter.getNext();
        Ogre::UserObjectBindings &bindings = bone->getUserObjectBindings();
        const Ogre::Any &data = bindings.getUserAny(NifOgre::sTextKeyExtraDataID);
        if(data.isEmpty() || !Ogre::any_cast<bool>(data))
            continue;

        for(int i = 0;i < skel->getNumAnimations();i++)
        {
            Ogre::Animation *anim = skel->getAnimation(i);
            const Ogre::Any &groupdata = bindings.getUserAny(std::string(NifOgre::sTextKeyExtraDataID)+
                                                             "@"+anim->getName());
            if(!groupdata.isEmpty())
                mTextKeys[anim->getName()] = Ogre::any_cast<NifOgre::TextKeyMap>(groupdata);
        }

        return bone;
    }

    return NULL;
}

void Animation::createEntityList(Ogre::SceneNode *node, const std::string &model)
{
    mInsert = node->createChildSceneNode();
    assert(mInsert);

    mEntityList = NifOgre::Loader::createEntities(mInsert, model);
    if(mEntityList.mSkelBase)
    {
        Ogre::AnimationStateSet *aset = mEntityList.mSkelBase->getAllAnimationStates();
        Ogre::AnimationStateIterator asiter = aset->getAnimationStateIterator();
        while(asiter.hasMoreElements())
        {
            Ogre::AnimationState *state = asiter.getNext();
            state->setEnabled(false);
            state->setLoop(false);
        }

        // Set the bones as manually controlled since we're applying the
        // transformations manually (needed if we want to apply an animation
        // from one skeleton onto another).
        Ogre::SkeletonInstance *skelinst = mEntityList.mSkelBase->getSkeleton();
        Ogre::Skeleton::BoneIterator boneiter = skelinst->getBoneIterator();
        while(boneiter.hasMoreElements())
            boneiter.getNext()->setManuallyControlled(true);

        Ogre::Bone *bone = insertSkeletonSource(skelinst->getName());
        if(!bone)
        {
            for(std::vector<Ogre::SkeletonPtr>::const_iterator iter(mSkeletonSources.begin());
                !bone && iter != mSkeletonSources.end();iter++)
            {
                Ogre::Skeleton::BoneIterator boneiter = (*iter)->getBoneIterator();
                while(boneiter.hasMoreElements())
                {
                    bone = boneiter.getNext();
                    Ogre::UserObjectBindings &bindings = bone->getUserObjectBindings();
                    const Ogre::Any &data = bindings.getUserAny(NifOgre::sTextKeyExtraDataID);
                    if(!data.isEmpty() && Ogre::any_cast<bool>(data))
                        break;

                    bone = NULL;
                }
            }
        }
        if(bone)
        {
            mAccumRoot = mInsert;
            mNonAccumRoot = skelinst->getBone(bone->getName());
        }
    }
}


bool Animation::hasAnimation(const std::string &anim)
{
    for(std::vector<Ogre::SkeletonPtr>::const_iterator iter(mSkeletonSources.begin());iter != mSkeletonSources.end();iter++)
    {
        if((*iter)->hasAnimation(anim))
            return true;
    }
    return false;
}


void Animation::setController(MWMechanics::CharacterController *controller)
{
    mController = controller;
}


void Animation::setAccumulation(const Ogre::Vector3 &accum)
{
    mAccumulate = accum;
}

void Animation::setSpeed(float speed)
{
    mAnimSpeedMult = 1.0f;
    if(mAnimVelocity > 1.0f && speed > 0.0f)
        mAnimSpeedMult = speed / mAnimVelocity;
}


void Animation::applyAnimation(const Ogre::Animation *anim, float time, Ogre::SkeletonInstance *skel)
{
    Ogre::TimeIndex timeindex = anim->_getTimeIndex(time);
    Ogre::Animation::NodeTrackIterator tracks = anim->getNodeTrackIterator();
    while(tracks.hasMoreElements())
    {
        Ogre::NodeAnimationTrack *track = tracks.getNext();
        const Ogre::String &targetname = track->getAssociatedNode()->getName();
        if(!skel->hasBone(targetname))
            continue;
        Ogre::Bone *bone = skel->getBone(targetname);
        bone->setOrientation(Ogre::Quaternion::IDENTITY);
        bone->setPosition(Ogre::Vector3::ZERO);
        bone->setScale(Ogre::Vector3::UNIT_SCALE);
        track->applyToNode(bone, timeindex);
    }

    // HACK: Dirty the animation state set so that Ogre will apply the
    // transformations to entities this skeleton instance is shared with.
    mEntityList.mSkelBase->getAllAnimationStates()->_notifyDirty();
}

void Animation::updateSkeletonInstance(const Ogre::SkeletonInstance *skelsrc, Ogre::SkeletonInstance *skel)
{
    Ogre::Skeleton::BoneIterator boneiter = skel->getBoneIterator();
    while(boneiter.hasMoreElements())
    {
        Ogre::Bone *bone = boneiter.getNext();
        if(!skelsrc->hasBone(bone->getName()))
            continue;
        Ogre::Bone *srcbone = skelsrc->getBone(bone->getName());
        bone->setOrientation(srcbone->getOrientation());
        bone->setPosition(srcbone->getPosition());
        bone->setScale(srcbone->getScale());
    }
}


Ogre::Vector3 Animation::updatePosition(float time)
{
    if(mLooping)
        mCurrentTime = std::fmod(std::max(time, 0.0f), mCurrentAnim->getLength());
    else
        mCurrentTime = std::min(mCurrentAnim->getLength(), std::max(time, 0.0f));
    applyAnimation(mCurrentAnim, mCurrentTime, mEntityList.mSkelBase->getSkeleton());

    Ogre::Vector3 posdiff = Ogre::Vector3::ZERO;
    if(mNonAccumRoot)
    {
        /* Get the non-accumulation root's difference from the last update. */
        posdiff = (mNonAccumRoot->getPosition() - mLastPosition) * mAccumulate;

        /* Translate the accumulation root back to compensate for the move. */
        mAccumRoot->translate(-posdiff);
        mLastPosition += posdiff;
    }
    return posdiff;
}

void Animation::reset(const std::string &marker)
{
    mNextKey = mCurrentKeys->begin();
    while(mNextKey != mCurrentKeys->end() && mNextKey->second != marker)
        mNextKey++;

    if(mNextKey != mCurrentKeys->end())
        mCurrentTime = mNextKey->first;
    else
    {
        mNextKey = mCurrentKeys->begin();
        mCurrentTime = 0.0f;
    }
    applyAnimation(mCurrentAnim, mCurrentTime, mEntityList.mSkelBase->getSkeleton());

    if(mNonAccumRoot)
    {
        mLastPosition = mNonAccumRoot->getPosition() * mAccumulate;
        mAccumRoot->setPosition(-mLastPosition);
    }
}


void Animation::play(const std::string &groupname, const std::string &start, bool loop)
{
    try {
        bool found = false;
        /* Look in reverse; last-inserted source has priority. */
        for(std::vector<Ogre::SkeletonPtr>::const_reverse_iterator iter(mSkeletonSources.rbegin());iter != mSkeletonSources.rend();iter++)
        {
            if((*iter)->hasAnimation(groupname))
            {
                mCurrentAnim = (*iter)->getAnimation(groupname);
                mCurrentKeys = &mTextKeys[groupname];
                mAnimVelocity = 0.0f;

                if(mNonAccumRoot)
                {
                    const Ogre::NodeAnimationTrack *track = 0;

                    Ogre::Animation::NodeTrackIterator trackiter = mCurrentAnim->getNodeTrackIterator();
                    while(!track && trackiter.hasMoreElements())
                    {
                        const Ogre::NodeAnimationTrack *cur = trackiter.getNext();
                        if(cur->getAssociatedNode()->getName() == mNonAccumRoot->getName())
                            track = cur;
                    }

                    if(track && track->getNumKeyFrames() > 1)
                    {
                        const Ogre::TransformKeyFrame *startkf, *endkf;
                        startkf = static_cast<const Ogre::TransformKeyFrame*>(track->getKeyFrame(0));
                        endkf = static_cast<const Ogre::TransformKeyFrame*>(track->getKeyFrame(track->getNumKeyFrames() - 1));

                        mAnimVelocity = startkf->getTranslate().distance(endkf->getTranslate()) /
                                        mCurrentAnim->getLength();
                    }
                }

                found = true;
                break;
            }
        }
        if(!found)
            throw std::runtime_error("Failed to find animation "+groupname);

        reset(start);
        mPlaying = true;
        mLooping = loop;
    }
    catch(std::exception &e) {
        std::cerr<< e.what() <<std::endl;
    }
}

Ogre::Vector3 Animation::runAnimation(float timepassed)
{
    Ogre::Vector3 movement = Ogre::Vector3::ZERO;

    timepassed *= mAnimSpeedMult;
    while(mCurrentAnim && mPlaying)
    {
        float targetTime = mCurrentTime + timepassed;
        if(mNextKey == mCurrentKeys->end() || mNextKey->first > targetTime)
        {
            movement += updatePosition(targetTime);
            mPlaying = (mLooping || mCurrentAnim->getLength() >= targetTime);
            break;
        }

        float time = mNextKey->first;
        const std::string &evt = mNextKey->second;
        mNextKey++;

        movement += updatePosition(time);
        timepassed = targetTime - time;

        if(evt == "start" || evt == "loop start")
        {
            /* Do nothing */
            continue;
        }
        if(evt == "loop stop")
        {
            if(mLooping)
            {
                reset("loop start");
                if(mCurrentTime >= time)
                    break;
            }
            continue;
        }
        if(evt == "stop")
        {
            if(mLooping)
            {
                reset("loop start");
                if(mCurrentTime >= time)
                    break;
            }
            else
            {
                mPlaying = false;
                if(mController)
                    mController->markerEvent(time, evt);
            }
            continue;
        }
        if(mController)
            mController->markerEvent(time, evt);
    }

    return movement;
}

}

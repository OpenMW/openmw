#include "animation.hpp"

#include <OgreSkeletonManager.h>
#include <OgreSkeletonInstance.h>
#include <OgreEntity.h>
#include <OgreBone.h>
#include <OgreSubMesh.h>
#include <OgreSceneManager.h>

#include "../mwbase/environment.hpp"
#include "../mwbase/soundmanager.hpp"
#include "../mwbase/world.hpp"

#include "../mwmechanics/character.hpp"

namespace MWRender
{

void Animation::destroyObjectList(Ogre::SceneManager *sceneMgr, NifOgre::ObjectList &objects)
{
    for(size_t i = 0;i < objects.mParticles.size();i++)
        sceneMgr->destroyParticleSystem(objects.mParticles[i]);
    for(size_t i = 0;i < objects.mEntities.size();i++)
        sceneMgr->destroyEntity(objects.mEntities[i]);
    objects.mControllers.clear();
    objects.mCameras.clear();
    objects.mParticles.clear();
    objects.mEntities.clear();
    objects.mSkelBase = NULL;
}

Animation::Animation(const MWWorld::Ptr &ptr)
    : mPtr(ptr)
    , mController(NULL)
    , mInsert(NULL)
    , mAccumRoot(NULL)
    , mNonAccumRoot(NULL)
    , mAccumulate(Ogre::Vector3::ZERO)
    , mLastPosition(0.0f)
    , mCurrentControllers(NULL)
    , mCurrentKeys(NULL)
    , mCurrentAnim(NULL)
    , mCurrentTime(0.0f)
    , mStopTime(0.0f)
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
        destroyObjectList(sceneMgr, mObjectList);

        for(size_t i = 0;i < mAnimationSources.size();i++)
            destroyObjectList(sceneMgr, mAnimationSources[i]);
        mAnimationSources.clear();
    }
}


void Animation::setAnimationSources(const std::vector<std::string> &names)
{
    if(!mObjectList.mSkelBase)
        return;
    Ogre::SceneManager *sceneMgr = mInsert->getCreator();

    mCurrentControllers = &mObjectList.mControllers;
    mCurrentAnim = NULL;
    mCurrentKeys = NULL;
    mAnimVelocity = 0.0f;
    mAccumRoot = NULL;
    mNonAccumRoot = NULL;
    mTextKeys.clear();
    for(size_t i = 0;i < mAnimationSources.size();i++)
        destroyObjectList(sceneMgr, mAnimationSources[i]);
    mAnimationSources.clear();

    Ogre::SharedPtr<Ogre::ControllerValue<Ogre::Real> > ctrlval(OGRE_NEW AnimationValue(this));
    Ogre::SkeletonInstance *skelinst = mObjectList.mSkelBase->getSkeleton();
    std::vector<std::string>::const_iterator nameiter;
    for(nameiter = names.begin();nameiter != names.end();nameiter++)
    {
        mAnimationSources.push_back(NifOgre::Loader::createObjectBase(sceneMgr, *nameiter));
        if(!mAnimationSources.back().mSkelBase)
        {
            std::cerr<< "Failed to get skeleton source "<<*nameiter <<std::endl;
            destroyObjectList(sceneMgr, mAnimationSources.back());
            mAnimationSources.pop_back();
            continue;
        }
        NifOgre::ObjectList &objects = mAnimationSources.back();

        for(size_t i = 0;i < objects.mControllers.size();i++)
        {
            NifOgre::NodeTargetValue<Ogre::Real> *dstval = dynamic_cast<NifOgre::NodeTargetValue<Ogre::Real>*>(objects.mControllers[i].getDestination().getPointer());
            if(!dstval) continue;

            const Ogre::String &trgtname = dstval->getNode()->getName();
            if(!skelinst->hasBone(trgtname)) continue;

            Ogre::Bone *bone = skelinst->getBone(trgtname);
            dstval->setNode(bone);
        }

        for(size_t i = 0;i < objects.mControllers.size();i++)
        {
            if(objects.mControllers[i].getSource().isNull())
                objects.mControllers[i].setSource(ctrlval);
        }

        Ogre::Entity *ent = objects.mSkelBase;
        Ogre::SkeletonPtr skel = Ogre::SkeletonManager::getSingleton().getByName(ent->getSkeleton()->getName());
        Ogre::Skeleton::BoneIterator boneiter = skel->getBoneIterator();
        while(boneiter.hasMoreElements())
        {
            Ogre::Bone *bone = boneiter.getNext();
            Ogre::UserObjectBindings &bindings = bone->getUserObjectBindings();
            const Ogre::Any &data = bindings.getUserAny(NifOgre::sTextKeyExtraDataID);
            if(data.isEmpty() || !Ogre::any_cast<bool>(data))
                continue;

            if(!mNonAccumRoot)
            {
                mAccumRoot = mInsert;
                mNonAccumRoot = mObjectList.mSkelBase->getSkeleton()->getBone(bone->getName());
            }

            for(int i = 0;i < skel->getNumAnimations();i++)
            {
                Ogre::Animation *anim = skel->getAnimation(i);
                const Ogre::Any &groupdata = bindings.getUserAny(std::string(NifOgre::sTextKeyExtraDataID)+
                                                                 "@"+anim->getName());
                if(!groupdata.isEmpty())
                    mTextKeys[anim->getName()] = Ogre::any_cast<NifOgre::TextKeyMap>(groupdata);
            }

            break;
        }
    }
}

void Animation::createObjectList(Ogre::SceneNode *node, const std::string &model)
{
    mInsert = node->createChildSceneNode();
    assert(mInsert);

    mObjectList = NifOgre::Loader::createObjects(mInsert, model);
    if(mObjectList.mSkelBase)
    {
        Ogre::AnimationStateSet *aset = mObjectList.mSkelBase->getAllAnimationStates();
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
        Ogre::SkeletonInstance *skelinst = mObjectList.mSkelBase->getSkeleton();
        Ogre::Skeleton::BoneIterator boneiter = skelinst->getBoneIterator();
        while(boneiter.hasMoreElements())
            boneiter.getNext()->setManuallyControlled(true);
    }

    Ogre::SharedPtr<Ogre::ControllerValue<Ogre::Real> > ctrlval(OGRE_NEW AnimationValue(this));
    for(size_t i = 0;i < mObjectList.mControllers.size();i++)
    {
        if(mObjectList.mControllers[i].getSource().isNull())
            mObjectList.mControllers[i].setSource(ctrlval);
    }
    mCurrentControllers = &mObjectList.mControllers;
}


Ogre::Node *Animation::getNode(const std::string &name)
{
    if(mObjectList.mSkelBase)
    {
        Ogre::SkeletonInstance *skel = mObjectList.mSkelBase->getSkeleton();
        if(skel->hasBone(name))
            return skel->getBone(name);
    }
    return NULL;
}


bool Animation::hasAnimation(const std::string &anim)
{
    for(std::vector<NifOgre::ObjectList>::const_iterator iter(mAnimationSources.begin());iter != mAnimationSources.end();iter++)
    {
        if(iter->mSkelBase->hasAnimationState(anim))
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

void Animation::setLooping(bool loop)
{
    mLooping = loop;
}

void Animation::updatePtr(const MWWorld::Ptr &ptr)
{
    mPtr = ptr;
}


void Animation::calcAnimVelocity()
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
        float loopstarttime = 0.0f;
        float loopstoptime = mCurrentAnim->getLength();
        NifOgre::TextKeyMap::const_iterator keyiter = mCurrentKeys->begin();
        while(keyiter != mCurrentKeys->end())
        {
            if(keyiter->second == "loop start")
                loopstarttime = keyiter->first;
            else if(keyiter->second == "loop stop")
            {
                loopstoptime = keyiter->first;
                break;
            }
            keyiter++;
        }

        if(loopstoptime > loopstarttime)
        {
            Ogre::TransformKeyFrame startkf(0, loopstarttime);
            Ogre::TransformKeyFrame endkf(0, loopstoptime);

            track->getInterpolatedKeyFrame(mCurrentAnim->_getTimeIndex(loopstarttime), &startkf);
            track->getInterpolatedKeyFrame(mCurrentAnim->_getTimeIndex(loopstoptime), &endkf);

            mAnimVelocity = startkf.getTranslate().distance(endkf.getTranslate()) /
                            (loopstoptime-loopstarttime);
        }
    }
}

static void updateBoneTree(const Ogre::SkeletonInstance *skelsrc, Ogre::Bone *bone)
{
    if(skelsrc->hasBone(bone->getName()))
    {
        Ogre::Bone *srcbone = skelsrc->getBone(bone->getName());
        if(!srcbone->getParent() || !bone->getParent())
        {
            bone->setOrientation(srcbone->getOrientation());
            bone->setPosition(srcbone->getPosition());
            bone->setScale(srcbone->getScale());
        }
        else
        {
            bone->_setDerivedOrientation(srcbone->_getDerivedOrientation());
            bone->_setDerivedPosition(srcbone->_getDerivedPosition());
            bone->setScale(Ogre::Vector3::UNIT_SCALE);
        }
    }
    else
    {
        // No matching bone in the source. Make sure it stays properly offset
        // from its parent.
        bone->resetToInitialState();
    }

    Ogre::Node::ChildNodeIterator boneiter = bone->getChildIterator();
    while(boneiter.hasMoreElements())
        updateBoneTree(skelsrc, static_cast<Ogre::Bone*>(boneiter.getNext()));
}

void Animation::updateSkeletonInstance(const Ogre::SkeletonInstance *skelsrc, Ogre::SkeletonInstance *skel)
{
    Ogre::Skeleton::BoneIterator boneiter = skel->getRootBoneIterator();
    while(boneiter.hasMoreElements())
        updateBoneTree(skelsrc, boneiter.getNext());
}


Ogre::Vector3 Animation::updatePosition()
{
    Ogre::Vector3 posdiff;

    Ogre::TransformKeyFrame kf(0, mCurrentTime);
    Ogre::Animation::NodeTrackIterator trackiter = mCurrentAnim->getNodeTrackIterator();
    while(trackiter.hasMoreElements())
    {
        const Ogre::NodeAnimationTrack *track = trackiter.getNext();
        if(track->getAssociatedNode()->getName() == mNonAccumRoot->getName())
        {
            track->getInterpolatedKeyFrame(mCurrentAnim->_getTimeIndex(mCurrentTime), &kf);
            break;
        }
    }

    /* Get the non-accumulation root's difference from the last update. */
    posdiff = (kf.getTranslate() - mLastPosition) * mAccumulate;

    /* Translate the accumulation root back to compensate for the move. */
    mLastPosition += posdiff;
    mAccumRoot->setPosition(-mLastPosition);

    return posdiff;
}

void Animation::reset(const std::string &start, const std::string &stop)
{
    mNextKey = mCurrentKeys->begin();

    while(mNextKey != mCurrentKeys->end() && mNextKey->second != start)
        mNextKey++;
    if(mNextKey != mCurrentKeys->end())
        mCurrentTime = mNextKey->first;
    else
    {
        mNextKey = mCurrentKeys->begin();
        while(mNextKey != mCurrentKeys->end() && mNextKey->second != "start")
            mNextKey++;
        if(mNextKey != mCurrentKeys->end())
            mCurrentTime = mNextKey->first;
        else
        {
            mNextKey = mCurrentKeys->begin();
            mCurrentTime = 0.0f;
        }
    }

    if(stop.length() > 0)
    {
        NifOgre::TextKeyMap::const_iterator stopKey = mNextKey;
        while(stopKey != mCurrentKeys->end() && stopKey->second != stop)
            stopKey++;
        if(stopKey != mCurrentKeys->end())
            mStopTime = stopKey->first;
        else
            mStopTime = mCurrentAnim->getLength();
    }

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

        if(track)
        {
            Ogre::TransformKeyFrame kf(0, mCurrentTime);
            track->getInterpolatedKeyFrame(mCurrentAnim->_getTimeIndex(mCurrentTime), &kf);
            mLastPosition = kf.getTranslate() * mAccumulate;
        }
    }
}


bool Animation::handleEvent(float time, const std::string &evt)
{
    if(evt == "start" || evt == "loop start")
    {
        /* Do nothing */
        return true;
    }

    if(evt.compare(0, 7, "sound: ") == 0)
    {
        MWBase::SoundManager *sndMgr = MWBase::Environment::get().getSoundManager();
        sndMgr->playSound3D(mPtr, evt.substr(7), 1.0f, 1.0f);
        return true;
    }
    if(evt.compare(0, 10, "soundgen: ") == 0)
    {
        // FIXME: Lookup the SoundGen (SNDG) for the specified sound that corresponds
        // to this actor type
        return true;
    }

    if(evt == "loop stop")
    {
        if(mLooping)
        {
            reset("loop start", "");
            if(mCurrentTime >= time)
                return false;
        }
        return true;
    }
    if(evt == "stop")
    {
        if(mLooping)
        {
            reset("loop start", "");
            if(mCurrentTime >= time)
                return false;
            return true;
        }
        // fall-through
    }
    if(mController)
        mController->markerEvent(time, evt);
    return true;
}


void Animation::play(const std::string &groupname, const std::string &start, const std::string &stop, bool loop)
{
    try {
        bool found = false;
        /* Look in reverse; last-inserted source has priority. */
        for(std::vector<NifOgre::ObjectList>::reverse_iterator iter(mAnimationSources.rbegin());iter != mAnimationSources.rend();iter++)
        {
            if(iter->mSkelBase->hasAnimationState(groupname))
            {
                mCurrentAnim = iter->mSkelBase->getSkeleton()->getAnimation(groupname);
                mCurrentKeys = &mTextKeys[groupname];
                mCurrentControllers = &iter->mControllers;
                mAnimVelocity = 0.0f;

                if(mNonAccumRoot)
                    calcAnimVelocity();

                found = true;
                break;
            }
        }
        if(!found)
            throw std::runtime_error("Failed to find animation "+groupname);

        reset(start, stop);
        setLooping(loop);
        mPlaying = true;
    }
    catch(std::exception &e) {
        std::cerr<< e.what() <<std::endl;
    }
}

Ogre::Vector3 Animation::runAnimation(float timepassed)
{
    Ogre::Vector3 movement(0.0f);

    timepassed *= mAnimSpeedMult;
    while(mCurrentAnim && mPlaying)
    {
        float targetTime = mCurrentTime + timepassed;
        if(mNextKey == mCurrentKeys->end() || mNextKey->first > targetTime)
        {
            mCurrentTime = std::min(mStopTime, targetTime);
            if(mNonAccumRoot)
                movement += updatePosition();
            mPlaying = (mLooping || mStopTime > mCurrentTime);
            timepassed = targetTime - mCurrentTime;
            break;
        }

        float time = mNextKey->first;
        const std::string &evt = mNextKey->second;
        mNextKey++;

        mCurrentTime = time;
        if(mNonAccumRoot)
            movement += updatePosition();
        mPlaying = (mLooping || mStopTime > mCurrentTime);
        timepassed = targetTime - mCurrentTime;

        if(!handleEvent(time, evt))
            break;
    }
    for(size_t i = 0;i < mCurrentControllers->size();i++)
        (*mCurrentControllers)[i].update();

    if(mObjectList.mSkelBase)
    {
        // HACK: Dirty the animation state set so that Ogre will apply the
        // transformations to entities this skeleton instance is shared with.
        mObjectList.mSkelBase->getAllAnimationStates()->_notifyDirty();
    }

    return movement;
}

}

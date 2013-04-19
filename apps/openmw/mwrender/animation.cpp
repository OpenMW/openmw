#include "animation.hpp"

#include <OgreSkeletonManager.h>
#include <OgreSkeletonInstance.h>
#include <OgreEntity.h>
#include <OgreSubEntity.h>
#include <OgreParticleSystem.h>
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
    , mSkelBase(NULL)
    , mAccumRoot(NULL)
    , mNonAccumRoot(NULL)
    , mAccumulate(0.0f)
    , mLastPosition(0.0f)
    , mCurrentAnim(NULL)
    , mCurrentControllers(NULL)
    , mCurrentKeys(NULL)
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
        for(size_t i = 0;i < mObjectLists.size();i++)
            destroyObjectList(sceneMgr, mObjectLists[i]);
        mObjectLists.clear();
    }
}


void Animation::addObjectList(Ogre::SceneNode *node, const std::string &model, bool baseonly)
{
    if(!mInsert)
    {
        mInsert = node->createChildSceneNode();
        assert(mInsert);
    }
    Ogre::SharedPtr<Ogre::ControllerValue<Ogre::Real> > ctrlval(OGRE_NEW AnimationValue(this));

    mObjectLists.push_back(!baseonly ? NifOgre::Loader::createObjects(mInsert, model) :
                                       NifOgre::Loader::createObjectBase(mInsert, model));
    NifOgre::ObjectList &objlist = mObjectLists.back();
    if(objlist.mSkelBase)
    {
        if(!mSkelBase)
            mSkelBase = objlist.mSkelBase;

        Ogre::AnimationStateSet *aset = objlist.mSkelBase->getAllAnimationStates();
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
        Ogre::SkeletonInstance *skelinst = objlist.mSkelBase->getSkeleton();
        Ogre::Skeleton::BoneIterator boneiter = skelinst->getBoneIterator();
        while(boneiter.hasMoreElements())
            boneiter.getNext()->setManuallyControlled(true);

        if(mSkelBase != objlist.mSkelBase)
        {
            Ogre::SkeletonInstance *baseinst = mSkelBase->getSkeleton();
            for(size_t i = 0;i < objlist.mControllers.size();i++)
            {
                NifOgre::NodeTargetValue<Ogre::Real> *dstval;
                dstval = dynamic_cast<NifOgre::NodeTargetValue<Ogre::Real>*>(objlist.mControllers[i].getDestination().getPointer());
                if(!dstval) continue;

                const Ogre::String &trgtname = dstval->getNode()->getName();
                if(!baseinst->hasBone(trgtname)) continue;

                Ogre::Bone *bone = baseinst->getBone(trgtname);
                dstval->setNode(bone);
            }
        }

        Ogre::SkeletonPtr skel = Ogre::SkeletonManager::getSingleton().getByName(skelinst->getName());
        boneiter = skel->getBoneIterator();
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
                mNonAccumRoot = mSkelBase->getSkeleton()->getBone(bone->getName());
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
    for(size_t i = 0;i < objlist.mControllers.size();i++)
    {
        if(objlist.mControllers[i].getSource().isNull())
            objlist.mControllers[i].setSource(ctrlval);
    }

    if(!mCurrentControllers || (*mCurrentControllers).size() == 0)
        mCurrentControllers = &objlist.mControllers;
}

void Animation::setRenderProperties(const NifOgre::ObjectList &objlist, Ogre::uint32 visflags, Ogre::uint8 solidqueue, Ogre::uint8 transqueue)
{
    for(size_t i = 0;i < objlist.mEntities.size();i++)
    {
        Ogre::Entity *ent = objlist.mEntities[i];
        if(visflags != 0)
            ent->setVisibilityFlags(visflags);

        for(unsigned int j = 0;j < ent->getNumSubEntities();++j)
        {
            Ogre::SubEntity* subEnt = ent->getSubEntity(j);
            subEnt->setRenderQueueGroup(subEnt->getMaterial()->isTransparent() ? transqueue : solidqueue);
        }
    }
    for(size_t i = 0;i < objlist.mParticles.size();i++)
    {
        Ogre::ParticleSystem *part = objlist.mParticles[i];
        if(visflags != 0)
            part->setVisibilityFlags(visflags);
        // TODO: Check particle material for actual transparency
        part->setRenderQueueGroup(transqueue);
    }
}


Ogre::Node *Animation::getNode(const std::string &name)
{
    if(mSkelBase)
    {
        Ogre::SkeletonInstance *skel = mSkelBase->getSkeleton();
        if(skel->hasBone(name))
            return skel->getBone(name);
    }
    return NULL;
}


bool Animation::hasAnimation(const std::string &anim)
{
    for(std::vector<NifOgre::ObjectList>::const_iterator iter(mObjectLists.begin());iter != mObjectLists.end();iter++)
    {
        if(iter->mSkelBase && iter->mSkelBase->hasAnimationState(anim))
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
    mStartKey = mCurrentKeys->begin();

    while(mStartKey != mCurrentKeys->end() && mStartKey->second != start)
        mStartKey++;
    if(mStartKey != mCurrentKeys->end())
        mCurrentTime = mStartKey->first;
    else
    {
        mStartKey = mCurrentKeys->begin();
        mCurrentTime = mStartKey->first;
    }
    mNextKey = mStartKey;

    if(stop.length() > 0)
    {
        mStopKey = mStartKey;
        while(mStopKey != mCurrentKeys->end() && mStopKey->second != stop)
            mStopKey++;
        if(mStopKey == mCurrentKeys->end())
            mStopKey--;
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
            reset("loop start");
            if(mCurrentTime >= time)
                return false;
        }
        return true;
    }
    if(evt == "stop")
    {
        if(mLooping)
        {
            reset("loop start");
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
        for(std::vector<NifOgre::ObjectList>::reverse_iterator iter(mObjectLists.rbegin());iter != mObjectLists.rend();iter++)
        {
            if(iter->mSkelBase && iter->mSkelBase->hasAnimationState(groupname))
            {
                Ogre::SkeletonInstance *skel = iter->mSkelBase->getSkeleton();
                mCurrentAnim = skel->getAnimation(groupname);
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
        if(mNextKey->first > targetTime)
        {
            mCurrentTime = targetTime;
            if(mNonAccumRoot)
                movement += updatePosition();
            break;
        }

        float time = mNextKey->first;
        const std::string &evt = mNextKey->second;
        mNextKey++;

        mCurrentTime = time;
        if(mNonAccumRoot)
            movement += updatePosition();

        mPlaying = (mLooping || mStopKey->first > mCurrentTime);
        timepassed = targetTime - mCurrentTime;

        if(!handleEvent(time, evt))
            break;
    }

    for(size_t i = 0;i < (*mCurrentControllers).size();i++)
        (*mCurrentControllers)[i].update();

    if(mSkelBase)
    {
        const Ogre::SkeletonInstance *baseinst = mSkelBase->getSkeleton();
        for(std::vector<NifOgre::ObjectList>::iterator iter(mObjectLists.begin());iter != mObjectLists.end();iter++)
        {
            Ogre::Entity *ent = iter->mSkelBase;
            if(!ent) continue;

            Ogre::SkeletonInstance *inst = ent->getSkeleton();
            if(baseinst != inst)
                updateSkeletonInstance(baseinst, inst);

            // HACK: Dirty the animation state set so that Ogre will apply the
            // transformations to entities this skeleton instance is shared with.
            ent->getAllAnimationStates()->_notifyDirty();
        }
    }

    return movement;
}

}

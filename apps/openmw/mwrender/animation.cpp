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

Animation::AnimLayer::AnimLayer()
  : mControllers(NULL)
  , mTextKeys(NULL)
  , mTime(0.0f)
  , mPlaying(false)
  , mLoopCount(0)
{
}


Ogre::Real Animation::AnimationValue::getValue() const
{
    size_t idx = mIndex;
    while(idx > 0 && mAnimation->mLayer[idx].mGroupName.empty())
        idx--;
    if(!mAnimation->mLayer[idx].mGroupName.empty())
        return mAnimation->mLayer[idx].mTime;
    return 0.0f;
}

void Animation::AnimationValue::setValue(Ogre::Real value)
{
    mAnimation->mLayer[mIndex].mTime = value;
}


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
    , mInsert(NULL)
    , mSkelBase(NULL)
    , mAccumRoot(NULL)
    , mNonAccumRoot(NULL)
    , mNonAccumCtrl(NULL)
    , mAccumulate(0.0f)
    , mLastPosition(0.0f)
    , mAnimVelocity(0.0f)
    , mAnimSpeedMult(1.0f)
{
    for(size_t i = 0;i < sMaxLayers;i++)
        mAnimationValuePtr[i].bind(OGRE_NEW AnimationValue(this, i));

    /* As long as we remain under 128 active controllers, we can avoid
     * reallocations. */
    mActiveCtrls.reserve(128);
}

Animation::~Animation()
{
    if(mInsert)
    {
        Ogre::SceneManager *sceneMgr = mInsert->getCreator();
        for(size_t i = 0;i < mObjects.size();i++)
            destroyObjectList(sceneMgr, mObjects[i].mObjectList);
        mObjects.clear();
    }
}


void Animation::addObjectList(Ogre::SceneNode *node, const std::string &model, bool baseonly)
{
    if(!mInsert)
    {
        mInsert = node->createChildSceneNode();
        assert(mInsert);
    }

    mObjects.push_back(ObjectInfo());
    ObjectInfo &obj = mObjects.back();
    obj.mActiveLayers = 0;
    obj.mObjectList = (!baseonly ? NifOgre::Loader::createObjects(mInsert, model) :
                                   NifOgre::Loader::createObjectBase(mInsert, model));

    NifOgre::ObjectList &objlist = obj.mObjectList;
    if(objlist.mSkelBase)
    {
        if(mObjects.size() == 1)
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
    }
    if(objlist.mSkelBase && mSkelBase)
    {
        Ogre::SkeletonInstance *baseinst = mSkelBase->getSkeleton();
        if(mSkelBase == objlist.mSkelBase)
        {
            if(objlist.mTextKeys.size() > 0)
            {
                mAccumRoot = mInsert;
                mNonAccumRoot = baseinst->getBone(objlist.mTextKeys.begin()->first);
            }
        }
        else
        {
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
    }
    for(size_t i = 0;i < objlist.mControllers.size();i++)
    {
        if(objlist.mControllers[i].getSource().isNull())
            objlist.mControllers[i].setSource(mAnimationValuePtr[0]);
    }

    mActiveCtrls.insert(mActiveCtrls.end(), objlist.mControllers.begin(), objlist.mControllers.end());
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


void Animation::clearExtraSources()
{
    for(size_t layer = 0;layer < sMaxLayers;layer++)
    {
        mLayer[layer].mGroupName.clear();
        mLayer[layer].mTextKeys = NULL;
        mLayer[layer].mControllers = NULL;
        mLayer[layer].mTime = 0.0f;
        mLayer[layer].mLoopCount = 0;
        mLayer[layer].mPlaying = false;
    }
    mNonAccumCtrl = NULL;
    mAnimVelocity = 0.0f;

    mLastPosition = Ogre::Vector3(0.0f);
    if(mAccumRoot)
        mAccumRoot->setPosition(mLastPosition);

    if(mObjects.size() > 1)
    {
        mObjects.resize(1);
        mObjects[0].mActiveLayers = 0;

        NifOgre::ObjectList &objlist = mObjects[0].mObjectList;
        mActiveCtrls.clear();
        mActiveCtrls.insert(mActiveCtrls.end(), objlist.mControllers.begin(), objlist.mControllers.end());
    }
}


void Animation::updateActiveControllers()
{
    mActiveCtrls.clear();

    /* First, get all controllers that don't target a node, or that target
     * nodes that don't belong to any particular layer.
     */
    std::vector<ObjectInfo>::iterator obj(mObjects.begin());
    for(;obj != mObjects.end();obj++)
    {
        std::vector<Ogre::Controller<Ogre::Real> >::const_iterator ctrl(obj->mObjectList.mControllers.begin());
        for(;ctrl != obj->mObjectList.mControllers.end();ctrl++)
        {
            NifOgre::NodeTargetValue<Ogre::Real> *dstval;
            dstval = dynamic_cast<NifOgre::NodeTargetValue<Ogre::Real>*>(ctrl->getDestination().getPointer());
            if(dstval)
            {
                /*if(getLayerByName(dstval->getNode()->getName()) >= 0)*/
                    continue;
            }
            mActiveCtrls.insert(mActiveCtrls.end(), *ctrl);
        }
    }

    std::vector<Ogre::Controller<Ogre::Real> > *ctrls = NULL;
    size_t layer = 0;
    while(layer < sMaxLayers)
    {
        /* Now get controllers that target nodes that belong to this layer from
         * whatever objectlist is active on this layer.
         */
        std::vector<ObjectInfo>::iterator obj(mObjects.begin());
        for(;obj != mObjects.end();obj++)
        {
            if((obj->mActiveLayers&(1<<layer)))
            {
                ctrls = &obj->mObjectList.mControllers;
                break;
            }
        }
        if(ctrls == NULL)
        {
            layer++;
            continue;
        }

        /* Check if any objectlists are active on subsequent layers. Include
         * those layers if not.
         */
        size_t nextlayer = layer+1;
        for(;nextlayer < sMaxLayers;nextlayer++)
        {
            for(obj = mObjects.begin();obj != mObjects.end();obj++)
            {
                if((obj->mActiveLayers&(1<<nextlayer)))
                    break;
            }
        }

        std::vector<Ogre::Controller<Ogre::Real> >::const_iterator ctrl(ctrls->begin());
        for(;ctrl != ctrls->end();ctrl++)
        {
            NifOgre::NodeTargetValue<Ogre::Real> *dstval;
            dstval = dynamic_cast<NifOgre::NodeTargetValue<Ogre::Real>*>(ctrl->getDestination().getPointer());
            if(dstval)
            {
                /*ssize_t idx = getLayerByName(dstval->getNode()->getName());
                if(idx >= (ssize_t)layer && idx < (ssize_t)nextlayer)*/
                    mActiveCtrls.insert(mActiveCtrls.end(), *ctrl);
            }
        }

        layer = nextlayer;
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


NifOgre::TextKeyMap::const_iterator Animation::findGroupStart(const NifOgre::TextKeyMap &keys, const std::string &groupname)
{
    NifOgre::TextKeyMap::const_iterator iter(keys.begin());
    for(;iter != keys.end();iter++)
    {
        if(iter->second.compare(0, groupname.size(), groupname) == 0 &&
           iter->second.compare(groupname.size(), 2, ": ") == 0)
            break;
    }
    return iter;
}


bool Animation::hasAnimation(const std::string &anim)
{
    if(!mSkelBase)
        return false;

    for(std::vector<ObjectInfo>::const_iterator iter(mObjects.begin());iter != mObjects.end();iter++)
    {
        if(iter->mObjectList.mTextKeys.size() == 0)
            continue;

        const NifOgre::TextKeyMap &keys = iter->mObjectList.mTextKeys.begin()->second;
        if(findGroupStart(keys, anim) != keys.end())
            return true;
    }
    return false;
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


void Animation::updatePtr(const MWWorld::Ptr &ptr)
{
    mPtr = ptr;
}


float Animation::calcAnimVelocity(const NifOgre::TextKeyMap &keys, NifOgre::NodeTargetValue<Ogre::Real> *nonaccumctrl, const Ogre::Vector3 &accum, const std::string &groupname)
{
    const std::string start = groupname+": start";
    const std::string loopstart = groupname+": loop start";
    const std::string loopstop = groupname+": loop stop";
    const std::string stop = groupname+": stop";
    float starttime = std::numeric_limits<float>::max();
    float stoptime = 0.0f;
    NifOgre::TextKeyMap::const_iterator keyiter(keys.begin());
    while(keyiter != keys.end())
    {
        if(keyiter->second == start || keyiter->second == loopstart)
            starttime = keyiter->first;
        else if(keyiter->second == loopstop || keyiter->second == stop)
        {
            stoptime = keyiter->first;
            break;
        }
        keyiter++;
    }

    if(stoptime > starttime)
    {
        Ogre::Vector3 startpos = nonaccumctrl->getTranslation(starttime) * accum;
        Ogre::Vector3 endpos = nonaccumctrl->getTranslation(stoptime) * accum;

        return startpos.distance(endpos) / (stoptime-starttime);
    }

    return 0.0f;
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


void Animation::updatePosition(Ogre::Vector3 &position)
{
    Ogre::Vector3 posdiff;

    /* Get the non-accumulation root's difference from the last update, and move the position
     * accordingly.
     */
    posdiff = (mNonAccumCtrl->getTranslation(mLayer[0].mTime) - mLastPosition) * mAccumulate;
    position += posdiff;

    /* Translate the accumulation root back to compensate for the move. */
    mLastPosition += posdiff;
    mAccumRoot->setPosition(-mLastPosition);
}

bool Animation::reset(size_t layeridx, const NifOgre::TextKeyMap &keys, NifOgre::NodeTargetValue<Ogre::Real> *nonaccumctrl, const std::string &groupname, const std::string &start, const std::string &stop, float startpoint)
{
    std::string tag = groupname+": "+start;
    NifOgre::TextKeyMap::const_iterator startkey(keys.begin());
    while(startkey != keys.end() && startkey->second != tag)
        startkey++;
    if(startkey == keys.end() && start == "loop start")
    {
        tag = groupname+": start";
        startkey = keys.begin();
        while(startkey != keys.end() && startkey->second != tag)
            startkey++;
    }
    if(startkey == keys.end())
        return false;

    tag = groupname+": "+stop;
    NifOgre::TextKeyMap::const_iterator stopkey(startkey);
    while(stopkey != keys.end() && stopkey->second != tag)
        stopkey++;
    if(stopkey == keys.end())
        return false;

    if(startkey == stopkey)
        return false;

    mLayer[layeridx].mStartKey = startkey;
    mLayer[layeridx].mLoopStartKey = startkey;
    mLayer[layeridx].mStopKey = stopkey;
    mLayer[layeridx].mNextKey = startkey;

    mLayer[layeridx].mTime = mLayer[layeridx].mStartKey->first + ((mLayer[layeridx].mStopKey->first-
                                                                   mLayer[layeridx].mStartKey->first) * startpoint);

    tag = groupname+": loop start";
    while(mLayer[layeridx].mNextKey->first <= mLayer[layeridx].mTime && mLayer[layeridx].mNextKey != mLayer[layeridx].mStopKey)
    {
        if(mLayer[layeridx].mNextKey->second == tag)
            mLayer[layeridx].mLoopStartKey = mLayer[layeridx].mNextKey;
        mLayer[layeridx].mNextKey++;
    }

    if(layeridx == 0 && nonaccumctrl)
        mLastPosition = nonaccumctrl->getTranslation(mLayer[layeridx].mTime) * mAccumulate;

    return true;
}

bool Animation::doLoop(size_t layeridx)
{
    if(mLayer[layeridx].mLoopCount == 0)
        return false;
    mLayer[layeridx].mLoopCount--;

    mLayer[layeridx].mTime = mLayer[layeridx].mLoopStartKey->first;
    mLayer[layeridx].mNextKey = mLayer[layeridx].mLoopStartKey;
    mLayer[layeridx].mNextKey++;
    mLayer[layeridx].mPlaying = true;
    if(layeridx == 0 && mNonAccumCtrl)
        mLastPosition = mNonAccumCtrl->getTranslation(mLayer[layeridx].mTime) * mAccumulate;

    return true;
}


bool Animation::handleTextKey(size_t layeridx, const NifOgre::TextKeyMap::const_iterator &key)
{
    float time = key->first;
    const std::string &evt = key->second;

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

    if(evt.compare(0, mLayer[layeridx].mGroupName.size(), mLayer[layeridx].mGroupName) != 0 ||
       evt.compare(mLayer[layeridx].mGroupName.size(), 2, ": ") != 0)
    {
        // Not ours, skip it
        return true;
    }
    size_t off = mLayer[layeridx].mGroupName.size()+2;
    size_t len = evt.size() - off;

    if(evt.compare(off, len, "start") == 0 || evt.compare(off, len, "loop start") == 0)
    {
        mLayer[layeridx].mLoopStartKey = key;
        return true;
    }

    if(evt.compare(off, len, "loop stop") == 0 || evt.compare(off, len, "stop") == 0)
    {
        if(doLoop(layeridx))
        {
            if(mLayer[layeridx].mTime >= time)
                return false;
        }
        return true;
    }

    std::cerr<< "Unhandled animation textkey: "<<evt <<std::endl;
    return true;
}


bool Animation::play(const std::string &groupname, const std::string &start, const std::string &stop, float startpoint, size_t loops)
{
    // TODO: parameterize this
    size_t layeridx = 0;

    if(!mSkelBase)
        return false;

    for(std::vector<ObjectInfo>::iterator iter(mObjects.begin());iter != mObjects.end();iter++)
        iter->mActiveLayers &= ~(1<<layeridx);

    mLayer[layeridx].mGroupName.clear();
    mLayer[layeridx].mTextKeys = NULL;
    mLayer[layeridx].mControllers = NULL;
    mLayer[layeridx].mTime = 0.0f;
    mLayer[layeridx].mLoopCount = 0;
    mLayer[layeridx].mPlaying = false;

    bool movinganim = false;
    bool foundanim = false;

    /* Look in reverse; last-inserted source has priority. */
    for(std::vector<ObjectInfo>::reverse_iterator iter(mObjects.rbegin());iter != mObjects.rend();iter++)
    {
        NifOgre::ObjectList &objlist = iter->mObjectList;
        if(objlist.mTextKeys.size() == 0)
            continue;

        const NifOgre::TextKeyMap &keys = objlist.mTextKeys.begin()->second;
        NifOgre::NodeTargetValue<Ogre::Real> *nonaccumctrl = NULL;
        if(layeridx == 0 && mNonAccumRoot)
        {
            for(size_t i = 0;i < objlist.mControllers.size();i++)
            {
                NifOgre::NodeTargetValue<Ogre::Real> *dstval;
                dstval = dynamic_cast<NifOgre::NodeTargetValue<Ogre::Real>*>(objlist.mControllers[i].getDestination().getPointer());
                if(dstval && dstval->getNode() == mNonAccumRoot)
                {
                    nonaccumctrl = dstval;
                    break;
                }
            }
        }

        if(!foundanim)
        {
            if(!reset(layeridx, keys, nonaccumctrl, groupname, start, stop, startpoint))
                continue;

            mLayer[layeridx].mGroupName = groupname;
            mLayer[layeridx].mTextKeys = &keys;
            mLayer[layeridx].mControllers = &objlist.mControllers;
            mLayer[layeridx].mLoopCount = loops;
            mLayer[layeridx].mPlaying = true;

            if(layeridx == 0)
            {
                mNonAccumCtrl = nonaccumctrl;
                mAnimVelocity = 0.0f;
            }

            iter->mActiveLayers |= (1<<layeridx);
            foundanim = true;

            if(mAccumulate == Ogre::Vector3(0.0f))
                break;
        }

        if(!nonaccumctrl)
            break;

        mAnimVelocity = calcAnimVelocity(keys, nonaccumctrl, mAccumulate, groupname);
        if(mAnimVelocity > 1.0f)
        {
            movinganim = (nonaccumctrl==mNonAccumCtrl);
            break;
        }
    }
    if(!foundanim)
        std::cerr<< "Failed to find animation "<<groupname <<std::endl;

    updateActiveControllers();

    return movinganim;
}

void Animation::disable(size_t layeridx)
{
    if(mLayer[layeridx].mGroupName.empty())
        return;

    for(std::vector<ObjectInfo>::iterator iter(mObjects.begin());iter != mObjects.end();iter++)
        iter->mActiveLayers &= ~(1<<layeridx);

    mLayer[layeridx].mGroupName.clear();
    mLayer[layeridx].mTextKeys = NULL;
    mLayer[layeridx].mControllers = NULL;
    mLayer[layeridx].mTime = 0.0f;
    mLayer[layeridx].mLoopCount = 0;
    mLayer[layeridx].mPlaying = false;

    updateActiveControllers();
}

bool Animation::getInfo(size_t layeridx, float *complete, std::string *groupname, std::string *start, std::string *stop) const
{
    if(mLayer[layeridx].mGroupName.empty())
    {
        if(complete) *complete = 0.0f;
        if(groupname) *groupname = "";
        if(start) *start = "";
        if(stop) *stop = "";
        return false;
    }

    if(complete) *complete = (mLayer[layeridx].mTime - mLayer[layeridx].mStartKey->first) /
                             (mLayer[layeridx].mStopKey->first - mLayer[layeridx].mStartKey->first);
    if(groupname) *groupname = mLayer[layeridx].mGroupName;
    if(start) *start = mLayer[layeridx].mStartKey->second.substr(mLayer[layeridx].mGroupName.size()+2);
    if(stop) *stop = mLayer[layeridx].mStopKey->second.substr(mLayer[layeridx].mGroupName.size()+2);
    return true;
}


Ogre::Vector3 Animation::runAnimation(float duration)
{
    Ogre::Vector3 movement(0.0f);

    duration *= mAnimSpeedMult;
    for(size_t layeridx = 0;layeridx < sMaxLayers;layeridx++)
    {
        if(mLayer[layeridx].mGroupName.empty())
            continue;

        float timepassed = duration;
        while(mLayer[layeridx].mPlaying)
        {
            float targetTime = mLayer[layeridx].mTime + timepassed;
            if(mLayer[layeridx].mNextKey->first > targetTime)
            {
                mLayer[layeridx].mTime = targetTime;
                if(layeridx == 0 && mNonAccumCtrl)
                    updatePosition(movement);
                break;
            }

            NifOgre::TextKeyMap::const_iterator key(mLayer[layeridx].mNextKey++);
            mLayer[layeridx].mTime = key->first;
            if(layeridx == 0 && mNonAccumCtrl)
                updatePosition(movement);

            mLayer[layeridx].mPlaying = (key != mLayer[layeridx].mStopKey);
            timepassed = targetTime - mLayer[layeridx].mTime;

            if(!handleTextKey(layeridx, key))
                break;
        }
    }

    for(size_t i = 0;i < mActiveCtrls.size();i++)
        mActiveCtrls[i].update();

    if(mSkelBase)
    {
        const Ogre::SkeletonInstance *baseinst = mSkelBase->getSkeleton();
        for(std::vector<ObjectInfo>::iterator iter(mObjects.begin());iter != mObjects.end();iter++)
        {
            Ogre::Entity *ent = iter->mObjectList.mSkelBase;
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

void Animation::showWeapons(bool showWeapon){}

}

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

Ogre::Real Animation::AnimationValue::getValue() const
{
    AnimLayerMap::const_iterator iter = mAnimation->mLayers.find(mAnimation->mAnimationName);
    if(iter != mAnimation->mLayers.end())
        return iter->second.mTime;
    return 0.0f;
}

void Animation::AnimationValue::setValue(Ogre::Real value)
{
    AnimLayerMap::iterator iter = mAnimation->mLayers.find(mAnimation->mAnimationName);
    if(iter != mAnimation->mLayers.end())
        iter->second.mTime = value;
}


void Animation::destroyObjectList(Ogre::SceneManager *sceneMgr, NifOgre::ObjectList &objects)
{
    for(size_t i = 0;i < objects.mParticles.size();i++)
        sceneMgr->destroyParticleSystem(objects.mParticles[i]);
    for(size_t i = 0;i < objects.mEntities.size();i++)
        sceneMgr->destroyEntity(objects.mEntities[i]);
    objects.mControllers.clear();
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
    mSource = NULL;
    mAnimationValuePtr.bind(OGRE_NEW AnimationValue(this));
}

Animation::~Animation()
{
    if(mInsert)
    {
        mAnimSources.clear();

        Ogre::SceneManager *sceneMgr = mInsert->getCreator();
        destroyObjectList(sceneMgr, mObjectRoot);
    }
}


void Animation::setObjectRoot(Ogre::SceneNode *node, const std::string &model, bool baseonly)
{
    OgreAssert(!mInsert, "Object already has a root!");
    mInsert = node->createChildSceneNode();

    std::string mdlname = Misc::StringUtils::lowerCase(model);
    std::string::size_type p = mdlname.rfind('\\');
    if(p == std::string::npos)
        p = mdlname.rfind('/');
    if(p != std::string::npos)
        mdlname.insert(mdlname.begin()+p+1, 'x');
    else
        mdlname.insert(mdlname.begin(), 'x');
    if(!Ogre::ResourceGroupManager::getSingleton().resourceExistsInAnyGroup(mdlname))
    {
        mdlname = model;
        Misc::StringUtils::toLower(mdlname);
    }

    mObjectRoot = (!baseonly ? NifOgre::Loader::createObjects(mInsert, mdlname) :
                               NifOgre::Loader::createObjectBase(mInsert, mdlname));
    if(mObjectRoot.mSkelBase)
    {
        mSkelBase = mObjectRoot.mSkelBase;

        Ogre::AnimationStateSet *aset = mObjectRoot.mSkelBase->getAllAnimationStates();
        Ogre::AnimationStateIterator asiter = aset->getAnimationStateIterator();
        while(asiter.hasMoreElements())
        {
            Ogre::AnimationState *state = asiter.getNext();
            state->setEnabled(false);
            state->setLoop(false);
        }

        // Set the bones as manually controlled since we're applying the
        // transformations manually
        Ogre::SkeletonInstance *skelinst = mObjectRoot.mSkelBase->getSkeleton();
        Ogre::Skeleton::BoneIterator boneiter = skelinst->getBoneIterator();
        while(boneiter.hasMoreElements())
            boneiter.getNext()->setManuallyControlled(true);
    }
    for(size_t i = 0;i < mObjectRoot.mControllers.size();i++)
    {
        if(mObjectRoot.mControllers[i].getSource().isNull())
            mObjectRoot.mControllers[i].setSource(mAnimationValuePtr);
    }
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


void Animation::addAnimSource(const std::string &model)
{
    OgreAssert(mInsert, "Object is missing a root!");
    if(!mSkelBase)
        return;

    std::string kfname = Misc::StringUtils::lowerCase(model);
    std::string::size_type p = kfname.rfind('\\');
    if(p == std::string::npos)
        p = kfname.rfind('/');
    if(p != std::string::npos)
        kfname.insert(kfname.begin()+p+1, 'x');
    else
        kfname.insert(kfname.begin(), 'x');

    if(kfname.size() > 4 && kfname.compare(kfname.size()-4, 4, ".nif") == 0)
        kfname.replace(kfname.size()-4, 4, ".kf");

    if(!Ogre::ResourceGroupManager::getSingleton().resourceExistsInAnyGroup(kfname))
        return;

    mAnimSources.push_back(AnimSource());
    NifOgre::Loader::createKfControllers(mSkelBase, kfname,
                                         mAnimSources.back().mTextKeys,
                                         mAnimSources.back().mControllers);
    if(mAnimSources.back().mTextKeys.size() == 0 || mAnimSources.back().mControllers.size() == 0)
    {
        mAnimSources.pop_back();
        return;
    }

    std::vector<Ogre::Controller<Ogre::Real> > &ctrls = mAnimSources.back().mControllers;
    NifOgre::NodeTargetValue<Ogre::Real> *dstval;

    for(size_t i = 0;i < ctrls.size();i++)
    {
        dstval = static_cast<NifOgre::NodeTargetValue<Ogre::Real>*>(ctrls[i].getDestination().getPointer());

        if(i == 0 && !mAccumRoot)
        {
            mAccumRoot = mInsert;
            mNonAccumRoot = dstval->getNode();
        }

        ctrls[i].setSource(mAnimationValuePtr);
    }
}

void Animation::clearAnimSources()
{
    mLayers.clear();

    mSource = NULL;
    mAnimationName.empty();

    mNonAccumCtrl = NULL;
    mAnimVelocity = 0.0f;

    mLastPosition = Ogre::Vector3(0.0f);
    mAccumRoot = NULL;
    mNonAccumRoot = NULL;

    mAnimSources.clear();
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
    AnimSourceList::const_iterator iter(mAnimSources.begin());
    for(;iter != mAnimSources.end();iter++)
    {
        const NifOgre::TextKeyMap &keys = iter->mTextKeys;
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


void Animation::updatePosition(float time, Ogre::Vector3 &position)
{
    Ogre::Vector3 posdiff;

    /* Get the non-accumulation root's difference from the last update, and move the position
     * accordingly.
     */
    posdiff = (mNonAccumCtrl->getTranslation(time) - mLastPosition) * mAccumulate;
    position += posdiff;

    /* Translate the accumulation root back to compensate for the move. */
    mLastPosition += posdiff;
    mAccumRoot->setPosition(-mLastPosition);
}

bool Animation::reset(AnimLayer &layer, const NifOgre::TextKeyMap &keys, NifOgre::NodeTargetValue<Ogre::Real> *nonaccumctrl, const std::string &groupname, const std::string &start, const std::string &stop, float startpoint)
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

    layer.mStartKey = startkey;
    layer.mLoopStartKey = startkey;
    layer.mStopKey = stopkey;
    layer.mNextKey = startkey;

    layer.mTime = layer.mStartKey->first + ((layer.mStopKey->first - layer.mStartKey->first) * startpoint);

    tag = groupname+": loop start";
    while(layer.mNextKey->first <= layer.mTime && layer.mNextKey != layer.mStopKey)
    {
        if(layer.mNextKey->second == tag)
            layer.mLoopStartKey = layer.mNextKey;
        layer.mNextKey++;
    }

    if(nonaccumctrl)
        mLastPosition = nonaccumctrl->getTranslation(layer.mTime) * mAccumulate;

    return true;
}

bool Animation::doLoop(AnimLayer &layer)
{
    if(layer.mLoopCount == 0)
        return false;
    layer.mLoopCount--;

    layer.mTime = layer.mLoopStartKey->first;
    layer.mNextKey = layer.mLoopStartKey;
    layer.mNextKey++;
    layer.mPlaying = true;
    if(mNonAccumCtrl)
        mLastPosition = mNonAccumCtrl->getTranslation(layer.mTime) * mAccumulate;

    return true;
}


bool Animation::handleTextKey(AnimLayer &layer, const std::string &groupname, const NifOgre::TextKeyMap::const_iterator &key)
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

    if(evt.compare(0, groupname.size(), groupname) != 0 ||
       evt.compare(groupname.size(), 2, ": ") != 0)
    {
        // Not ours, skip it
        return true;
    }
    size_t off = groupname.size()+2;
    size_t len = evt.size() - off;

    if(evt.compare(off, len, "start") == 0 || evt.compare(off, len, "loop start") == 0)
    {
        layer.mLoopStartKey = key;
        return true;
    }

    if(evt.compare(off, len, "loop stop") == 0 || evt.compare(off, len, "stop") == 0)
    {
        if(doLoop(layer))
        {
            if(layer.mTime >= time)
                return false;
        }
        return true;
    }

    std::cerr<< "Unhandled animation textkey: "<<evt <<std::endl;
    return true;
}


bool Animation::play(const std::string &groupname, const std::string &start, const std::string &stop, float startpoint, size_t loops)
{
    if(!mSkelBase || groupname.empty())
        return false;

    AnimLayerMap::iterator layeriter = mLayers.find(groupname);
    if(layeriter != mLayers.end())
        mLayers.erase(layeriter);
    // HACK: Don't clear all active animations
    mLayers.clear();

    bool movinganim = false;
    bool foundanim = false;

    /* Look in reverse; last-inserted source has priority. */
    AnimSourceList::reverse_iterator iter(mAnimSources.rbegin());
    for(;iter != mAnimSources.rend();iter++)
    {
        const NifOgre::TextKeyMap &keys = iter->mTextKeys;
        NifOgre::NodeTargetValue<Ogre::Real> *nonaccumctrl = NULL;
        if(mNonAccumRoot)
        {
            for(size_t i = 0;i < iter->mControllers.size();i++)
            {
                NifOgre::NodeTargetValue<Ogre::Real> *dstval;
                dstval = dynamic_cast<NifOgre::NodeTargetValue<Ogre::Real>*>(iter->mControllers[i].getDestination().getPointer());
                if(dstval && dstval->getNode() == mNonAccumRoot)
                {
                    nonaccumctrl = dstval;
                    break;
                }
            }
        }

        if(!foundanim)
        {
            AnimLayer layer;
            if(!reset(layer, keys, nonaccumctrl, groupname, start, stop, startpoint))
                continue;
            foundanim = true;

            layer.mLoopCount = loops;
            layer.mPlaying = true;
            mLayers[groupname] = layer;

            // FIXME
            mSource = &*iter;
            mAnimationName = groupname;

            mNonAccumCtrl = nonaccumctrl;
            mAnimVelocity = 0.0f;

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

    return movinganim;
}

bool Animation::getInfo(const std::string &groupname, float *complete, std::string *start, std::string *stop) const
{
    AnimLayerMap::const_iterator iter = mLayers.find(groupname);
    if(iter == mLayers.end())
    {
        if(complete) *complete = 0.0f;
        if(start) *start = "";
        if(stop) *stop = "";
        return false;
    }

    if(complete) *complete = (iter->second.mTime - iter->second.mStartKey->first) /
                             (iter->second.mStopKey->first - iter->second.mStartKey->first);
    if(start) *start = iter->second.mStartKey->second.substr(groupname.size()+2);
    if(stop) *stop = iter->second.mStopKey->second.substr(groupname.size()+2);
    return true;
}


Ogre::Vector3 Animation::runAnimation(float duration)
{
    Ogre::Vector3 movement(0.0f);

    duration *= mAnimSpeedMult;
    AnimLayerMap::iterator layeriter = mLayers.begin();
    for(;layeriter != mLayers.end();layeriter++)
    {
        AnimLayer &layer = layeriter->second;
        float timepassed = duration;
        while(layer.mPlaying)
        {
            float targetTime = layer.mTime + timepassed;
            if(layer.mNextKey->first > targetTime)
            {
                layer.mTime = targetTime;
                if(mNonAccumCtrl)
                    updatePosition(layer.mTime, movement);
                break;
            }

            NifOgre::TextKeyMap::const_iterator key(layer.mNextKey++);
            layer.mTime = key->first;
            if(mNonAccumCtrl)
                updatePosition(layer.mTime, movement);

            layer.mPlaying = (key != layer.mStopKey);
            timepassed = targetTime - layer.mTime;

            if(!handleTextKey(layer, layeriter->first, key))
                break;
        }
    }

    for(size_t i = 0;i < mObjectRoot.mControllers.size();i++)
        mObjectRoot.mControllers[i].update();
    if(mSource)
    {
        for(size_t i = 0;i < mSource->mControllers.size();i++)
            mSource->mControllers[i].update();
    }

    if(mSkelBase)
    {
        // HACK: Dirty the animation state set so that Ogre will apply the
        // transformations to entities this skeleton instance is shared with.
        mSkelBase->getAllAnimationStates()->_notifyDirty();
    }

    return movement;
}

void Animation::showWeapons(bool showWeapon)
{
}

}

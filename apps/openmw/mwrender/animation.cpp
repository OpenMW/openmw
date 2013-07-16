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
    AnimStateMap::const_iterator iter = mAnimation->mStates.find(mAnimationName);
    if(iter != mAnimation->mStates.end())
        return iter->second.mTime;
    return 0.0f;
}

void Animation::AnimationValue::setValue(Ogre::Real)
{
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
    , mCamera(NULL)
    , mInsert(NULL)
    , mSkelBase(NULL)
    , mAccumRoot(NULL)
    , mNonAccumRoot(NULL)
    , mNonAccumCtrl(NULL)
    , mAccumulate(0.0f)
{
    for(size_t i = 0;i < sNumGroups;i++)
        mAnimationValuePtr[i].bind(OGRE_NEW AnimationValue(this));
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
    OgreAssert(mAnimSources.size() == 0, "Setting object root while animation sources are set!");
    if(!mInsert)
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

    mSkelBase = NULL;
    destroyObjectList(mInsert->getCreator(), mObjectRoot);

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

        // Reattach any objects that have been attached to this one
        ObjectAttachMap::iterator iter = mAttachedObjects.begin();
        while(iter != mAttachedObjects.end())
        {
            if(!skelinst->hasBone(iter->second))
                mAttachedObjects.erase(iter++);
            else
            {
                mSkelBase->attachObjectToBone(iter->second, iter->first);
                iter++;
            }
        }
    }
    else
        mAttachedObjects.clear();

    for(size_t i = 0;i < mObjectRoot.mControllers.size();i++)
    {
        if(mObjectRoot.mControllers[i].getSource().isNull())
            mObjectRoot.mControllers[i].setSource(mAnimationValuePtr[0]);
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


size_t Animation::detectAnimGroup(const Ogre::Node *node)
{
    static const char sGroupRoots[sNumGroups][32] = {
        "", /* Lower body / character root */
        "Bip01 Spine1", /* Torso */
        "Bip01 L Clavicle", /* Left arm */
        "Bip01 R Clavicle", /* Right arm */
    };

    while(node)
    {
        const Ogre::String &name = node->getName();
        for(size_t i = 1;i < sNumGroups;i++)
        {
            if(name == sGroupRoots[i])
                return i;
        }

        node = node->getParent();
    }

    return 0;
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

    std::vector<Ogre::Controller<Ogre::Real> > ctrls;
    Ogre::SharedPtr<AnimSource> animsrc(OGRE_NEW AnimSource);
    NifOgre::Loader::createKfControllers(mSkelBase, kfname, animsrc->mTextKeys, ctrls);
    if(animsrc->mTextKeys.size() == 0 || ctrls.size() == 0)
        return;

    mAnimSources.push_back(animsrc);

    std::vector<Ogre::Controller<Ogre::Real> > *grpctrls = animsrc->mControllers;
    for(size_t i = 0;i < ctrls.size();i++)
    {
        NifOgre::NodeTargetValue<Ogre::Real> *dstval;
        dstval = static_cast<NifOgre::NodeTargetValue<Ogre::Real>*>(ctrls[i].getDestination().getPointer());

        size_t grp = detectAnimGroup(dstval->getNode());

        if(!mAccumRoot && grp == 0)
        {
            mAccumRoot = mInsert;
            mNonAccumRoot = dstval->getNode();
        }

        ctrls[i].setSource(mAnimationValuePtr[grp]);
        grpctrls[grp].push_back(ctrls[i]);
    }
}

void Animation::clearAnimSources()
{
    mStates.clear();

    for(size_t i = 0;i < sNumGroups;i++)
        mAnimationValuePtr[i]->setAnimName(std::string());

    mNonAccumCtrl = NULL;

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
        const NifOgre::TextKeyMap &keys = (*iter)->mTextKeys;
        if(findGroupStart(keys, anim) != keys.end())
            return true;
    }

    return false;
}


void Animation::setAccumulation(const Ogre::Vector3 &accum)
{
    mAccumulate = accum;
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

        return startpos.distance(endpos) / (stoptime - starttime);
    }

    return 0.0f;
}

float Animation::getVelocity(const std::string &groupname) const
{
    /* Look in reverse; last-inserted source has priority. */
    AnimSourceList::const_reverse_iterator animsrc(mAnimSources.rbegin());
    for(;animsrc != mAnimSources.rend();animsrc++)
    {
        const NifOgre::TextKeyMap &keys = (*animsrc)->mTextKeys;
        if(findGroupStart(keys, groupname) != keys.end())
            break;
    }
    if(animsrc == mAnimSources.rend())
        return 0.0f;

    float velocity = 0.0f;
    const NifOgre::TextKeyMap &keys = (*animsrc)->mTextKeys;
    const std::vector<Ogre::Controller<Ogre::Real> >&ctrls = (*animsrc)->mControllers[0];
    for(size_t i = 0;i < ctrls.size();i++)
    {
        NifOgre::NodeTargetValue<Ogre::Real> *dstval;
        dstval = static_cast<NifOgre::NodeTargetValue<Ogre::Real>*>(ctrls[i].getDestination().getPointer());
        if(dstval->getNode() == mNonAccumRoot)
        {
            velocity = calcAnimVelocity(keys, dstval, mAccumulate, groupname);
            break;
        }
    }

    // If there's no velocity, keep looking
    if(!(velocity > 1.0f))
    {
        AnimSourceList::const_reverse_iterator animiter = mAnimSources.rbegin();
        while(*animiter != *animsrc)
            ++animiter;

        while(!(velocity > 1.0f) && ++animiter != mAnimSources.rend())
        {
            const NifOgre::TextKeyMap &keys = (*animiter)->mTextKeys;
            const std::vector<Ogre::Controller<Ogre::Real> >&ctrls = (*animiter)->mControllers[0];
            for(size_t i = 0;i < ctrls.size();i++)
            {
                NifOgre::NodeTargetValue<Ogre::Real> *dstval;
                dstval = static_cast<NifOgre::NodeTargetValue<Ogre::Real>*>(ctrls[i].getDestination().getPointer());
                if(dstval->getNode() == mNonAccumRoot)
                {
                    velocity = calcAnimVelocity(keys, dstval, mAccumulate, groupname);
                    break;
                }
            }
        }
    }

    return velocity;
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


void Animation::updatePosition(float oldtime, float newtime, Ogre::Vector3 &position)
{
    /* Get the non-accumulation root's difference from the last update, and move the position
     * accordingly.
     */
    Ogre::Vector3 off = mNonAccumCtrl->getTranslation(newtime)*mAccumulate;
    position += off - mNonAccumCtrl->getTranslation(oldtime)*mAccumulate;

    /* Translate the accumulation root back to compensate for the move. */
    mAccumRoot->setPosition(-off);
}

bool Animation::reset(AnimState &state, const NifOgre::TextKeyMap &keys, const std::string &groupname, const std::string &start, const std::string &stop, float startpoint)
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

    state.mStartKey = startkey;
    state.mLoopStartKey = startkey;
    state.mStopKey = stopkey;
    state.mNextKey = startkey;

    state.mTime = state.mStartKey->first + ((state.mStopKey->first - state.mStartKey->first) * startpoint);

    tag = groupname+": loop start";
    while(state.mNextKey->first <= state.mTime && state.mNextKey != state.mStopKey)
    {
        if(state.mNextKey->second == tag)
            state.mLoopStartKey = state.mNextKey;
        state.mNextKey++;
    }

    return true;
}

bool Animation::doLoop(AnimState &state)
{
    if(state.mLoopCount == 0)
        return false;
    state.mLoopCount--;

    state.mTime = state.mLoopStartKey->first;
    state.mNextKey = state.mLoopStartKey;
    state.mNextKey++;
    state.mPlaying = true;

    return true;
}


bool Animation::handleTextKey(AnimState &state, const std::string &groupname, const NifOgre::TextKeyMap::const_iterator &key)
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
        state.mLoopStartKey = key;
        return true;
    }

    if(evt.compare(off, len, "loop stop") == 0 || evt.compare(off, len, "stop") == 0)
    {
        if(doLoop(state))
        {
            if(state.mTime >= time)
                return false;
        }
        return true;
    }

    if(evt.compare(off, len, "equip attach") == 0)
    {
        showWeapons(true);
        return true;
    }
    if(evt.compare(off, len, "unequip detach") == 0)
    {
        showWeapons(false);
        return true;
    }

    /* Nothing to do for these */
    if(evt.compare(off, len, "equip start") == 0 || evt.compare(off, len, "equip stop") == 0 ||
       evt.compare(off, len, "unequip start") == 0 || evt.compare(off, len, "unequip stop") == 0)
        return true;

    std::cerr<< "Unhandled animation textkey: "<<evt <<std::endl;
    return true;
}


void Animation::play(const std::string &groupname, int priority, int groups, bool autodisable, float speedmult, const std::string &start, const std::string &stop, float startpoint, size_t loops)
{
    if(!mSkelBase)
        return;

    if(groupname.empty())
    {
        resetActiveGroups();
        return;
    }

    priority = std::max(0, priority);

    AnimStateMap::iterator stateiter = mStates.begin();
    while(stateiter != mStates.end())
    {
        if(stateiter->second.mPriority == priority)
            mStates.erase(stateiter++);
        else
            stateiter++;
    }

    stateiter = mStates.find(groupname);
    if(stateiter != mStates.end())
    {
        stateiter->second.mPriority = priority;
        resetActiveGroups();
        return;
    }

    /* Look in reverse; last-inserted source has priority. */
    AnimSourceList::reverse_iterator iter(mAnimSources.rbegin());
    for(;iter != mAnimSources.rend();iter++)
    {
        AnimState state;
        if(reset(state, (*iter)->mTextKeys, groupname, start, stop, startpoint))
        {
            state.mSource = *iter;
            state.mSpeedMult = speedmult;
            state.mLoopCount = loops;
            state.mPlaying = true;
            state.mPriority = priority;
            state.mGroups = groups;
            state.mAutoDisable = autodisable;
            mStates[groupname] = state;

            break;
        }
    }
    if(iter == mAnimSources.rend())
        std::cerr<< "Failed to find animation "<<groupname <<std::endl;

    resetActiveGroups();
}

bool Animation::isPlaying(const std::string &groupname) const
{
    AnimStateMap::const_iterator state(mStates.find(groupname));
    if(state != mStates.end())
        return state->second.mPlaying;
    return false;
}

void Animation::resetActiveGroups()
{
    for(size_t grp = 0;grp < sNumGroups;grp++)
    {
        AnimStateMap::const_iterator active = mStates.end();

        AnimStateMap::const_iterator state = mStates.begin();
        for(;state != mStates.end();state++)
        {
            if(!(state->second.mGroups&(1<<grp)))
                continue;

            if(active == mStates.end() || active->second.mPriority < state->second.mPriority)
                active = state;
        }

        mAnimationValuePtr[grp]->setAnimName((active == mStates.end()) ?
                                             std::string() : active->first);
    }
    mNonAccumCtrl = NULL;

    if(!mNonAccumRoot || mAccumulate == Ogre::Vector3(0.0f))
        return;

    AnimStateMap::const_iterator state = mStates.find(mAnimationValuePtr[0]->getAnimName());
    if(state == mStates.end())
        return;

    const Ogre::SharedPtr<AnimSource> &animsrc = state->second.mSource;
    //const NifOgre::TextKeyMap &keys = animsrc->mTextKeys;
    const std::vector<Ogre::Controller<Ogre::Real> >&ctrls = animsrc->mControllers[0];
    for(size_t i = 0;i < ctrls.size();i++)
    {
        NifOgre::NodeTargetValue<Ogre::Real> *dstval;
        dstval = static_cast<NifOgre::NodeTargetValue<Ogre::Real>*>(ctrls[i].getDestination().getPointer());
        if(dstval->getNode() == mNonAccumRoot)
        {
            mNonAccumCtrl = dstval;
            break;
        }
    }
}


bool Animation::getInfo(const std::string &groupname, float *complete, float *speedmult, std::string *start, std::string *stop) const
{
    AnimStateMap::const_iterator iter = mStates.find(groupname);
    if(iter == mStates.end())
    {
        if(complete) *complete = 0.0f;
        if(speedmult) *speedmult = 0.0f;
        if(start) *start = "";
        if(stop) *stop = "";
        return false;
    }

    if(complete) *complete = (iter->second.mTime - iter->second.mStartKey->first) /
                             (iter->second.mStopKey->first - iter->second.mStartKey->first);
    if(speedmult) *speedmult = iter->second.mSpeedMult;
    if(start) *start = iter->second.mStartKey->second.substr(groupname.size()+2);
    if(stop) *stop = iter->second.mStopKey->second.substr(groupname.size()+2);
    return true;
}


void Animation::disable(const std::string &groupname)
{
    AnimStateMap::iterator iter = mStates.find(groupname);
    if(iter != mStates.end())
        mStates.erase(iter);
    resetActiveGroups();
}


Ogre::Vector3 Animation::runAnimation(float duration)
{
    Ogre::Vector3 movement(0.0f);

    AnimStateMap::iterator stateiter = mStates.begin();
    while(stateiter != mStates.end())
    {
        AnimState &state = stateiter->second;
        float timepassed = duration * state.mSpeedMult;
        while(state.mPlaying)
        {
            float targetTime = state.mTime + timepassed;
            if(state.mNextKey->first > targetTime)
            {
                if(mNonAccumCtrl && stateiter->first == mAnimationValuePtr[0]->getAnimName())
                    updatePosition(state.mTime, targetTime, movement);
                state.mTime = targetTime;
                break;
            }

            NifOgre::TextKeyMap::const_iterator key(state.mNextKey++);
            if(mNonAccumCtrl && stateiter->first == mAnimationValuePtr[0]->getAnimName())
                updatePosition(state.mTime, key->first, movement);
            state.mTime = key->first;

            state.mPlaying = (key != state.mStopKey);
            timepassed = targetTime - state.mTime;

            if(!handleTextKey(state, stateiter->first, key))
                break;
        }

        if(!state.mPlaying && state.mAutoDisable)
        {
            mStates.erase(stateiter++);
            resetActiveGroups();
        }
        else
            stateiter++;
    }

    for(size_t i = 0;i < mObjectRoot.mControllers.size();i++)
        mObjectRoot.mControllers[i].update();

    // Apply group controllers
    for(size_t grp = 0;grp < sNumGroups;grp++)
    {
        const std::string &name = mAnimationValuePtr[grp]->getAnimName();
        if(!name.empty() && (stateiter=mStates.find(name)) != mStates.end())
        {
            const Ogre::SharedPtr<AnimSource> &src = stateiter->second.mSource;
            for(size_t i = 0;i < src->mControllers[grp].size();i++)
                src->mControllers[grp][i].update();
        }
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

bool Animation::isPriorityActive(int priority) const
{
    for (AnimStateMap::const_iterator it = mStates.begin(); it != mStates.end(); ++it)
        if (it->second.mPriority == priority)
            return true;
    return false;
}

Ogre::TagPoint *Animation::attachObjectToBone(const Ogre::String &bonename, Ogre::MovableObject *obj)
{
    Ogre::TagPoint *tag = NULL;
    Ogre::SkeletonInstance *skel = (mSkelBase ? mSkelBase->getSkeleton() : NULL);
    if(skel && skel->hasBone(bonename))
    {
        tag = mSkelBase->attachObjectToBone(bonename, obj);
        mAttachedObjects[obj] = bonename;
    }
    return tag;
}

void Animation::detachObjectFromBone(Ogre::MovableObject *obj)
{
    ObjectAttachMap::iterator iter = mAttachedObjects.find(obj);
    if(iter != mAttachedObjects.end())
        mAttachedObjects.erase(iter);
    mSkelBase->detachObjectFromBone(obj);
}

}

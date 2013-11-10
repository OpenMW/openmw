#include "animation.hpp"

#include <OgreSkeletonManager.h>
#include <OgreSkeletonInstance.h>
#include <OgreEntity.h>
#include <OgreSubEntity.h>
#include <OgreParticleSystem.h>
#include <OgreBone.h>
#include <OgreSubMesh.h>
#include <OgreSceneManager.h>
#include <OgreControllerManager.h>
#include <OgreStaticGeometry.h>

#include <libs/openengine/ogre/lights.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/soundmanager.hpp"
#include "../mwbase/world.hpp"

#include "../mwmechanics/character.hpp"
#include "../mwmechanics/creaturestats.hpp"
#include "../mwworld/class.hpp"
#include "../mwworld/fallback.hpp"

#include "renderconst.hpp"

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
    for(size_t i = 0;i < objects.mLights.size();i++)
    {
        Ogre::Light *light = objects.mLights[i];
        // If parent is a scene node, it was created specifically for this light. Destroy it now.
        if(light->isAttached() && !light->isParentTagPoint())
            sceneMgr->destroySceneNode(light->getParentSceneNode());
        sceneMgr->destroyLight(light);
    }
    for(size_t i = 0;i < objects.mParticles.size();i++)
        sceneMgr->destroyParticleSystem(objects.mParticles[i]);
    for(size_t i = 0;i < objects.mEntities.size();i++)
        sceneMgr->destroyEntity(objects.mEntities[i]);
    objects.mControllers.clear();
    objects.mLights.clear();
    objects.mParticles.clear();
    objects.mEntities.clear();
    objects.mSkelBase = NULL;
}

Animation::Animation(const MWWorld::Ptr &ptr, Ogre::SceneNode *node)
    : mPtr(ptr)
    , mCamera(NULL)
    , mInsert(node)
    , mSkelBase(NULL)
    , mAccumRoot(NULL)
    , mNonAccumRoot(NULL)
    , mNonAccumCtrl(NULL)
    , mAccumulate(0.0f)
    , mNullAnimationValuePtr(OGRE_NEW NullAnimationValue)
{
    for(size_t i = 0;i < sNumGroups;i++)
        mAnimationValuePtr[i].bind(OGRE_NEW AnimationValue(this));
}

Animation::~Animation()
{
    mAnimSources.clear();

    Ogre::SceneManager *sceneMgr = mInsert->getCreator();
    destroyObjectList(sceneMgr, mObjectRoot);
}


void Animation::setObjectRoot(const std::string &model, bool baseonly)
{
    OgreAssert(mAnimSources.empty(), "Setting object root while animation sources are set!");

    mSkelBase = NULL;
    destroyObjectList(mInsert->getCreator(), mObjectRoot);

    if(model.empty())
        return;

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

        // Reattach any objects that have been attached to this one
        ObjectAttachMap::iterator iter = mAttachedObjects.begin();
        while(iter != mAttachedObjects.end())
        {
            if(!skelinst->hasBone(iter->second))
                mAttachedObjects.erase(iter++);
            else
            {
                mSkelBase->attachObjectToBone(iter->second, iter->first);
                ++iter;
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


class VisQueueSet {
    Ogre::uint32 mVisFlags;
    Ogre::uint8 mSolidQueue, mTransQueue;
    Ogre::Real mDist;

public:
    VisQueueSet(Ogre::uint32 visflags, Ogre::uint8 solidqueue, Ogre::uint8 transqueue, Ogre::Real dist)
      : mVisFlags(visflags), mSolidQueue(solidqueue), mTransQueue(transqueue), mDist(dist)
    { }

    void operator()(Ogre::Entity *entity) const
    {
        if(mVisFlags != 0)
            entity->setVisibilityFlags(mVisFlags);
        entity->setRenderingDistance(mDist);

        unsigned int numsubs = entity->getNumSubEntities();
        for(unsigned int i = 0;i < numsubs;++i)
        {
            Ogre::SubEntity* subEnt = entity->getSubEntity(i);
            subEnt->setRenderQueueGroup(subEnt->getMaterial()->isTransparent() ? mTransQueue : mSolidQueue);
        }
    }

    void operator()(Ogre::ParticleSystem *psys) const
    {
        if(mVisFlags != 0)
            psys->setVisibilityFlags(mVisFlags);
        psys->setRenderingDistance(mDist);
        // TODO: Check particle material for actual transparency
        psys->setRenderQueueGroup(mTransQueue);
    }
};

void Animation::setRenderProperties(const NifOgre::ObjectList &objlist, Ogre::uint32 visflags, Ogre::uint8 solidqueue, Ogre::uint8 transqueue, Ogre::Real dist)
{
    std::for_each(objlist.mEntities.begin(), objlist.mEntities.end(),
                  VisQueueSet(visflags, solidqueue, transqueue, dist));
    std::for_each(objlist.mParticles.begin(), objlist.mParticles.end(),
                  VisQueueSet(visflags, solidqueue, transqueue, dist));
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
    if(animsrc->mTextKeys.empty() || ctrls.empty())
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
            mNonAccumRoot = dstval->getNode();
            mAccumRoot = mNonAccumRoot->getParent();
            if(!mAccumRoot)
            {
                std::cerr<< "Non-Accum root for "<<mPtr.getCellRef().mRefID<<" is skeleton root??" <<std::endl;
                mNonAccumRoot = NULL;
            }
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


void Animation::addExtraLight(Ogre::SceneManager *sceneMgr, NifOgre::ObjectList &objlist, const ESM::Light *light)
{
    const MWWorld::Fallback *fallback = MWBase::Environment::get().getWorld()->getFallback();

    const int clr = light->mData.mColor;
    Ogre::ColourValue color(((clr >> 0) & 0xFF) / 255.0f,
                            ((clr >> 8) & 0xFF) / 255.0f,
                            ((clr >> 16) & 0xFF) / 255.0f);
    const float radius = float(light->mData.mRadius);

    if((light->mData.mFlags&ESM::Light::Negative))
        color *= -1;

    objlist.mLights.push_back(sceneMgr->createLight());
    Ogre::Light *olight = objlist.mLights.back();
    olight->setDiffuseColour(color);

    Ogre::ControllerValueRealPtr src(Ogre::ControllerManager::getSingleton().getFrameTimeSource());
    Ogre::ControllerValueRealPtr dest(OGRE_NEW OEngine::Render::LightValue(olight, color));
    Ogre::ControllerFunctionRealPtr func(OGRE_NEW OEngine::Render::LightFunction(
        (light->mData.mFlags&ESM::Light::Flicker) ? OEngine::Render::LT_Flicker :
        (light->mData.mFlags&ESM::Light::FlickerSlow) ? OEngine::Render::LT_FlickerSlow :
        (light->mData.mFlags&ESM::Light::Pulse) ? OEngine::Render::LT_Pulse :
        (light->mData.mFlags&ESM::Light::PulseSlow) ? OEngine::Render::LT_PulseSlow :
        OEngine::Render::LT_Normal
    ));
    objlist.mControllers.push_back(Ogre::Controller<Ogre::Real>(src, dest, func));

    bool interior = !(mPtr.isInCell() && mPtr.getCell()->mCell->isExterior());
    bool quadratic = fallback->getFallbackBool("LightAttenuation_OutQuadInLin") ?
                     !interior : fallback->getFallbackBool("LightAttenuation_UseQuadratic");

    // with the standard 1 / (c + d*l + d*d*q) equation the attenuation factor never becomes zero,
    // so we ignore lights if their attenuation falls below this factor.
    const float threshold = 0.03;

    if (!quadratic)
    {
        float r = radius * fallback->getFallbackFloat("LightAttenuation_LinearRadiusMult");
        float attenuation = fallback->getFallbackFloat("LightAttenuation_LinearValue") / r;
        float activationRange = 1.0f / (threshold * attenuation);
        olight->setAttenuation(activationRange, 0, attenuation, 0);
    }
    else
    {
        float r = radius * fallback->getFallbackFloat("LightAttenuation_QuadraticRadiusMult");
        float attenuation = fallback->getFallbackFloat("LightAttenuation_QuadraticValue") / std::pow(r, 2);
        float activationRange = std::sqrt(1.0f / (threshold * attenuation));
        olight->setAttenuation(activationRange, 0, 0, attenuation);
    }

    // If there's an AttachLight bone, attach the light to that, otherwise put it in the center,
    if(objlist.mSkelBase && objlist.mSkelBase->getSkeleton()->hasBone("AttachLight"))
        objlist.mSkelBase->attachObjectToBone("AttachLight", olight);
    else
    {
        Ogre::AxisAlignedBox bounds = Ogre::AxisAlignedBox::BOX_NULL;
        for(size_t i = 0;i < objlist.mEntities.size();i++)
        {
            Ogre::Entity *ent = objlist.mEntities[i];
            bounds.merge(ent->getBoundingBox());
        }

        Ogre::SceneNode *node = bounds.isFinite() ? mInsert->createChildSceneNode(bounds.getCenter())
                                                  : mInsert->createChildSceneNode();
        node->attachObject(olight);
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
    const NifOgre::TextKeyMap::const_iterator groupstart = findGroupStart(keys, groupname);

    std::string starttag = groupname+": "+start;
    NifOgre::TextKeyMap::const_iterator startkey(groupstart);
    while(startkey != keys.end() && startkey->second != starttag)
        startkey++;
    if(startkey == keys.end() && start == "loop start")
    {
        starttag = groupname+": start";
        startkey = groupstart;
        while(startkey != keys.end() && startkey->second != starttag)
            startkey++;
    }
    if(startkey == keys.end())
        return false;

    const std::string stoptag = groupname+": "+stop;
    NifOgre::TextKeyMap::const_iterator stopkey(groupstart);
    while(stopkey != keys.end() && stopkey->second != stoptag)
        stopkey++;
    if(stopkey == keys.end())
        return false;

    if(startkey->first > stopkey->first)
        return false;

    state.mStartTime = startkey->first;
    state.mLoopStartTime = startkey->first;
    state.mLoopStopTime = stopkey->first;
    state.mStopTime = stopkey->first;

    state.mTime = state.mStartTime + ((state.mStopTime - state.mStartTime) * startpoint);
    if(state.mTime > state.mStartTime)
    {
        const std::string loopstarttag = groupname+": loop start";
        const std::string loopstoptag = groupname+": loop stop";
        NifOgre::TextKeyMap::const_iterator key(groupstart);
        while(key->first <= state.mTime && key != stopkey)
        {
            if(key->second == loopstarttag)
                state.mLoopStartTime = key->first;
            else if(key->second == loopstoptag)
                state.mLoopStopTime = key->first;
            key++;
        }
    }

    return true;
}


void Animation::handleTextKey(AnimState &state, const std::string &groupname, const NifOgre::TextKeyMap::const_iterator &key)
{
    //float time = key->first;
    const std::string &evt = key->second;

    if(evt.compare(0, 7, "sound: ") == 0)
    {
        MWBase::SoundManager *sndMgr = MWBase::Environment::get().getSoundManager();
        sndMgr->playSound3D(mPtr, evt.substr(7), 1.0f, 1.0f);
        return;
    }
    if(evt.compare(0, 10, "soundgen: ") == 0)
    {
        std::string sound = MWWorld::Class::get(mPtr).getSoundIdFromSndGen(mPtr, evt.substr(10));
        if(!sound.empty())
        {
            MWBase::SoundManager *sndMgr = MWBase::Environment::get().getSoundManager();
            MWBase::SoundManager::PlayType type = MWBase::SoundManager::Play_TypeSfx;
            if(evt.compare(10, evt.size()-10, "left") == 0 || evt.compare(10, evt.size()-10, "right") == 0)
                type = MWBase::SoundManager::Play_TypeFoot;
            sndMgr->playSound3D(mPtr, sound, 1.0f, 1.0f, type);
        }
        return;
    }

    if(evt.compare(0, groupname.size(), groupname) != 0 ||
       evt.compare(groupname.size(), 2, ": ") != 0)
    {
        // Not ours, skip it
        return;
    }
    size_t off = groupname.size()+2;
    size_t len = evt.size() - off;

    if(evt.compare(off, len, "loop start") == 0)
        state.mLoopStartTime = key->first;
    else if(evt.compare(off, len, "loop stop") == 0)
        state.mLoopStopTime = key->first;
    else if(evt.compare(off, len, "equip attach") == 0)
        showWeapons(true);
    else if(evt.compare(off, len, "unequip detach") == 0)
        showWeapons(false);
    else if(evt.compare(off, len, "chop hit") == 0)
        MWWorld::Class::get(mPtr).hit(mPtr, MWMechanics::CreatureStats::AT_Chop);
    else if(evt.compare(off, len, "slash hit") == 0)
        MWWorld::Class::get(mPtr).hit(mPtr, MWMechanics::CreatureStats::AT_Slash);
    else if(evt.compare(off, len, "thrust hit") == 0)
        MWWorld::Class::get(mPtr).hit(mPtr, MWMechanics::CreatureStats::AT_Thrust);
    else if(evt.compare(off, len, "hit") == 0)
        MWWorld::Class::get(mPtr).hit(mPtr);

    else if (groupname == "spellcast" && evt.substr(evt.size()-7, 7) == "release")
        MWBase::Environment::get().getWorld()->castSpell(mPtr);
}


void Animation::play(const std::string &groupname, int priority, int groups, bool autodisable, float speedmult, const std::string &start, const std::string &stop, float startpoint, size_t loops)
{
    if(!mSkelBase || mAnimSources.empty())
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
            ++stateiter;
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
        const NifOgre::TextKeyMap &textkeys = (*iter)->mTextKeys;
        AnimState state;
        if(reset(state, textkeys, groupname, start, stop, startpoint))
        {
            state.mSource = *iter;
            state.mSpeedMult = speedmult;
            state.mLoopCount = loops;
            state.mPlaying = (state.mTime < state.mStopTime);
            state.mPriority = priority;
            state.mGroups = groups;
            state.mAutoDisable = autodisable;
            mStates[groupname] = state;

            NifOgre::TextKeyMap::const_iterator textkey(textkeys.lower_bound(state.mTime));
            while(textkey != textkeys.end() && textkey->first <= state.mTime)
            {
                handleTextKey(state, groupname, textkey);
                textkey++;
            }

            if(state.mTime >= state.mLoopStopTime && state.mLoopCount > 0)
            {
                state.mLoopCount--;
                state.mTime = state.mLoopStartTime;
                state.mPlaying = true;
                if(state.mTime >= state.mLoopStopTime)
                    break;

                textkey = textkeys.lower_bound(state.mTime);
                while(textkey != textkeys.end() && textkey->first <= state.mTime)
                {
                    handleTextKey(state, groupname, textkey);
                    textkey++;
                }
            }

            break;
        }
    }
    if(iter == mAnimSources.rend())
        std::cerr<< "Failed to find animation "<<groupname<<" for "<<mPtr.getCellRef().mRefID <<std::endl;

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
        for(;state != mStates.end();++state)
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


bool Animation::getInfo(const std::string &groupname, float *complete, float *speedmult) const
{
    AnimStateMap::const_iterator iter = mStates.find(groupname);
    if(iter == mStates.end())
    {
        if(complete) *complete = 0.0f;
        if(speedmult) *speedmult = 0.0f;
        return false;
    }

    if(complete)
    {
        if(iter->second.mStopTime > iter->second.mStartTime)
            *complete = (iter->second.mTime - iter->second.mStartTime) /
                        (iter->second.mStopTime - iter->second.mStartTime);
        else
            *complete = (iter->second.mPlaying ? 0.0f : 1.0f);
    }
    if(speedmult) *speedmult = iter->second.mSpeedMult;
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
        const NifOgre::TextKeyMap &textkeys = state.mSource->mTextKeys;
        NifOgre::TextKeyMap::const_iterator textkey(textkeys.upper_bound(state.mTime));

        float timepassed = duration * state.mSpeedMult;
        while(state.mPlaying)
        {
            float targetTime;

            if(state.mTime >= state.mLoopStopTime && state.mLoopCount > 0)
                goto handle_loop;

            targetTime = state.mTime + timepassed;
            if(textkey == textkeys.end() || textkey->first > targetTime)
            {
                if(mNonAccumCtrl && stateiter->first == mAnimationValuePtr[0]->getAnimName())
                    updatePosition(state.mTime, targetTime, movement);
                state.mTime = std::min(targetTime, state.mStopTime);
            }
            else
            {
                if(mNonAccumCtrl && stateiter->first == mAnimationValuePtr[0]->getAnimName())
                    updatePosition(state.mTime, textkey->first, movement);
                state.mTime = textkey->first;
            }

            state.mPlaying = (state.mTime < state.mStopTime);
            timepassed = targetTime - state.mTime;

            while(textkey != textkeys.end() && textkey->first <= state.mTime)
            {
                handleTextKey(state, stateiter->first, textkey);
                textkey++;
            }

            if(state.mTime >= state.mLoopStopTime && state.mLoopCount > 0)
            {
            handle_loop:
                state.mLoopCount--;
                state.mTime = state.mLoopStartTime;
                state.mPlaying = true;

                textkey = textkeys.lower_bound(state.mTime);
                while(textkey != textkeys.end() && textkey->first <= state.mTime)
                {
                    handleTextKey(state, stateiter->first, textkey);
                    textkey++;
                }

                if(state.mTime >= state.mLoopStopTime)
                    break;
            }

            if(timepassed <= 0.0f)
                break;
        }

        if(!state.mPlaying && state.mAutoDisable)
        {
            mStates.erase(stateiter++);
            resetActiveGroups();
        }
        else
            ++stateiter;
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


class ToggleLight {
    bool mEnable;

public:
    ToggleLight(bool enable) : mEnable(enable) { }

    void operator()(Ogre::Light *light) const
    { light->setVisible(mEnable); }
};

void Animation::enableLights(bool enable)
{
    std::for_each(mObjectRoot.mLights.begin(), mObjectRoot.mLights.end(), ToggleLight(enable));
}


class MergeBounds {
    Ogre::AxisAlignedBox *mBounds;

public:
    MergeBounds(Ogre::AxisAlignedBox *bounds) : mBounds(bounds) { }

    void operator()(Ogre::MovableObject *obj)
    {
        mBounds->merge(obj->getWorldBoundingBox(true));
    }
};

Ogre::AxisAlignedBox Animation::getWorldBounds()
{
    Ogre::AxisAlignedBox bounds = Ogre::AxisAlignedBox::BOX_NULL;
    std::for_each(mObjectRoot.mEntities.begin(), mObjectRoot.mEntities.end(), MergeBounds(&bounds));
    return bounds;
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


ObjectAnimation::ObjectAnimation(const MWWorld::Ptr& ptr, const std::string &model)
  : Animation(ptr, ptr.getRefData().getBaseNode())
{
    setObjectRoot(model, false);

    Ogre::Vector3 extents = getWorldBounds().getSize();
    float size = std::max(std::max(extents.x, extents.y), extents.z);

    bool small = (size < Settings::Manager::getInt("small object size", "Viewing distance")) &&
                 Settings::Manager::getBool("limit small object distance", "Viewing distance");
    // do not fade out doors. that will cause holes and look stupid
    if(ptr.getTypeName().find("Door") != std::string::npos)
        small = false;

    float dist = small ? Settings::Manager::getInt("small object distance", "Viewing distance") : 0.0f;
    setRenderProperties(mObjectRoot, (mPtr.getTypeName() == typeid(ESM::Static).name()) ?
                                     (small ? RV_StaticsSmall : RV_Statics) : RV_Misc,
                        RQG_Main, RQG_Alpha, dist);
}

void ObjectAnimation::addLight(const ESM::Light *light)
{
    addExtraLight(mInsert->getCreator(), mObjectRoot, light);
}


class FindEntityTransparency {
public:
    bool operator()(Ogre::Entity *ent) const
    {
        unsigned int numsubs = ent->getNumSubEntities();
        for(unsigned int i = 0;i < numsubs;++i)
        {
            if(ent->getSubEntity(i)->getMaterial()->isTransparent())
                return true;
        }
        return false;
    }
};

bool ObjectAnimation::canBatch() const
{
    if(!mObjectRoot.mParticles.empty() || !mObjectRoot.mLights.empty() || !mObjectRoot.mControllers.empty())
        return false;
    return std::find_if(mObjectRoot.mEntities.begin(), mObjectRoot.mEntities.end(),
                        FindEntityTransparency()) == mObjectRoot.mEntities.end();
}

void ObjectAnimation::fillBatch(Ogre::StaticGeometry *sg)
{
    std::vector<Ogre::Entity*>::reverse_iterator iter = mObjectRoot.mEntities.rbegin();
    for(;iter != mObjectRoot.mEntities.rend();++iter)
    {
        Ogre::Node *node = (*iter)->getParentNode();
        sg->addEntity(*iter, node->_getDerivedPosition(), node->_getDerivedOrientation(), node->_getDerivedScale());
    }
}

}

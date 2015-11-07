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
#include <OgreSceneNode.h>
#include <OgreTechnique.h>

#include <components/esm/loadligh.hpp>
#include <components/esm/loadweap.hpp>
#include <components/esm/loadench.hpp>
#include <components/esm/loadstat.hpp>
#include <components/misc/resourcehelpers.hpp>
#include <components/settings/settings.hpp>

#include <libs/openengine/ogre/lights.hpp>

#include <extern/shiny/Main/Factory.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/soundmanager.hpp"
#include "../mwbase/world.hpp"

#include "../mwmechanics/character.hpp"
#include "../mwmechanics/creaturestats.hpp"

#include "../mwworld/class.hpp"
#include "../mwworld/fallback.hpp"
#include "../mwworld/cellstore.hpp"
#include "../mwworld/esmstore.hpp"

#include "renderconst.hpp"


namespace MWRender
{

Ogre::Real Animation::AnimationTime::getValue() const
{
    AnimStateMap::const_iterator iter = mAnimation->mStates.find(mAnimationName);
    if(iter != mAnimation->mStates.end())
        return iter->second.mTime;
    return 0.0f;
}

void Animation::AnimationTime::setValue(Ogre::Real)
{
}

Ogre::Real Animation::EffectAnimationTime::getValue() const
{
    return mTime;
}

void Animation::EffectAnimationTime::setValue(Ogre::Real)
{
}

Animation::Animation(const MWWorld::Ptr &ptr, Ogre::SceneNode *node)
    : mPtr(ptr)
    , mGlowLight(NULL)
    , mInsert(node)
    , mSkelBase(NULL)
    , mAccumRoot(NULL)
    , mNonAccumRoot(NULL)
    , mNonAccumCtrl(NULL)
    , mAccumulate(0.0f)
    , mNullAnimationTimePtr(OGRE_NEW NullAnimationTime)
{
    for(size_t i = 0;i < sNumGroups;i++)
        mAnimationTimePtr[i].bind(OGRE_NEW AnimationTime(this));
}

Animation::~Animation()
{
    setLightEffect(0);

    mEffects.clear();

    mAnimSources.clear();
}

std::string Animation::getObjectRootName() const
{
    if (mSkelBase)
        return mSkelBase->getMesh()->getName();
    return std::string();
}

void Animation::setObjectRoot(const std::string &model, bool baseonly)
{
    OgreAssert(mAnimSources.empty(), "Setting object root while animation sources are set!");

    mSkelBase = NULL;
    mObjectRoot.setNull();

    if(model.empty())
        return;

    mObjectRoot = (!baseonly ? NifOgre::Loader::createObjects(mInsert, model) :
                               NifOgre::Loader::createObjectBase(mInsert, model));

    if(mObjectRoot->mSkelBase)
    {
        mSkelBase = mObjectRoot->mSkelBase;

        Ogre::AnimationStateSet *aset = mObjectRoot->mSkelBase->getAllAnimationStates();
        Ogre::AnimationStateIterator asiter = aset->getAnimationStateIterator();
        while(asiter.hasMoreElements())
        {
            Ogre::AnimationState *state = asiter.getNext();
            state->setEnabled(false);
            state->setLoop(false);
        }

        // Set the bones as manually controlled since we're applying the
        // transformations manually
        Ogre::SkeletonInstance *skelinst = mObjectRoot->mSkelBase->getSkeleton();
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
}

struct AddGlow
{
    Ogre::Vector3* mColor;
    NifOgre::MaterialControllerManager* mMaterialControllerMgr;
    AddGlow(Ogre::Vector3* col, NifOgre::MaterialControllerManager* materialControllerMgr)
        : mColor(col)
        , mMaterialControllerMgr(materialControllerMgr)
    {}

    void operator()(Ogre::Entity* entity) const
    {
        if (!entity->getNumSubEntities())
            return;
        Ogre::MaterialPtr writableMaterial = mMaterialControllerMgr->getWritableMaterial(entity);
        sh::MaterialInstance* instance = sh::Factory::getInstance().getMaterialInstance(writableMaterial->getName());

        instance->setProperty("env_map", sh::makeProperty(new sh::BooleanValue(true)));
        instance->setProperty("env_map_color", sh::makeProperty(new sh::Vector3(mColor->x, mColor->y, mColor->z)));
        // Workaround for crash in Ogre (https://bitbucket.org/sinbad/ogre/pull-request/447/fix-shadows-crash-for-textureunitstates/diff)
        // Remove when the fix is merged
        instance->getMaterial()->setShadowCasterMaterial("openmw_shadowcaster_noalpha");
    }
};

class VisQueueSet
{
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
            sh::Factory::getInstance()._ensureMaterial(subEnt->getMaterial()->getName(), "Default");
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

void Animation::setRenderProperties(NifOgre::ObjectScenePtr objlist, Ogre::uint32 visflags, Ogre::uint8 solidqueue, Ogre::uint8 transqueue, Ogre::Real dist, bool enchantedGlow, Ogre::Vector3* glowColor)
{
    std::for_each(objlist->mEntities.begin(), objlist->mEntities.end(),
                  VisQueueSet(visflags, solidqueue, transqueue, dist));
    std::for_each(objlist->mParticles.begin(), objlist->mParticles.end(),
                  VisQueueSet(visflags, solidqueue, transqueue, dist));

    if (enchantedGlow)
        std::for_each(objlist->mEntities.begin(), objlist->mEntities.end(),
                  AddGlow(glowColor, &objlist->mMaterialControllerMgr));
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

    std::string kfname = model;
    Misc::StringUtils::toLower(kfname);

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
                std::cerr<< "Non-Accum root for "<<mPtr.getCellRef().getRefId()<<" is skeleton root??" <<std::endl;
                mNonAccumRoot = NULL;
            }
        }

        if (grp == 0 && (dstval->getNode()->getName() == "Bip01" || dstval->getNode()->getName() == "Root Bone"))
        {
            mNonAccumRoot = dstval->getNode();
            mAccumRoot = mNonAccumRoot->getParent();
            if(!mAccumRoot)
            {
                std::cerr<< "Non-Accum root for "<<mPtr.getCellRef().getRefId()<<" is skeleton root??" <<std::endl;
                mNonAccumRoot = NULL;
            }
        }

        ctrls[i].setSource(mAnimationTimePtr[grp]);
        grpctrls[grp].push_back(ctrls[i]);
    }

    for (unsigned int i = 0; i < mObjectRoot->mControllers.size(); ++i)
    {
        if (mObjectRoot->mControllers[i].getSource().isNull())
            mObjectRoot->mControllers[i].setSource(mAnimationTimePtr[0]);
    }
}

void Animation::clearAnimSources()
{
    mStates.clear();

    for(size_t i = 0;i < sNumGroups;i++)
        mAnimationTimePtr[i]->setAnimName(std::string());

    mNonAccumCtrl = NULL;

    mAccumRoot = NULL;
    mNonAccumRoot = NULL;

    mAnimSources.clear();
}


void Animation::addExtraLight(Ogre::SceneManager *sceneMgr, NifOgre::ObjectScenePtr objlist, const ESM::Light *light)
{
    const MWWorld::Fallback *fallback = MWBase::Environment::get().getWorld()->getFallback();

    const unsigned int clr = light->mData.mColor;
    Ogre::ColourValue color(((clr >> 0) & 0xFF) / 255.0f,
                            ((clr >> 8) & 0xFF) / 255.0f,
                            ((clr >> 16) & 0xFF) / 255.0f);
    const float radius = float(light->mData.mRadius);

    if((light->mData.mFlags&ESM::Light::Negative))
        color *= -1;

    objlist->mLights.push_back(sceneMgr->createLight());
    Ogre::Light *olight = objlist->mLights.back();
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
    objlist->mControllers.push_back(Ogre::Controller<Ogre::Real>(src, dest, func));

    bool interior = !(mPtr.isInCell() && mPtr.getCell()->getCell()->isExterior());

    static bool outQuadInLin = fallback->getFallbackBool("LightAttenuation_OutQuadInLin");
    static bool useQuadratic = fallback->getFallbackBool("LightAttenuation_UseQuadratic");
    static float quadraticValue = fallback->getFallbackFloat("LightAttenuation_QuadraticValue");
    static float quadraticRadiusMult = fallback->getFallbackFloat("LightAttenuation_QuadraticRadiusMult");
    static bool useLinear = fallback->getFallbackBool("LightAttenuation_UseLinear");
    static float linearRadiusMult = fallback->getFallbackFloat("LightAttenuation_LinearRadiusMult");
    static float linearValue = fallback->getFallbackFloat("LightAttenuation_LinearValue");

    bool quadratic = useQuadratic && (!outQuadInLin || !interior);


    // with the standard 1 / (c + d*l + d*d*q) equation the attenuation factor never becomes zero,
    // so we ignore lights if their attenuation falls below this factor.
    const float threshold = 0.03f;

    float quadraticAttenuation = 0;
    float linearAttenuation = 0;
    float activationRange = 0;
    if (quadratic)
    {
        float r = radius * quadraticRadiusMult;
        quadraticAttenuation = quadraticValue / std::pow(r, 2);
        activationRange = std::sqrt(1.0f / (threshold * quadraticAttenuation));
    }
    if (useLinear)
    {
        float r = radius * linearRadiusMult;
        linearAttenuation = linearValue / r;
        activationRange = std::max(activationRange, 1.0f / (threshold * linearAttenuation));
    }

    olight->setAttenuation(activationRange, 0, linearAttenuation, quadraticAttenuation);

    // If there's an AttachLight bone, attach the light to that, otherwise put it in the center,
    if(objlist->mSkelBase && objlist->mSkelBase->getSkeleton()->hasBone("AttachLight"))
        objlist->mSkelBase->attachObjectToBone("AttachLight", olight);
    else
    {
        Ogre::AxisAlignedBox bounds = Ogre::AxisAlignedBox::BOX_NULL;
        for(size_t i = 0;i < objlist->mEntities.size();i++)
        {
            Ogre::Entity *ent = objlist->mEntities[i];
            bounds.merge(ent->getBoundingBox());
        }

        Ogre::SceneNode *node = bounds.isFinite() ? mInsert->createChildSceneNode(bounds.getCenter())
                                                  : mInsert->createChildSceneNode();
        node->attachObject(olight);
    }
}


Ogre::Node* Animation::getNode(const std::string &name)
{
    if(mSkelBase)
    {
        Ogre::SkeletonInstance *skel = mSkelBase->getSkeleton();
        if(skel->hasBone(name))
            return skel->getBone(name);
    }
    return NULL;
}

Ogre::Node* Animation::getNode(int handle)
{
    if (mSkelBase)
    {
        Ogre::SkeletonInstance *skel = mSkelBase->getSkeleton();
        return skel->getBone(handle);
    }
    return NULL;
}

NifOgre::TextKeyMap::const_iterator Animation::findGroupStart(const NifOgre::TextKeyMap &keys, const std::string &groupname)
{
    NifOgre::TextKeyMap::const_iterator iter(keys.begin());
    for(;iter != keys.end();++iter)
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
    for(;iter != mAnimSources.end();++iter)
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

    // Pick the last Loop Stop key and the last Loop Start key.
    // This is required because of broken text keys in AshVampire.nif.
    // It has *two* WalkForward: Loop Stop keys at different times, the first one is used for stopping playback
    // but the animation velocity calculation uses the second one.
    // As result the animation velocity calculation is not correct, and this incorrect velocity must be replicated,
    // because otherwise the Creature's Speed (dagoth uthol) would not be sufficient to move fast enough.
    NifOgre::TextKeyMap::const_reverse_iterator keyiter(keys.rbegin());
    while(keyiter != keys.rend())
    {
        if(keyiter->second == start || keyiter->second == loopstart)
        {
            starttime = keyiter->first;
            break;
        }
        ++keyiter;
    }
    keyiter = keys.rbegin();
    while(keyiter != keys.rend())
    {
        if (keyiter->second == stop)
            stoptime = keyiter->first;
        else if (keyiter->second == loopstop)
        {
            stoptime = keyiter->first;
            break;
        }
        ++keyiter;
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
    for(;animsrc != mAnimSources.rend();++animsrc)
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
    if(bone->getName() != " " // really should be != "", but see workaround in skeleton.cpp for empty node names
            && skelsrc->hasBone(bone->getName()))
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

bool Animation::reset(AnimState &state, const NifOgre::TextKeyMap &keys, const std::string &groupname, const std::string &start, const std::string &stop, float startpoint, bool loopfallback)
{
    // Look for text keys in reverse. This normally wouldn't matter, but for some reason undeadwolf_2.nif has two
    // separate walkforward keys, and the last one is supposed to be used.
    NifOgre::TextKeyMap::const_reverse_iterator groupend(keys.rbegin());
    for(;groupend != keys.rend();++groupend)
    {
        if(groupend->second.compare(0, groupname.size(), groupname) == 0 &&
           groupend->second.compare(groupname.size(), 2, ": ") == 0)
            break;
    }

    std::string starttag = groupname+": "+start;
    NifOgre::TextKeyMap::const_reverse_iterator startkey(groupend);
    while(startkey != keys.rend() && startkey->second != starttag)
        ++startkey;
    if(startkey == keys.rend() && start == "loop start")
    {
        starttag = groupname+": start";
        startkey = groupend;
        while(startkey != keys.rend() && startkey->second != starttag)
            ++startkey;
    }
    if(startkey == keys.rend())
        return false;

    const std::string stoptag = groupname+": "+stop;
    NifOgre::TextKeyMap::const_reverse_iterator stopkey(groupend);
    while(stopkey != keys.rend()
          // We have to ignore extra garbage at the end.
          // The Scrib's idle3 animation has "Idle3: Stop." instead of "Idle3: Stop".
          // Why, just why? :(
          && (stopkey->second.size() < stoptag.size() || stopkey->second.substr(0,stoptag.size()) != stoptag))
        ++stopkey;
    if(stopkey == keys.rend())
        return false;

    if(startkey->first > stopkey->first)
        return false;

    state.mStartTime = startkey->first;
    if (loopfallback)
    {
        state.mLoopStartTime = startkey->first;
        state.mLoopStopTime = stopkey->first;
    }
    else
    {
        state.mLoopStartTime = startkey->first;
        state.mLoopStopTime = std::numeric_limits<float>::max();
    }
    state.mStopTime = stopkey->first;

    state.mTime = state.mStartTime + ((state.mStopTime - state.mStartTime) * startpoint);

    // mLoopStartTime and mLoopStopTime normally get assigned when encountering these keys while playing the animation
    // (see handleTextKey). But if startpoint is already past these keys, we need to assign them now.
    if(state.mTime > state.mStartTime)
    {
        const std::string loopstarttag = groupname+": loop start";
        const std::string loopstoptag = groupname+": loop stop";

        NifOgre::TextKeyMap::const_reverse_iterator key(groupend);
        for (; key != startkey && key != keys.rend(); ++key)
        {
            if (key->first > state.mTime)
                continue;

            if (key->second == loopstarttag)
                state.mLoopStartTime = key->first;
            else if (key->second == loopstoptag)
                state.mLoopStopTime = key->first;
        }
    }

    return true;
}

void split(const std::string &s, char delim, std::vector<std::string> &elems) {
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
        elems.push_back(item);
    }
}

void Animation::handleTextKey(AnimState &state, const std::string &groupname, const NifOgre::TextKeyMap::const_iterator &key,
                              const NifOgre::TextKeyMap& textkeys)
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
        std::string soundgen = evt.substr(10);

        // The event can optionally contain volume and pitch modifiers
        float volume=1.f, pitch=1.f;
        if (soundgen.find(" ") != std::string::npos)
        {
            std::vector<std::string> tokens;
            split(soundgen, ' ', tokens);
            soundgen = tokens[0];
            if (tokens.size() >= 2)
                volume = Ogre::StringConverter::parseReal(tokens[1]);
            if (tokens.size() >= 3)
                pitch = Ogre::StringConverter::parseReal(tokens[2]);
        }

        std::string sound = mPtr.getClass().getSoundIdFromSndGen(mPtr, soundgen);
        if(!sound.empty())
        {
            MWBase::SoundManager *sndMgr = MWBase::Environment::get().getSoundManager();
            MWBase::SoundManager::PlayType type = MWBase::SoundManager::Play_TypeSfx;
            if(evt.compare(10, evt.size()-10, "left") == 0 || evt.compare(10, evt.size()-10, "right") == 0 || evt.compare(10, evt.size()-10, "land") == 0)
                type = MWBase::SoundManager::Play_TypeFoot;
            sndMgr->playSound3D(mPtr, sound, volume, pitch, type);
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
        mPtr.getClass().hit(mPtr, ESM::Weapon::AT_Chop);
    else if(evt.compare(off, len, "slash hit") == 0)
        mPtr.getClass().hit(mPtr, ESM::Weapon::AT_Slash);
    else if(evt.compare(off, len, "thrust hit") == 0)
        mPtr.getClass().hit(mPtr, ESM::Weapon::AT_Thrust);
    else if(evt.compare(off, len, "hit") == 0)
    {
        if (groupname == "attack1")
            mPtr.getClass().hit(mPtr, ESM::Weapon::AT_Chop);
        else if (groupname == "attack2")
            mPtr.getClass().hit(mPtr, ESM::Weapon::AT_Slash);
        else if (groupname == "attack3")
            mPtr.getClass().hit(mPtr, ESM::Weapon::AT_Thrust);
        else
            mPtr.getClass().hit(mPtr);
    }
    else if (!groupname.empty() && groupname.compare(0, groupname.size()-1, "attack") == 0
             && evt.compare(off, len, "start") == 0)
    {
        NifOgre::TextKeyMap::const_iterator hitKey = key;

        // Not all animations have a hit key defined. If there is none, the hit happens with the start key.
        bool hasHitKey = false;
        while (hitKey != textkeys.end())
        {
            if (hitKey->second == groupname + ": hit")
            {
                hasHitKey = true;
                break;
            }
            if (hitKey->second == groupname + ": stop")
                break;
            ++hitKey;
        }
        if (!hasHitKey)
        {
            if (groupname == "attack1")
                mPtr.getClass().hit(mPtr, ESM::Weapon::AT_Chop);
            else if (groupname == "attack2")
                mPtr.getClass().hit(mPtr, ESM::Weapon::AT_Slash);
            else if (groupname == "attack3")
                mPtr.getClass().hit(mPtr, ESM::Weapon::AT_Thrust);
        }
    }
    else if (evt.compare(off, len, "shoot attach") == 0)
        attachArrow();
    else if (evt.compare(off, len, "shoot release") == 0)
        releaseArrow();
    else if (evt.compare(off, len, "shoot follow attach") == 0)
        attachArrow();

    else if (groupname == "spellcast" && evt.substr(evt.size()-7, 7) == "release")
    {
        // Make sure this key is actually for the RangeType we are casting. The flame atronach has
        // the same animation for all range types, so there are 3 "release" keys on the same time, one for each range type.
        // FIXME: This logic should really be in the CharacterController
        const std::string& spellid = mPtr.getClass().getCreatureStats(mPtr).getSpells().getSelectedSpell();
        const ESM::Spell* spell = MWBase::Environment::get().getWorld()->getStore().get<ESM::Spell>().find(spellid);
        const ESM::ENAMstruct &effectentry = spell->mEffects.mList.at(0);
        int range = 0;
        if (evt.compare(off, len, "self release") == 0)
            range = 0;
        else if (evt.compare(off, len, "touch release") == 0)
            range = 1;
        else if (evt.compare(off, len, "target release") == 0)
            range = 2;
        if (effectentry.mRange == range)
        {
            MWBase::Environment::get().getWorld()->castSpell(mPtr);
        }
    }

    else if (groupname == "shield" && evt.compare(off, len, "block hit") == 0)
        mPtr.getClass().block(mPtr);
}

void Animation::changeGroups(const std::string &groupname, int groups)
{
    AnimStateMap::iterator stateiter = mStates.find(groupname);
    if(stateiter != mStates.end())
    {
        if(stateiter->second.mGroups != groups)
        {
            stateiter->second.mGroups = groups;
            resetActiveGroups();
        }
        return;
    }
}

void Animation::stopLooping(const std::string& groupname)
{
    AnimStateMap::iterator stateiter = mStates.find(groupname);
    if(stateiter != mStates.end())
    {
        stateiter->second.mLoopCount = 0;
        return;
    }
}

void Animation::play(const std::string &groupname, int priority, int groups, bool autodisable, float speedmult, const std::string &start, const std::string &stop, float startpoint, size_t loops, bool loopfallback)
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
    AnimState state;
    AnimSourceList::reverse_iterator iter(mAnimSources.rbegin());
    for(;iter != mAnimSources.rend();++iter)
    {
        const NifOgre::TextKeyMap &textkeys = (*iter)->mTextKeys;
        if(reset(state, textkeys, groupname, start, stop, startpoint, loopfallback))
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
            if (state.mPlaying)
            {
                while(textkey != textkeys.end() && textkey->first <= state.mTime)
                {
                    handleTextKey(state, groupname, textkey, textkeys);
                    ++textkey;
                }
            }

            if(state.mTime >= state.mLoopStopTime && state.mLoopCount > 0)
            {
                state.mLoopCount--;
                state.mTime = state.mLoopStartTime;
                state.mPlaying = true;
                if(state.mTime >= state.mLoopStopTime)
                    break;

                NifOgre::TextKeyMap::const_iterator textkey(textkeys.lower_bound(state.mTime));
                while(textkey != textkeys.end() && textkey->first <= state.mTime)
                {
                    handleTextKey(state, groupname, textkey, textkeys);
                    ++textkey;
                }
            }

            break;
        }
    }
    if(iter == mAnimSources.rend())
        std::cerr<< "Failed to find animation "<<groupname<<" for "<<mPtr.getCellRef().getRefId() <<std::endl;

    resetActiveGroups();

    if (!state.mPlaying && mNonAccumCtrl)
    {
        // If the animation state is not playing, we need to manually apply the accumulation
        // (see updatePosition, which would be called if the animation was playing)
        mAccumRoot->setPosition(-mNonAccumCtrl->getTranslation(state.mTime)*mAccumulate);
    }
}

void Animation::adjustSpeedMult(const std::string &groupname, float speedmult)
{
    AnimStateMap::iterator state(mStates.find(groupname));
    if(state != mStates.end())
        state->second.mSpeedMult = speedmult;
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

        mAnimationTimePtr[grp]->setAnimName((active == mStates.end()) ?
                                             std::string() : active->first);
    }
    mNonAccumCtrl = NULL;

    if(!mNonAccumRoot || mAccumulate == Ogre::Vector3(0.0f))
        return;

    AnimStateMap::const_iterator state = mStates.find(mAnimationTimePtr[0]->getAnimName());
    if(state == mStates.end())
    {
        if (mAccumRoot && mNonAccumRoot)
            mAccumRoot->setPosition(-mNonAccumRoot->getPosition()*mAccumulate);
        return;
    }

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

    if (mAccumRoot && mNonAccumCtrl)
        mAccumRoot->setPosition(-mNonAccumCtrl->getTranslation(state->second.mTime)*mAccumulate);
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

float Animation::getStartTime(const std::string &groupname) const
{
    for(AnimSourceList::const_iterator iter(mAnimSources.begin()); iter != mAnimSources.end(); ++iter)
    {
        const NifOgre::TextKeyMap &keys = (*iter)->mTextKeys;

        NifOgre::TextKeyMap::const_iterator found = findGroupStart(keys, groupname);
        if(found != keys.end())
            return found->first;
    }
    return -1.f;
}

float Animation::getTextKeyTime(const std::string &textKey) const
{
    for(AnimSourceList::const_iterator iter(mAnimSources.begin()); iter != mAnimSources.end(); ++iter)
    {
        const NifOgre::TextKeyMap &keys = (*iter)->mTextKeys;

        for(NifOgre::TextKeyMap::const_iterator iterKey(keys.begin()); iterKey != keys.end(); ++iterKey)
        {
            if(iterKey->second.compare(0, textKey.size(), textKey) == 0)
                return iterKey->first;
        }
    }

    return -1.f;
}

float Animation::getCurrentTime(const std::string &groupname) const
{
    AnimStateMap::const_iterator iter = mStates.find(groupname);
    if(iter == mStates.end())
        return -1.f;

    return iter->second.mTime;
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
                if(mNonAccumCtrl && stateiter->first == mAnimationTimePtr[0]->getAnimName())
                    updatePosition(state.mTime, targetTime, movement);
                state.mTime = std::min(targetTime, state.mStopTime);
            }
            else
            {
                if(mNonAccumCtrl && stateiter->first == mAnimationTimePtr[0]->getAnimName())
                    updatePosition(state.mTime, textkey->first, movement);
                state.mTime = textkey->first;
            }

            state.mPlaying = (state.mTime < state.mStopTime);
            timepassed = targetTime - state.mTime;

            while(textkey != textkeys.end() && textkey->first <= state.mTime)
            {
                handleTextKey(state, stateiter->first, textkey, textkeys);
                ++textkey;
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
                    handleTextKey(state, stateiter->first, textkey, textkeys);
                    ++textkey;
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

    for(size_t i = 0;i < mObjectRoot->mControllers.size();i++)
    {
        if(!mObjectRoot->mControllers[i].getSource().isNull())
            mObjectRoot->mControllers[i].update();
    }

    // Apply group controllers
    for(size_t grp = 0;grp < sNumGroups;grp++)
    {
        const std::string &name = mAnimationTimePtr[grp]->getAnimName();
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

    updateEffects(duration);

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
    std::for_each(mObjectRoot->mLights.begin(), mObjectRoot->mLights.end(), ToggleLight(enable));
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
    std::for_each(mObjectRoot->mEntities.begin(), mObjectRoot->mEntities.end(), MergeBounds(&bounds));
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

bool Animation::upperBodyReady() const
{
    for (AnimStateMap::const_iterator stateiter = mStates.begin(); stateiter != mStates.end(); ++stateiter)
    {
        if((stateiter->second.mPriority > MWMechanics::Priority_Movement
                && stateiter->second.mPriority < MWMechanics::Priority_Torch)
                || stateiter->second.mPriority == MWMechanics::Priority_Death)
            return false;
    }
    return true;
}

void Animation::addEffect(const std::string &model, int effectId, bool loop, const std::string &bonename, std::string texture)
{
    // Early out if we already have this effect
    for (std::vector<EffectParams>::iterator it = mEffects.begin(); it != mEffects.end(); ++it)
        if (it->mLoop && loop && it->mEffectId == effectId && it->mBoneName == bonename)
            return;

    std::string correctedTexture = Misc::ResourceHelpers::correctTexturePath(texture);

    EffectParams params;
    params.mModelName = model;
    if (bonename.empty())
        params.mObjects = NifOgre::Loader::createObjects(mInsert, model);
    else
    {
        if (!mSkelBase)
            return;
        params.mObjects = NifOgre::Loader::createObjects(mSkelBase, bonename, "", mInsert, model);
    }

    setRenderProperties(params.mObjects, RV_Effects,
                        RQG_Main, RQG_Alpha, 0.f, false, NULL);

    params.mLoop = loop;
    params.mEffectId = effectId;
    params.mBoneName = bonename;

    for(size_t i = 0;i < params.mObjects->mControllers.size();i++)
    {
        if(params.mObjects->mControllers[i].getSource().isNull())
            params.mObjects->mControllers[i].setSource(Ogre::SharedPtr<EffectAnimationTime> (new EffectAnimationTime()));
    }


    // Do some manual adjustments on the created entities/particle systems

    // It looks like vanilla MW totally ignores lighting settings for effects attached to characters.
    // If we don't do this, some effects will look way too dark depending on the environment
    // (e.g. magic_cast_dst.nif). They were clearly meant to use emissive lighting.
    // We used to have this hack in the NIF material loader, but for effects not attached to characters
    // (e.g. ash storms) the lighting settings do seem to be in use. Is there maybe a flag we have missed?
    Ogre::ColourValue ambient = Ogre::ColourValue(0.f, 0.f, 0.f);
    Ogre::ColourValue diffuse = Ogre::ColourValue(0.f, 0.f, 0.f);
    Ogre::ColourValue specular = Ogre::ColourValue(0.f, 0.f, 0.f);
    Ogre::ColourValue emissive = Ogre::ColourValue(1.f, 1.f, 1.f);
    for(size_t i = 0;i < params.mObjects->mParticles.size(); ++i)
    {
        Ogre::ParticleSystem* partSys = params.mObjects->mParticles[i];

        Ogre::MaterialPtr mat = params.mObjects->mMaterialControllerMgr.getWritableMaterial(partSys);

        for (int t=0; t<mat->getNumTechniques(); ++t)
        {
            Ogre::Technique* tech = mat->getTechnique(t);
            for (int p=0; p<tech->getNumPasses(); ++p)
            {
                Ogre::Pass* pass = tech->getPass(p);

                pass->setAmbient(ambient);
                pass->setDiffuse(diffuse);
                pass->setSpecular(specular);
                pass->setEmissive(emissive);

                if (!texture.empty())
                {
                    for (int tex=0; tex<pass->getNumTextureUnitStates(); ++tex)
                    {
                        Ogre::TextureUnitState* tus = pass->getTextureUnitState(tex);
                        tus->setTextureName(correctedTexture);
                    }
                }
            }
        }
    }
    for(size_t i = 0;i < params.mObjects->mEntities.size(); ++i)
    {
        Ogre::Entity* ent = params.mObjects->mEntities[i];
        if (ent == params.mObjects->mSkelBase)
            continue;
        Ogre::MaterialPtr mat = params.mObjects->mMaterialControllerMgr.getWritableMaterial(ent);

        for (int t=0; t<mat->getNumTechniques(); ++t)
        {
            Ogre::Technique* tech = mat->getTechnique(t);
            for (int p=0; p<tech->getNumPasses(); ++p)
            {
                Ogre::Pass* pass = tech->getPass(p);

                pass->setAmbient(ambient);
                pass->setDiffuse(diffuse);
                pass->setSpecular(specular);
                pass->setEmissive(emissive);

                if (!texture.empty())
                {
                    for (int tex=0; tex<pass->getNumTextureUnitStates(); ++tex)
                    {
                        Ogre::TextureUnitState* tus = pass->getTextureUnitState(tex);
                        tus->setTextureName(correctedTexture);
                    }
                }
            }
        }
    }

    mEffects.push_back(params);
}

void Animation::removeEffect(int effectId)
{
    for (std::vector<EffectParams>::iterator it = mEffects.begin(); it != mEffects.end(); ++it)
    {
        if (it->mEffectId == effectId)
        {
            mEffects.erase(it);
            return;
        }
    }
}

void Animation::getLoopingEffects(std::vector<int> &out)
{
    for (std::vector<EffectParams>::iterator it = mEffects.begin(); it != mEffects.end(); ++it)
    {
        if (it->mLoop)
            out.push_back(it->mEffectId);
    }
}

void Animation::updateEffects(float duration)
{
    for (std::vector<EffectParams>::iterator it = mEffects.begin(); it != mEffects.end(); )
    {
        NifOgre::ObjectScenePtr objects = it->mObjects;
        for(size_t i = 0; i < objects->mControllers.size() ;i++)
        {
            EffectAnimationTime* value = dynamic_cast<EffectAnimationTime*>(objects->mControllers[i].getSource().get());
            if (value)
                value->addTime(duration);

            objects->mControllers[i].update();
        }

        if (objects->mControllers[0].getSource()->getValue() >= objects->mMaxControllerLength)
        {
            if (it->mLoop)
            {
                // Start from the beginning again; carry over the remainder
                float remainder = objects->mControllers[0].getSource()->getValue() - objects->mMaxControllerLength;
                for(size_t i = 0; i < objects->mControllers.size() ;i++)
                {
                    EffectAnimationTime* value = dynamic_cast<EffectAnimationTime*>(objects->mControllers[i].getSource().get());
                    if (value)
                        value->resetTime(remainder);
                }
            }
            else
            {
                it = mEffects.erase(it);
                continue;
            }
        }
         ++it;
    }
}

void Animation::preRender(Ogre::Camera *camera)
{
    for (std::vector<EffectParams>::iterator it = mEffects.begin(); it != mEffects.end(); ++it)
    {
        NifOgre::ObjectScenePtr objects = it->mObjects;
        objects->rotateBillboardNodes(camera);
    }
    mObjectRoot->rotateBillboardNodes(camera);
}

// TODO: Should not be here
Ogre::Vector3 Animation::getEnchantmentColor(MWWorld::Ptr item)
{
    Ogre::Vector3 result(1,1,1);
    std::string enchantmentName = item.getClass().getEnchantment(item);
    if (enchantmentName.empty())
        return result;
    const ESM::Enchantment* enchantment = MWBase::Environment::get().getWorld()->getStore().get<ESM::Enchantment>().find(enchantmentName);
    assert (enchantment->mEffects.mList.size());
    const ESM::MagicEffect* magicEffect = MWBase::Environment::get().getWorld()->getStore().get<ESM::MagicEffect>().find(
            enchantment->mEffects.mList.front().mEffectID);
    result.x = magicEffect->mData.mRed / 255.f;
    result.y = magicEffect->mData.mGreen / 255.f;
    result.z = magicEffect->mData.mBlue / 255.f;
    return result;
}

void Animation::setLightEffect(float effect)
{
    if (effect == 0)
    {
        if (mGlowLight)
        {
            mInsert->getCreator()->destroySceneNode(mGlowLight->getParentSceneNode());
            mInsert->getCreator()->destroyLight(mGlowLight);
            mGlowLight = NULL;
        }
    }
    else
    {
        if (!mGlowLight)
        {
            mGlowLight = mInsert->getCreator()->createLight();

            Ogre::AxisAlignedBox bounds = Ogre::AxisAlignedBox::BOX_NULL;
            for(size_t i = 0;i < mObjectRoot->mEntities.size();i++)
            {
                Ogre::Entity *ent = mObjectRoot->mEntities[i];
                bounds.merge(ent->getBoundingBox());
            }
            mInsert->createChildSceneNode(bounds.getCenter())->attachObject(mGlowLight);
        }
        mGlowLight->setType(Ogre::Light::LT_POINT);
        effect += 3;
        mGlowLight->setAttenuation(1.0f / (0.03f * (0.5f/effect)), 0, 0.5f/effect, 0);
    }
}


ObjectAnimation::ObjectAnimation(const MWWorld::Ptr& ptr, const std::string &model)
  : Animation(ptr, ptr.getRefData().getBaseNode())
{
    if (!model.empty())
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
        Ogre::Vector3 col = getEnchantmentColor(ptr);
        setRenderProperties(mObjectRoot, (mPtr.getTypeName() == typeid(ESM::Static).name()) ?
                                         (small ? RV_StaticsSmall : RV_Statics) : RV_Misc,
                            RQG_Main, RQG_Alpha, dist, !ptr.getClass().getEnchantment(ptr).empty(), &col);
    }
    else
    {
        // No model given. Create an object root anyway, so that lights can be added to it if needed.
        mObjectRoot = NifOgre::ObjectScenePtr (new NifOgre::ObjectScene(mInsert->getCreator()));
    }
}


class FindEntityTransparency {
public:
    bool operator()(Ogre::Entity *ent) const
    {
        unsigned int numsubs = ent->getNumSubEntities();
        for(unsigned int i = 0;i < numsubs;++i)
        {
            sh::Factory::getInstance()._ensureMaterial(ent->getSubEntity(i)->getMaterial()->getName(), "Default");
            if(ent->getSubEntity(i)->getMaterial()->isTransparent())
                return true;
        }
        return false;
    }
};

bool ObjectAnimation::canBatch() const
{
    if(!mObjectRoot->mParticles.empty() || !mObjectRoot->mLights.empty() || !mObjectRoot->mControllers.empty())
        return false;
    if (!mObjectRoot->mBillboardNodes.empty())
        return false;
    return std::find_if(mObjectRoot->mEntities.begin(), mObjectRoot->mEntities.end(),
                        FindEntityTransparency()) == mObjectRoot->mEntities.end();
}

void ObjectAnimation::fillBatch(Ogre::StaticGeometry *sg)
{
    std::vector<Ogre::Entity*>::reverse_iterator iter = mObjectRoot->mEntities.rbegin();
    for(;iter != mObjectRoot->mEntities.rend();++iter)
    {
        Ogre::Node *node = (*iter)->getParentNode();
        if ((*iter)->isVisible())
            sg->addEntity(*iter, node->_getDerivedPosition(), node->_getDerivedOrientation(), node->_getDerivedScale());
    }
}

}

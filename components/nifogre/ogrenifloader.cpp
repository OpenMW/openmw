/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008-2010  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: http://openmw.sourceforge.net/

  This file (ogre_nif_loader.cpp) is part of the OpenMW package.

  OpenMW is distributed as free software: you can redistribute it
  and/or modify it under the terms of the GNU General Public License
  version 3, as published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  version 3 along with this program. If not, see
  http://www.gnu.org/licenses/ .

 */

#include "ogrenifloader.hpp"

#include <algorithm>

#include <OgreTechnique.h>
#include <OgreRoot.h>
#include <OgreEntity.h>
#include <OgreSubEntity.h>
#include <OgreTagPoint.h>
#include <OgreParticleSystem.h>
#include <OgreParticleEmitter.h>
#include <OgreParticleAffector.h>
#include <OgreMeshManager.h>
#include <OgreSkeletonManager.h>
#include <OgreControllerManager.h>

#include <components/nif/node.hpp>
#include <components/misc/stringops.hpp>

#include "skeleton.hpp"
#include "material.hpp"
#include "mesh.hpp"

namespace NifOgre
{

// FIXME: Should not be here.
class DefaultFunction : public Ogre::ControllerFunction<Ogre::Real>
{
private:
    float mFrequency;
    float mPhase;
    float mStartTime;
    float mStopTime;

public:
    DefaultFunction(const Nif::Controller *ctrl, bool deltaInput)
        : Ogre::ControllerFunction<Ogre::Real>(deltaInput)
        , mFrequency(ctrl->frequency)
        , mPhase(ctrl->phase)
        , mStartTime(ctrl->timeStart)
        , mStopTime(ctrl->timeStop)
    {
        if(mDeltaInput)
            mDeltaCount = mPhase;
    }

    virtual Ogre::Real calculate(Ogre::Real value)
    {
        if(mDeltaInput)
        {
            mDeltaCount += value*mFrequency;
            if(mDeltaCount < mStartTime)
                mDeltaCount = mStopTime - std::fmod(mStartTime - mDeltaCount,
                                                    mStopTime - mStartTime);
            mDeltaCount = std::fmod(mDeltaCount - mStartTime,
                                    mStopTime - mStartTime) + mStartTime;
            return mDeltaCount;
        }

        value = std::min(mStopTime, std::max(mStartTime, value+mPhase));
        return value;
    }
};

class VisController
{
public:
    class Value : public NodeTargetValue<Ogre::Real>
    {
    private:
        std::vector<Nif::NiVisData::VisData> mData;

        bool calculate(Ogre::Real time) const
        {
            if(mData.size() == 0)
                return true;

            for(size_t i = 1;i < mData.size();i++)
            {
                if(mData[i].time > time)
                    return mData[i-1].isSet;
            }
            return mData.back().isSet;
        }

        // FIXME: We are not getting all objects here. Skinned meshes get
        // attached to the object's root node, and won't be connected via a
        // TagPoint.
        static void setVisible(Ogre::Node *node, int vis)
        {
            Ogre::Node::ChildNodeIterator iter = node->getChildIterator();
            while(iter.hasMoreElements())
            {
                node = iter.getNext();
                setVisible(node, vis);

                Ogre::TagPoint *tag = dynamic_cast<Ogre::TagPoint*>(node);
                if(tag != NULL)
                {
                    Ogre::MovableObject *obj = tag->getChildObject();
                    if(obj != NULL)
                        obj->setVisible(vis);
                }
            }
        }

    public:
        Value(Ogre::Node *target, const Nif::NiVisData *data)
          : NodeTargetValue<Ogre::Real>(target)
          , mData(data->mVis)
        { }

        virtual Ogre::Quaternion getRotation(float time) const
        { return Ogre::Quaternion(); }

        virtual Ogre::Vector3 getTranslation(float time) const
        { return Ogre::Vector3(0.0f); }

        virtual Ogre::Vector3 getScale(float time) const
        { return Ogre::Vector3(1.0f); }

        virtual Ogre::Real getValue() const
        {
            // Should not be called
            return 0.0f;
        }

        virtual void setValue(Ogre::Real time)
        {
            bool vis = calculate(time);
            setVisible(mNode, vis);
        }
    };

    typedef DefaultFunction Function;
};

class KeyframeController
{
public:
    class Value : public NodeTargetValue<Ogre::Real>
    {
    private:
        Nif::QuaternionKeyList mRotations;
        Nif::Vector3KeyList mTranslations;
        Nif::FloatKeyList mScales;

        static float interpKey(const Nif::FloatKeyList::VecType &keys, float time)
        {
            if(time <= keys.front().mTime)
                return keys.front().mValue;

            Nif::FloatKeyList::VecType::const_iterator iter(keys.begin()+1);
            for(;iter != keys.end();iter++)
            {
                if(iter->mTime < time)
                    continue;

                Nif::FloatKeyList::VecType::const_iterator last(iter-1);
                float a = (time-last->mTime) / (iter->mTime-last->mTime);
                return last->mValue + ((iter->mValue - last->mValue)*a);
            }
            return keys.back().mValue;
        }

        static Ogre::Vector3 interpKey(const Nif::Vector3KeyList::VecType &keys, float time)
        {
            if(time <= keys.front().mTime)
                return keys.front().mValue;

            Nif::Vector3KeyList::VecType::const_iterator iter(keys.begin()+1);
            for(;iter != keys.end();iter++)
            {
                if(iter->mTime < time)
                    continue;

                Nif::Vector3KeyList::VecType::const_iterator last(iter-1);
                float a = (time-last->mTime) / (iter->mTime-last->mTime);
                return last->mValue + ((iter->mValue - last->mValue)*a);
            }
            return keys.back().mValue;
        }

        static Ogre::Quaternion interpKey(const Nif::QuaternionKeyList::VecType &keys, float time)
        {
            if(time <= keys.front().mTime)
                return keys.front().mValue;

            Nif::QuaternionKeyList::VecType::const_iterator iter(keys.begin()+1);
            for(;iter != keys.end();iter++)
            {
                if(iter->mTime < time)
                    continue;

                Nif::QuaternionKeyList::VecType::const_iterator last(iter-1);
                float a = (time-last->mTime) / (iter->mTime-last->mTime);
                return Ogre::Quaternion::nlerp(a, last->mValue, iter->mValue);
            }
            return keys.back().mValue;
        }

    public:
        Value(Ogre::Node *target, const Nif::NiKeyframeData *data)
          : NodeTargetValue<Ogre::Real>(target)
          , mRotations(data->mRotations)
          , mTranslations(data->mTranslations)
          , mScales(data->mScales)
        { }

        virtual Ogre::Quaternion getRotation(float time) const
        {
            if(mRotations.mKeys.size() > 0)
                return interpKey(mRotations.mKeys, time);
            return mNode->getOrientation();
        }

        virtual Ogre::Vector3 getTranslation(float time) const
        {
            if(mTranslations.mKeys.size() > 0)
                return interpKey(mTranslations.mKeys, time);
            return mNode->getPosition();
        }

        virtual Ogre::Vector3 getScale(float time) const
        {
            if(mScales.mKeys.size() > 0)
                return Ogre::Vector3(interpKey(mScales.mKeys, time));
            return mNode->getScale();
        }

        virtual Ogre::Real getValue() const
        {
            // Should not be called
            return 0.0f;
        }

        virtual void setValue(Ogre::Real time)
        {
            if(mRotations.mKeys.size() > 0)
                mNode->setOrientation(interpKey(mRotations.mKeys, time));
            if(mTranslations.mKeys.size() > 0)
                mNode->setPosition(interpKey(mTranslations.mKeys, time));
            if(mScales.mKeys.size() > 0)
                mNode->setScale(Ogre::Vector3(interpKey(mScales.mKeys, time)));
        }
    };

    typedef DefaultFunction Function;
};

class UVController
{
public:
    class Value : public Ogre::ControllerValue<Ogre::Real>
    {
    private:
        Ogre::MaterialPtr mMaterial;
        Nif::FloatKeyList mUTrans;
        Nif::FloatKeyList mVTrans;
        Nif::FloatKeyList mUScale;
        Nif::FloatKeyList mVScale;

        static float lookupValue(const Nif::FloatKeyList &keys, float time, float def)
        {
            if(keys.mKeys.size() == 0)
                return def;

            if(time <= keys.mKeys.front().mTime)
                return keys.mKeys.front().mValue;

            Nif::FloatKeyList::VecType::const_iterator iter(keys.mKeys.begin()+1);
            for(;iter != keys.mKeys.end();iter++)
            {
                if(iter->mTime < time)
                    continue;

                Nif::FloatKeyList::VecType::const_iterator last(iter-1);
                float a = (time-last->mTime) / (iter->mTime-last->mTime);
                return last->mValue + ((iter->mValue - last->mValue)*a);
            }
            return keys.mKeys.back().mValue;
        }

    public:
        Value(const Ogre::MaterialPtr &material, const Nif::NiUVData *data)
          : mMaterial(material)
          , mUTrans(data->mKeyList[0])
          , mVTrans(data->mKeyList[1])
          , mUScale(data->mKeyList[2])
          , mVScale(data->mKeyList[3])
        { }

        virtual Ogre::Real getValue() const
        {
            // Should not be called
            return 1.0f;
        }

        virtual void setValue(Ogre::Real value)
        {
            float uTrans = lookupValue(mUTrans, value, 0.0f);
            float vTrans = lookupValue(mVTrans, value, 0.0f);
            float uScale = lookupValue(mUScale, value, 1.0f);
            float vScale = lookupValue(mVScale, value, 1.0f);

            Ogre::Material::TechniqueIterator techs = mMaterial->getTechniqueIterator();
            while(techs.hasMoreElements())
            {
                Ogre::Technique *tech = techs.getNext();
                Ogre::Technique::PassIterator passes = tech->getPassIterator();
                while(passes.hasMoreElements())
                {
                    Ogre::Pass *pass = passes.getNext();
                    Ogre::TextureUnitState *tex = pass->getTextureUnitState(0);
                    tex->setTextureScroll(uTrans, vTrans);
                    tex->setTextureScale(uScale, vScale);
                }
            }
        }
    };

    typedef DefaultFunction Function;
};

class ParticleSystemController
{
public:
    class Value : public Ogre::ControllerValue<Ogre::Real>
    {
    private:
        Ogre::ParticleSystem *mParticleSys;
        float mEmitStart;
        float mEmitStop;

    public:
        Value(Ogre::ParticleSystem *psys, const Nif::NiParticleSystemController *pctrl)
          : mParticleSys(psys)
          , mEmitStart(pctrl->startTime)
          , mEmitStop(pctrl->stopTime)
        {
        }

        Ogre::Real getValue() const
        { return 0.0f; }

        void setValue(Ogre::Real value)
        {
            mParticleSys->setEmitting(value >= mEmitStart && value < mEmitStop);
        }
    };

    typedef DefaultFunction Function;
};

class GeomMorpherController
{
public:
    class Value : public Ogre::ControllerValue<Ogre::Real>
    {
    private:
        Ogre::SubEntity *mSubEntity;
        std::vector<Nif::NiMorphData::MorphData> mMorphs;

    public:
        Value(Ogre::SubEntity *subent, const Nif::NiMorphData *data)
          : mSubEntity(subent)
          , mMorphs(data->mMorphs)
        { }

        virtual Ogre::Real getValue() const
        {
            // Should not be called
            return 0.0f;
        }

        virtual void setValue(Ogre::Real value)
        {
            // TODO: Implement
        }
    };

    typedef DefaultFunction Function;
};


/** Object creator for NIFs. This is the main class responsible for creating
 * "live" Ogre objects (entities, particle systems, controllers, etc) from
 * their NIF equivalents.
 */
class NIFObjectLoader
{
    static void warn(const std::string &msg)
    {
        std::cerr << "NIFObjectLoader: Warn: " << msg << std::endl;
    }

    static void fail(const std::string &msg)
    {
        std::cerr << "NIFObjectLoader: Fail: "<< msg << std::endl;
        abort();
    }


    static void createEntity(const std::string &name, const std::string &group,
                             Ogre::SceneManager *sceneMgr, ObjectList &objectlist,
                             const Nif::Node *node, int flags, int animflags)
    {
        const Nif::NiTriShape *shape = static_cast<const Nif::NiTriShape*>(node);

        std::string fullname = name+"@index="+Ogre::StringConverter::toString(shape->recIndex);
        if(shape->name.length() > 0)
            fullname += "@shape="+shape->name;
        Misc::StringUtils::toLower(fullname);

        Ogre::MeshManager &meshMgr = Ogre::MeshManager::getSingleton();
        if(meshMgr.getByName(fullname).isNull())
            NIFMeshLoader::createMesh(name, fullname, group, shape->recIndex);

        Ogre::Entity *entity = sceneMgr->createEntity(fullname);
        entity->setVisible(!(flags&Nif::NiNode::Flag_Hidden));

        objectlist.mEntities.push_back(entity);
        if(objectlist.mSkelBase)
        {
            if(entity->hasSkeleton())
                entity->shareSkeletonInstanceWith(objectlist.mSkelBase);
            else
            {
                int trgtid = NIFSkeletonLoader::lookupOgreBoneHandle(name, shape->recIndex);
                Ogre::Bone *trgtbone = objectlist.mSkelBase->getSkeleton()->getBone(trgtid);
                objectlist.mSkelBase->attachObjectToBone(trgtbone->getName(), entity);
            }
        }

        Nif::ControllerPtr ctrl = node->controller;
        while(!ctrl.empty())
        {
            if(ctrl->recType == Nif::RC_NiUVController)
            {
                const Nif::NiUVController *uv = static_cast<const Nif::NiUVController*>(ctrl.getPtr());

                const Ogre::MaterialPtr &material = entity->getSubEntity(0)->getMaterial();
                Ogre::ControllerValueRealPtr srcval((animflags&Nif::NiNode::AnimFlag_AutoPlay) ?
                                                    Ogre::ControllerManager::getSingleton().getFrameTimeSource() :
                                                    Ogre::ControllerValueRealPtr());
                Ogre::ControllerValueRealPtr dstval(OGRE_NEW UVController::Value(material, uv->data.getPtr()));
                Ogre::ControllerFunctionRealPtr func(OGRE_NEW UVController::Function(uv, (animflags&Nif::NiNode::AnimFlag_AutoPlay)));

                objectlist.mControllers.push_back(Ogre::Controller<Ogre::Real>(srcval, dstval, func));
            }
            else if(ctrl->recType == Nif::RC_NiGeomMorpherController)
            {
                const Nif::NiGeomMorpherController *geom = static_cast<const Nif::NiGeomMorpherController*>(ctrl.getPtr());

                Ogre::SubEntity *subent = entity->getSubEntity(0);
                Ogre::ControllerValueRealPtr srcval((animflags&Nif::NiNode::AnimFlag_AutoPlay) ?
                                                    Ogre::ControllerManager::getSingleton().getFrameTimeSource() :
                                                    Ogre::ControllerValueRealPtr());
                Ogre::ControllerValueRealPtr dstval(OGRE_NEW GeomMorpherController::Value(subent, geom->data.getPtr()));
                Ogre::ControllerFunctionRealPtr func(OGRE_NEW GeomMorpherController::Function(geom, (animflags&Nif::NiNode::AnimFlag_AutoPlay)));

                objectlist.mControllers.push_back(Ogre::Controller<Ogre::Real>(srcval, dstval, func));
            }
            ctrl = ctrl->next;
        }
    }


    static void createParticleEmitterAffectors(Ogre::ParticleSystem *partsys, const Nif::NiParticleSystemController *partctrl)
    {
        Ogre::ParticleEmitter *emitter = partsys->addEmitter("Nif");
        emitter->setParticleVelocity(partctrl->velocity - partctrl->velocityRandom*0.5f,
                                     partctrl->velocity + partctrl->velocityRandom*0.5f);
        emitter->setEmissionRate(partctrl->emitRate);
        emitter->setTimeToLive(partctrl->lifetime - partctrl->lifetimeRandom*0.5f,
                               partctrl->lifetime + partctrl->lifetimeRandom*0.5f);
        emitter->setParameter("width", Ogre::StringConverter::toString(partctrl->offsetRandom.x));
        emitter->setParameter("height", Ogre::StringConverter::toString(partctrl->offsetRandom.y));
        emitter->setParameter("depth", Ogre::StringConverter::toString(partctrl->offsetRandom.z));
        emitter->setParameter("vertical_direction", Ogre::StringConverter::toString(Ogre::Radian(partctrl->verticalDir).valueDegrees()));
        emitter->setParameter("vertical_angle", Ogre::StringConverter::toString(Ogre::Radian(partctrl->verticalAngle).valueDegrees()));
        emitter->setParameter("horizontal_direction", Ogre::StringConverter::toString(Ogre::Radian(partctrl->horizontalDir).valueDegrees()));
        emitter->setParameter("horizontal_angle", Ogre::StringConverter::toString(Ogre::Radian(partctrl->horizontalAngle).valueDegrees()));

        Nif::ExtraPtr e = partctrl->extra;
        while(!e.empty())
        {
            if(e->recType == Nif::RC_NiParticleGrowFade)
            {
                const Nif::NiParticleGrowFade *gf = static_cast<const Nif::NiParticleGrowFade*>(e.getPtr());

                Ogre::ParticleAffector *affector = partsys->addAffector("GrowFade");
                affector->setParameter("grow_time", Ogre::StringConverter::toString(gf->growTime));
                affector->setParameter("fade_time", Ogre::StringConverter::toString(gf->fadeTime));
            }
            else if(e->recType == Nif::RC_NiGravity)
            {
                const Nif::NiGravity *gr = static_cast<const Nif::NiGravity*>(e.getPtr());

                Ogre::ParticleAffector *affector = partsys->addAffector("Gravity");
                affector->setParameter("force", Ogre::StringConverter::toString(gr->mForce));
                affector->setParameter("force_type", (gr->mType==0) ? "wind" : "point");
                affector->setParameter("direction", Ogre::StringConverter::toString(gr->mDirection));
                affector->setParameter("position", Ogre::StringConverter::toString(gr->mPosition));
            }
            else if(e->recType == Nif::RC_NiParticleColorModifier)
            {
                const Nif::NiParticleColorModifier *cl = static_cast<const Nif::NiParticleColorModifier*>(e.getPtr());
                const Nif::NiColorData *clrdata = cl->data.getPtr();

                Ogre::ParticleAffector *affector = partsys->addAffector("ColourInterpolator");
                size_t num_colors = std::min<size_t>(6, clrdata->mKeyList.mKeys.size());
                for(size_t i = 0;i < num_colors;i++)
                {
                    Ogre::ColourValue color;
                    color.r = clrdata->mKeyList.mKeys[i].mValue[0];
                    color.g = clrdata->mKeyList.mKeys[i].mValue[1];
                    color.b = clrdata->mKeyList.mKeys[i].mValue[2];
                    color.a = clrdata->mKeyList.mKeys[i].mValue[3];
                    affector->setParameter("colour"+Ogre::StringConverter::toString(i),
                                           Ogre::StringConverter::toString(color));
                    affector->setParameter("time"+Ogre::StringConverter::toString(i),
                                           Ogre::StringConverter::toString(clrdata->mKeyList.mKeys[i].mTime));
                }
            }
            else if(e->recType == Nif::RC_NiParticleRotation)
            {
                // TODO: Implement (Ogre::RotationAffector?)
            }
            else
                warn("Unhandled particle modifier "+e->recName);
            e = e->extra;
        }
    }

    static void createParticleSystem(const std::string &name, const std::string &group,
                                     Ogre::SceneManager *sceneMgr, ObjectList &objectlist,
                                     const Nif::Node *partnode, int flags, int partflags)
    {
        const Nif::NiAutoNormalParticlesData *particledata = NULL;
        if(partnode->recType == Nif::RC_NiAutoNormalParticles)
            particledata = static_cast<const Nif::NiAutoNormalParticles*>(partnode)->data.getPtr();
        else if(partnode->recType == Nif::RC_NiRotatingParticles)
            particledata = static_cast<const Nif::NiRotatingParticles*>(partnode)->data.getPtr();

        std::string fullname = name+"@index="+Ogre::StringConverter::toString(partnode->recIndex);
        if(partnode->name.length() > 0)
            fullname += "@type="+partnode->name;
        Misc::StringUtils::toLower(fullname);

        Ogre::ParticleSystem *partsys = sceneMgr->createParticleSystem();

        const Nif::NiTexturingProperty *texprop = NULL;
        const Nif::NiMaterialProperty *matprop = NULL;
        const Nif::NiAlphaProperty *alphaprop = NULL;
        const Nif::NiVertexColorProperty *vertprop = NULL;
        const Nif::NiZBufferProperty *zprop = NULL;
        const Nif::NiSpecularProperty *specprop = NULL;
        const Nif::NiWireframeProperty *wireprop = NULL;
        bool needTangents = false;

        partnode->getProperties(texprop, matprop, alphaprop, vertprop, zprop, specprop, wireprop);
        partsys->setMaterialName(NIFMaterialLoader::getMaterial(particledata, fullname, group,
                                                                texprop, matprop, alphaprop,
                                                                vertprop, zprop, specprop,
                                                                wireprop, needTangents));

        partsys->setDefaultDimensions(particledata->particleRadius*2.0f,
                                        particledata->particleRadius*2.0f);
        partsys->setCullIndividually(false);
        partsys->setParticleQuota(particledata->numParticles);
        // TODO: There is probably a field or flag to specify this, as some
        // particle effects have it and some don't.
        partsys->setKeepParticlesInLocalSpace(true);

        Nif::ControllerPtr ctrl = partnode->controller;
        while(!ctrl.empty())
        {
            if(ctrl->recType == Nif::RC_NiParticleSystemController)
            {
                const Nif::NiParticleSystemController *partctrl = static_cast<const Nif::NiParticleSystemController*>(ctrl.getPtr());

                createParticleEmitterAffectors(partsys, partctrl);
                if(!partctrl->emitter.empty() && !partsys->isAttached())
                {
                    int trgtid = NIFSkeletonLoader::lookupOgreBoneHandle(name, partctrl->emitter->recIndex);
                    Ogre::Bone *trgtbone = objectlist.mSkelBase->getSkeleton()->getBone(trgtid);
                    objectlist.mSkelBase->attachObjectToBone(trgtbone->getName(), partsys);
                }

                Ogre::ControllerValueRealPtr srcval((partflags&Nif::NiNode::ParticleFlag_AutoPlay) ?
                                                    Ogre::ControllerManager::getSingleton().getFrameTimeSource() :
                                                    Ogre::ControllerValueRealPtr());
                Ogre::ControllerValueRealPtr dstval(OGRE_NEW ParticleSystemController::Value(partsys, partctrl));
                Ogre::ControllerFunctionRealPtr func(OGRE_NEW ParticleSystemController::Function(partctrl, (partflags&Nif::NiNode::ParticleFlag_AutoPlay)));

                objectlist.mControllers.push_back(Ogre::Controller<Ogre::Real>(srcval, dstval, func));
            }
            ctrl = ctrl->next;
        }

        if(!partsys->isAttached())
        {
            int trgtid = NIFSkeletonLoader::lookupOgreBoneHandle(name, partnode->recIndex);
            Ogre::Bone *trgtbone = objectlist.mSkelBase->getSkeleton()->getBone(trgtid);
            objectlist.mSkelBase->attachObjectToBone(trgtbone->getName(), partsys);
        }

        partsys->setVisible(!(flags&Nif::NiNode::Flag_Hidden));
        objectlist.mParticles.push_back(partsys);
    }


    static void createNodeControllers(const std::string &name, Nif::ControllerPtr ctrl, ObjectList &objectlist, int animflags)
    {
        do {
            if(ctrl->recType == Nif::RC_NiVisController)
            {
                const Nif::NiVisController *vis = static_cast<const Nif::NiVisController*>(ctrl.getPtr());

                int trgtid = NIFSkeletonLoader::lookupOgreBoneHandle(name, ctrl->target->recIndex);
                Ogre::Bone *trgtbone = objectlist.mSkelBase->getSkeleton()->getBone(trgtid);
                Ogre::ControllerValueRealPtr srcval((animflags&Nif::NiNode::AnimFlag_AutoPlay) ?
                                                    Ogre::ControllerManager::getSingleton().getFrameTimeSource() :
                                                    Ogre::ControllerValueRealPtr());
                Ogre::ControllerValueRealPtr dstval(OGRE_NEW VisController::Value(trgtbone, vis->data.getPtr()));
                Ogre::ControllerFunctionRealPtr func(OGRE_NEW VisController::Function(vis, (animflags&Nif::NiNode::AnimFlag_AutoPlay)));

                objectlist.mControllers.push_back(Ogre::Controller<Ogre::Real>(srcval, dstval, func));
            }
            else if(ctrl->recType == Nif::RC_NiKeyframeController)
            {
                const Nif::NiKeyframeController *key = static_cast<const Nif::NiKeyframeController*>(ctrl.getPtr());
                if(!key->data.empty())
                {
                    int trgtid = NIFSkeletonLoader::lookupOgreBoneHandle(name, ctrl->target->recIndex);
                    Ogre::Bone *trgtbone = objectlist.mSkelBase->getSkeleton()->getBone(trgtid);
                    Ogre::ControllerValueRealPtr srcval((animflags&Nif::NiNode::AnimFlag_AutoPlay) ?
                                                        Ogre::ControllerManager::getSingleton().getFrameTimeSource() :
                                                        Ogre::ControllerValueRealPtr());
                    Ogre::ControllerValueRealPtr dstval(OGRE_NEW KeyframeController::Value(trgtbone, key->data.getPtr()));
                    Ogre::ControllerFunctionRealPtr func(OGRE_NEW KeyframeController::Function(key, (animflags&Nif::NiNode::AnimFlag_AutoPlay)));

                    objectlist.mControllers.push_back(Ogre::Controller<Ogre::Real>(srcval, dstval, func));
                }
            }
            ctrl = ctrl->next;
        } while(!ctrl.empty());
    }


    static void extractTextKeys(const Nif::NiTextKeyExtraData *tk, TextKeyMap &textkeys)
    {
        for(size_t i = 0;i < tk->list.size();i++)
        {
            const std::string &str = tk->list[i].text;
            std::string::size_type pos = 0;
            while(pos < str.length())
            {
                if(::isspace(str[pos]))
                {
                    pos++;
                    continue;
                }

                std::string::size_type nextpos = std::min(str.find('\r', pos), str.find('\n', pos));
                std::string result = str.substr(pos, nextpos-pos);
                textkeys.insert(std::make_pair(tk->list[i].time, Misc::StringUtils::toLower(result)));

                pos = nextpos;
            }
        }
    }


    static void createObjects(const std::string &name, const std::string &group,
                              Ogre::SceneManager *sceneMgr, const Nif::Node *node,
                              ObjectList &objectlist, int flags, int animflags, int partflags)
    {
        // Do not create objects for the collision shape (includes all children)
        if(node->recType == Nif::RC_RootCollisionNode)
            return;

        // Marker objects: just skip the entire node branch
        /// \todo don't do this in the editor
        if (node->name.find("marker") != std::string::npos)
            return;

        if(node->recType == Nif::RC_NiBSAnimationNode)
            animflags |= node->flags;
        else if(node->recType == Nif::RC_NiBSParticleNode)
            partflags |= node->flags;
        else
            flags |= node->flags;

        Nif::ExtraPtr e = node->extra;
        while(!e.empty())
        {
            if(e->recType == Nif::RC_NiTextKeyExtraData)
            {
                const Nif::NiTextKeyExtraData *tk = static_cast<const Nif::NiTextKeyExtraData*>(e.getPtr());

                int trgtid = NIFSkeletonLoader::lookupOgreBoneHandle(name, node->recIndex);
                extractTextKeys(tk, objectlist.mTextKeys[trgtid]);
            }
            else if(e->recType == Nif::RC_NiStringExtraData)
            {
                const Nif::NiStringExtraData *sd = static_cast<const Nif::NiStringExtraData*>(e.getPtr());
                // String markers may contain important information
                // affecting the entire subtree of this obj
                if(sd->string == "MRK")
                {
                    // Marker objects. These meshes are only visible in the
                    // editor.
                    flags |= 0x80000000;
                }
            }

            e = e->extra;
        }

        if(!node->controller.empty())
            createNodeControllers(name, node->controller, objectlist, animflags);

        if(node->recType == Nif::RC_NiCamera)
        {
            /* Ignored */
        }

        if(node->recType == Nif::RC_NiTriShape && !(flags&0x80000000))
        {
            createEntity(name, group, sceneMgr, objectlist, node, flags, animflags);
        }

        if((node->recType == Nif::RC_NiAutoNormalParticles ||
            node->recType == Nif::RC_NiRotatingParticles) && !(flags&0x40000000))
        {
            createParticleSystem(name, group, sceneMgr, objectlist, node, flags, partflags);
        }

        const Nif::NiNode *ninode = dynamic_cast<const Nif::NiNode*>(node);
        if(ninode)
        {
            const Nif::NodeList &children = ninode->children;
            for(size_t i = 0;i < children.length();i++)
            {
                if(!children[i].empty())
                    createObjects(name, group, sceneMgr, children[i].getPtr(), objectlist, flags, animflags, partflags);
            }
        }
    }

    static void createSkelBase(const std::string &name, const std::string &group,
                               Ogre::SceneManager *sceneMgr, const Nif::Node *node,
                               ObjectList &objectlist)
    {
        /* This creates an empty mesh to which a skeleton gets attached. This
         * is to ensure we have an entity with a skeleton instance, even if all
         * other entities are attached to bones and not skinned. */
        Ogre::MeshManager &meshMgr = Ogre::MeshManager::getSingleton();
        if(meshMgr.getByName(name).isNull())
            NIFMeshLoader::createMesh(name, name, group, ~(size_t)0);

        objectlist.mSkelBase = sceneMgr->createEntity(name);
        objectlist.mEntities.push_back(objectlist.mSkelBase);
    }

public:
    static void load(Ogre::SceneManager *sceneMgr, ObjectList &objectlist, const std::string &name, const std::string &group, int flags=0)
    {
        Nif::NIFFile::ptr nif = Nif::NIFFile::create(name);
        if(nif->numRoots() < 1)
        {
            nif->warn("Found no root nodes in "+name+".");
            return;
        }

        const Nif::Record *r = nif->getRoot(0);
        assert(r != NULL);

        const Nif::Node *node = dynamic_cast<const Nif::Node*>(r);
        if(node == NULL)
        {
            nif->warn("First root in "+name+" was not a node, but a "+
                      r->recName+".");
            return;
        }

        if(Ogre::SkeletonManager::getSingleton().resourceExists(name) ||
           !NIFSkeletonLoader::createSkeleton(name, group, node).isNull())
        {
            // Create a base skeleton entity if this NIF needs one
            createSkelBase(name, group, sceneMgr, node, objectlist);
        }
        createObjects(name, group, sceneMgr, node, objectlist, flags, 0, 0);
    }

    static void loadKf(Ogre::Skeleton *skel, const std::string &name,
                       TextKeyMap &textKeys, std::vector<Ogre::Controller<Ogre::Real> > &ctrls)
    {
        Nif::NIFFile::ptr nif = Nif::NIFFile::create(name);
        if(nif->numRoots() < 1)
        {
            nif->warn("Found no root nodes in "+name+".");
            return;
        }

        const Nif::Record *r = nif->getRoot(0);
        assert(r != NULL);

        if(r->recType != Nif::RC_NiSequenceStreamHelper)
        {
            nif->warn("First root was not a NiSequenceStreamHelper, but a "+
                      r->recName+".");
            return;
        }
        const Nif::NiSequenceStreamHelper *seq = static_cast<const Nif::NiSequenceStreamHelper*>(r);

        Nif::ExtraPtr extra = seq->extra;
        if(extra.empty() || extra->recType != Nif::RC_NiTextKeyExtraData)
        {
            nif->warn("First extra data was not a NiTextKeyExtraData, but a "+
                      (extra.empty() ? std::string("nil") : extra->recName)+".");
            return;
        }

        extractTextKeys(static_cast<const Nif::NiTextKeyExtraData*>(extra.getPtr()), textKeys);

        extra = extra->extra;
        Nif::ControllerPtr ctrl = seq->controller;
        for(;!extra.empty() && !ctrl.empty();(extra=extra->extra),(ctrl=ctrl->next))
        {
            if(extra->recType != Nif::RC_NiStringExtraData || ctrl->recType != Nif::RC_NiKeyframeController)
            {
                nif->warn("Unexpected extra data "+extra->recName+" with controller "+ctrl->recName);
                continue;
            }

            const Nif::NiStringExtraData *strdata = static_cast<const Nif::NiStringExtraData*>(extra.getPtr());
            const Nif::NiKeyframeController *key = static_cast<const Nif::NiKeyframeController*>(ctrl.getPtr());

            if(key->data.empty())
                continue;
            if(!skel->hasBone(strdata->string))
                continue;

            Ogre::Bone *trgtbone = skel->getBone(strdata->string);
            Ogre::ControllerValueRealPtr srcval;
            Ogre::ControllerValueRealPtr dstval(OGRE_NEW KeyframeController::Value(trgtbone, key->data.getPtr()));
            Ogre::ControllerFunctionRealPtr func(OGRE_NEW KeyframeController::Function(key, false));

            ctrls.push_back(Ogre::Controller<Ogre::Real>(srcval, dstval, func));
        }
    }
};


ObjectList Loader::createObjects(Ogre::SceneNode *parentNode, std::string name, const std::string &group)
{
    ObjectList objectlist;

    Misc::StringUtils::toLower(name);
    NIFObjectLoader::load(parentNode->getCreator(), objectlist, name, group);

    for(size_t i = 0;i < objectlist.mEntities.size();i++)
    {
        Ogre::Entity *entity = objectlist.mEntities[i];
        if(!entity->isAttached())
            parentNode->attachObject(entity);
    }

    return objectlist;
}

ObjectList Loader::createObjects(Ogre::Entity *parent, const std::string &bonename,
                                 Ogre::SceneNode *parentNode,
                                 std::string name, const std::string &group)
{
    ObjectList objectlist;

    Misc::StringUtils::toLower(name);
    NIFObjectLoader::load(parentNode->getCreator(), objectlist, name, group);

    bool isskinned = false;
    for(size_t i = 0;i < objectlist.mEntities.size();i++)
    {
        Ogre::Entity *ent = objectlist.mEntities[i];
        if(objectlist.mSkelBase != ent && ent->hasSkeleton())
        {
            isskinned = true;
            break;
        }
    }

    Ogre::Vector3 scale(1.0f);
    if(bonename.find("Left") != std::string::npos)
        scale.x *= -1.0f;

    if(isskinned)
    {
        std::string filter = "@shape=tri "+bonename;
        Misc::StringUtils::toLower(filter);
        for(size_t i = 0;i < objectlist.mEntities.size();i++)
        {
            Ogre::Entity *entity = objectlist.mEntities[i];
            if(entity->hasSkeleton())
            {
                if(entity == objectlist.mSkelBase ||
                   entity->getMesh()->getName().find(filter) != std::string::npos)
                    parentNode->attachObject(entity);
            }
            else
            {
                if(entity->getMesh()->getName().find(filter) == std::string::npos)
                    entity->detachFromParent();
            }
        }
    }
    else
    {
        for(size_t i = 0;i < objectlist.mEntities.size();i++)
        {
            Ogre::Entity *entity = objectlist.mEntities[i];
            if(!entity->isAttached())
            {
                Ogre::TagPoint *tag = parent->attachObjectToBone(bonename, entity);
                tag->setScale(scale);
            }
        }
    }

    return objectlist;
}


ObjectList Loader::createObjectBase(Ogre::SceneNode *parentNode, std::string name, const std::string &group)
{
    ObjectList objectlist;

    Misc::StringUtils::toLower(name);
    NIFObjectLoader::load(parentNode->getCreator(), objectlist, name, group, 0xC0000000);

    if(objectlist.mSkelBase)
        parentNode->attachObject(objectlist.mSkelBase);

    return objectlist;
}


void Loader::createKfControllers(Ogre::Entity *skelBase,
                                 const std::string &name,
                                 TextKeyMap &textKeys,
                                 std::vector<Ogre::Controller<Ogre::Real> > &ctrls)
{
    NIFObjectLoader::loadKf(skelBase->getSkeleton(), name, textKeys, ctrls);
}


} // namespace NifOgre

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

namespace std
{

// TODO: Do something useful
ostream& operator<<(ostream &o, const NifOgre::TextKeyMap&)
{ return o; }

}

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
        {
            mDeltaCount = mPhase;
            while(mDeltaCount < mStartTime)
                mDeltaCount += (mStopTime-mStartTime);
        }
    }

    virtual Ogre::Real calculate(Ogre::Real value)
    {
        if(mDeltaInput)
        {
            mDeltaCount += value*mFrequency;
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

        virtual bool calculate(Ogre::Real time)
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

        virtual Ogre::Real getValue() const
        {
            // Should not be called
            return 1.0f;
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

    public:
        Value(Ogre::Node *target, const Nif::NiKeyframeData *data)
          : NodeTargetValue<Ogre::Real>(target)
          , mRotations(data->mRotations)
          , mTranslations(data->mTranslations)
          , mScales(data->mScales)
        { }

        virtual Ogre::Real getValue() const
        {
            // Should not be called
            return 0.0f;
        }

        virtual void setValue(Ogre::Real time)
        {
            if(mRotations.mKeys.size() > 0)
            {
                if(time <= mRotations.mKeys.front().mTime)
                    mNode->setOrientation(mRotations.mKeys.front().mValue);
                else if(time >= mRotations.mKeys.back().mTime)
                    mNode->setOrientation(mRotations.mKeys.back().mValue);
                else
                {
                    Nif::QuaternionKeyList::VecType::const_iterator iter(mRotations.mKeys.begin()+1);
                    for(;iter != mRotations.mKeys.end();iter++)
                    {
                        if(iter->mTime < time)
                            continue;

                        Nif::QuaternionKeyList::VecType::const_iterator last(iter-1);
                        float a = (time-last->mTime) / (iter->mTime-last->mTime);
                        mNode->setOrientation(Ogre::Quaternion::nlerp(a, last->mValue, iter->mValue));
                        break;
                    }
                }
            }
            if(mTranslations.mKeys.size() > 0)
            {
                if(time <= mTranslations.mKeys.front().mTime)
                    mNode->setPosition(mTranslations.mKeys.front().mValue);
                else if(time >= mTranslations.mKeys.back().mTime)
                    mNode->setPosition(mTranslations.mKeys.back().mValue);
                else
                {
                    Nif::Vector3KeyList::VecType::const_iterator iter(mTranslations.mKeys.begin()+1);
                    for(;iter != mTranslations.mKeys.end();iter++)
                    {
                        if(iter->mTime < time)
                            continue;

                        Nif::Vector3KeyList::VecType::const_iterator last(iter-1);
                        float a = (time-last->mTime) / (iter->mTime-last->mTime);
                        mNode->setPosition(last->mValue + ((iter->mValue - last->mValue)*a));
                        break;
                    }
                }
            }
            if(mScales.mKeys.size() > 0)
            {
                if(time <= mScales.mKeys.front().mTime)
                    mNode->setScale(Ogre::Vector3(mScales.mKeys.front().mValue));
                else if(time >= mScales.mKeys.back().mTime)
                    mNode->setScale(Ogre::Vector3(mScales.mKeys.back().mValue));
                else
                {
                    Nif::FloatKeyList::VecType::const_iterator iter(mScales.mKeys.begin()+1);
                    for(;iter != mScales.mKeys.end();iter++)
                    {
                        if(iter->mTime < time)
                            continue;

                        Nif::FloatKeyList::VecType::const_iterator last(iter-1);
                        float a = (time-last->mTime) / (iter->mTime-last->mTime);
                        mNode->setScale(Ogre::Vector3(last->mValue + ((iter->mValue - last->mValue)*a)));
                        break;
                    }
                }
            }
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
        Value(const Ogre::MaterialPtr &material, Nif::NiUVData *data)
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



/** Manual resource loader for NIF objects (meshes, particle systems, etc).
 * This is the main class responsible for translating the internal NIF
 * structures into something Ogre can use.
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


    static void createParticleEmitterAffectors(Ogre::ParticleSystem *partsys, const Nif::NiParticleSystemController *partctrl)
    {
        Ogre::ParticleEmitter *emitter = partsys->addEmitter("Point");
        emitter->setDirection(Ogre::Vector3(0.0f, 0.0f, std::cos(partctrl->verticalDir)));
        emitter->setAngle(Ogre::Radian(partctrl->verticalAngle));
        emitter->setParticleVelocity(partctrl->velocity-partctrl->velocityRandom,
                                     partctrl->velocity+partctrl->velocityRandom);
        emitter->setEmissionRate(partctrl->emitRate);
        emitter->setTimeToLive(partctrl->lifetime-partctrl->lifetimeRandom,
                               partctrl->lifetime+partctrl->lifetimeRandom);

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
            else if(e->recType == Nif::RC_NiParticleRotation)
            {
                // TODO: Implement (Ogre::RotationAffector?)
            }
            else if(e->recType == Nif::RC_NiParticleColorModifier)
            {
                // TODO: Implement (Ogre::ColourInterpolatorAffector?)
            }
            else if(e->recType == Nif::RC_NiGravity)
            {
                // TODO: Implement
            }
            else
                warn("Unhandled particle modifier "+e->recName);
            e = e->extra;
        }
    }

    static Ogre::ParticleSystem *createParticleSystem(const std::string &name, const std::string &group,
                                                      Ogre::SceneManager *sceneMgr, Ogre::Entity *entitybase,
                                                      const Nif::Node *partnode)
    {
        const Nif::NiAutoNormalParticlesData *particledata = NULL;
        if(partnode->recType == Nif::RC_NiAutoNormalParticles)
            particledata = static_cast<const Nif::NiAutoNormalParticles*>(partnode)->data.getPtr();
        else if(partnode->recType == Nif::RC_NiRotatingParticles)
            particledata = static_cast<const Nif::NiRotatingParticles*>(partnode)->data.getPtr();

        Ogre::ParticleSystem *partsys = sceneMgr->createParticleSystem();
        try {
            std::string fullname = name+"@index="+Ogre::StringConverter::toString(partnode->recIndex);
            if(partnode->name.length() > 0)
                fullname += "@type="+partnode->name;
            Misc::StringUtils::toLower(fullname);

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
                        Ogre::Bone *trgtbone = entitybase->getSkeleton()->getBone(trgtid);
                        entitybase->attachObjectToBone(trgtbone->getName(), partsys);
                    }
                }
                ctrl = ctrl->next;
            }

            if(!partsys->isAttached())
            {
                int trgtid = NIFSkeletonLoader::lookupOgreBoneHandle(name, partnode->recIndex);
                Ogre::Bone *trgtbone = entitybase->getSkeleton()->getBone(trgtid);
                entitybase->attachObjectToBone(trgtbone->getName(), partsys);
            }
        }
        catch(std::exception &e) {
            std::cerr<< "Particles exception: "<<e.what() <<std::endl;
            sceneMgr->destroyParticleSystem(partsys);
            partsys = NULL;
        };
        return partsys;
    }


    static void createObjects(const std::string &name, const std::string &group,
                              Ogre::SceneManager *sceneMgr, const Nif::Node *node,
                              ObjectList &objectlist, int flags=0)
    {
        // Do not create objects for the collision shape (includes all children)
        if(node->recType == Nif::RC_RootCollisionNode)
            return;

        // Marker objects: just skip the entire node branch
        /// \todo don't do this in the editor
        if (node->name.find("marker") != std::string::npos)
            return;

        flags |= node->flags;

        Nif::ExtraPtr e = node->extra;
        while(!e.empty())
        {
            if(e->recType == Nif::RC_NiStringExtraData)
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

        if(node->recType == Nif::RC_NiCamera)
        {
            int trgtid = NIFSkeletonLoader::lookupOgreBoneHandle(name, node->recIndex);
            Ogre::Bone *trgtbone = objectlist.mSkelBase->getSkeleton()->getBone(trgtid);
            objectlist.mCameras.push_back(trgtbone);
        }

        Nif::ControllerPtr ctrl = node->controller;
        while(!ctrl.empty())
        {
            if(ctrl->recType == Nif::RC_NiVisController)
            {
                const Nif::NiVisController *vis = static_cast<const Nif::NiVisController*>(ctrl.getPtr());

                int trgtid = NIFSkeletonLoader::lookupOgreBoneHandle(name, ctrl->target->recIndex);
                Ogre::Bone *trgtbone = objectlist.mSkelBase->getSkeleton()->getBone(trgtid);
                Ogre::ControllerValueRealPtr srcval; /* Filled in later */
                Ogre::ControllerValueRealPtr dstval(OGRE_NEW VisController::Value(trgtbone, vis->data.getPtr()));
                Ogre::ControllerFunctionRealPtr func(OGRE_NEW VisController::Function(vis, false));

                objectlist.mControllers.push_back(Ogre::Controller<Ogre::Real>(srcval, dstval, func));
            }
            else if(ctrl->recType == Nif::RC_NiKeyframeController)
            {
                const Nif::NiKeyframeController *key = static_cast<const Nif::NiKeyframeController*>(ctrl.getPtr());
                if(!key->data.empty())
                {
                    int trgtid = NIFSkeletonLoader::lookupOgreBoneHandle(name, ctrl->target->recIndex);
                    Ogre::Bone *trgtbone = objectlist.mSkelBase->getSkeleton()->getBone(trgtid);
                    Ogre::ControllerValueRealPtr srcval; /* Filled in later */
                    Ogre::ControllerValueRealPtr dstval(OGRE_NEW KeyframeController::Value(trgtbone, key->data.getPtr()));
                    Ogre::ControllerFunctionRealPtr func(OGRE_NEW KeyframeController::Function(key, false));

                    objectlist.mControllers.push_back(Ogre::Controller<Ogre::Real>(srcval, dstval, func));
                }
            }
            ctrl = ctrl->next;
        }

        if(node->recType == Nif::RC_NiTriShape && !(flags&0x80000000))
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
            entity->setVisible(!(flags&0x01));

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
                    Ogre::ControllerValueRealPtr srcval(Ogre::ControllerManager::getSingleton().getFrameTimeSource());
                    Ogre::ControllerValueRealPtr dstval(OGRE_NEW UVController::Value(material, uv->data.getPtr()));
                    Ogre::ControllerFunctionRealPtr func(OGRE_NEW UVController::Function(uv, true));

                    objectlist.mControllers.push_back(Ogre::Controller<Ogre::Real>(srcval, dstval, func));
                }
                ctrl = ctrl->next;
            }
        }

        if(node->recType == Nif::RC_NiAutoNormalParticles ||
           node->recType == Nif::RC_NiRotatingParticles)
        {
            Ogre::ParticleSystem *partsys = createParticleSystem(name, group, sceneMgr, objectlist.mSkelBase, node);
            if(partsys != NULL)
            {
                partsys->setVisible(!(flags&0x01));
                objectlist.mParticles.push_back(partsys);
            }
        }

        const Nif::NiNode *ninode = dynamic_cast<const Nif::NiNode*>(node);
        if(ninode)
        {
            const Nif::NodeList &children = ninode->children;
            for(size_t i = 0;i < children.length();i++)
            {
                if(!children[i].empty())
                    createObjects(name, group, sceneMgr, children[i].getPtr(), objectlist, flags);
            }
        }
    }

    static void createSkelBase(const std::string &name, const std::string &group,
                               Ogre::SceneManager *sceneMgr, const Nif::Node *node,
                               ObjectList &objectlist)
    {
        /* This creates an empty mesh to which a skeleton gets attached. This
         * is to ensure we have an entity with a skeleton instance, even if all
         * other meshes are hidden or entities attached to a specific node
         * instead of skinned. */
        Ogre::MeshManager &meshMgr = Ogre::MeshManager::getSingleton();
        if(meshMgr.getByName(name).isNull())
            NIFMeshLoader::createMesh(name, name, group, ~(size_t)0);

        objectlist.mSkelBase = sceneMgr->createEntity(name);
        objectlist.mEntities.push_back(objectlist.mSkelBase);
    }

public:
    static void load(Ogre::SceneManager *sceneMgr, ObjectList &objectlist, const std::string &name, const std::string &group)
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
        createObjects(name, group, sceneMgr, node, objectlist);
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


ObjectList Loader::createObjectBase(Ogre::SceneManager *sceneMgr, std::string name, const std::string &group)
{
    ObjectList objectlist;

    Misc::StringUtils::toLower(name);
    NIFObjectLoader::load(sceneMgr, objectlist, name, group);

    return objectlist;
}

} // namespace NifOgre

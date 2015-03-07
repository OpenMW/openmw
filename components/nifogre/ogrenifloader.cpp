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
#include <OgreEntity.h>
#include <OgreSubEntity.h>
#include <OgreTagPoint.h>
#include <OgreParticleSystem.h>
#include <OgreParticleEmitter.h>
#include <OgreParticleAffector.h>
#include <OgreMeshManager.h>
#include <OgreSkeletonManager.h>
#include <OgreControllerManager.h>
#include <OgreMaterialManager.h>
#include <OgreCamera.h>
#include <OgreSceneManager.h>
#include <OgreSkeletonInstance.h>
#include <OgreSceneNode.h>
#include <OgreMesh.h>

#include <extern/shiny/Main/Factory.hpp>

#include <components/nif/node.hpp>
#include <components/nifcache/nifcache.hpp>
#include <components/misc/stringops.hpp>
#include <components/misc/resourcehelpers.hpp>

#include "skeleton.hpp"
#include "material.hpp"
#include "mesh.hpp"
#include "controller.hpp"
#include "particles.hpp"

namespace
{

    void getAllNiNodes(const Nif::Node* node, std::vector<const Nif::NiNode*>& out)
    {
        const Nif::NiNode* ninode = dynamic_cast<const Nif::NiNode*>(node);
        if (ninode)
        {
            out.push_back(ninode);
            for (unsigned int i=0; i<ninode->children.length(); ++i)
                if (!ninode->children[i].empty())
                    getAllNiNodes(ninode->children[i].getPtr(), out);
        }
    }

}

namespace NifOgre
{

Ogre::MaterialPtr MaterialControllerManager::getWritableMaterial(Ogre::MovableObject *movable)
{
    if (mClonedMaterials.find(movable) != mClonedMaterials.end())
        return mClonedMaterials[movable];

    else
    {
        Ogre::MaterialPtr mat;
        if (Ogre::Entity* ent = dynamic_cast<Ogre::Entity*>(movable))
            mat = ent->getSubEntity(0)->getMaterial();
        else if (Ogre::ParticleSystem* partSys = dynamic_cast<Ogre::ParticleSystem*>(movable))
            mat = Ogre::MaterialManager::getSingleton().getByName(partSys->getMaterialName());

        static int count=0;
        Ogre::String newName = mat->getName() + Ogre::StringConverter::toString(count++);
        sh::Factory::getInstance().createMaterialInstance(newName, mat->getName());
        // Make sure techniques are created
        sh::Factory::getInstance()._ensureMaterial(newName, "Default");
        mat = Ogre::MaterialManager::getSingleton().getByName(newName);

        mClonedMaterials[movable] = mat;

        if (Ogre::Entity* ent = dynamic_cast<Ogre::Entity*>(movable))
            ent->getSubEntity(0)->setMaterial(mat);
        else if (Ogre::ParticleSystem* partSys = dynamic_cast<Ogre::ParticleSystem*>(movable))
            partSys->setMaterialName(mat->getName());

        return mat;
    }
}

MaterialControllerManager::~MaterialControllerManager()
{
    for (std::map<Ogre::MovableObject*, Ogre::MaterialPtr>::iterator it = mClonedMaterials.begin(); it != mClonedMaterials.end(); ++it)
    {
        sh::Factory::getInstance().destroyMaterialInstance(it->second->getName());
    }
}

ObjectScene::~ObjectScene()
{
    for(size_t i = 0;i < mLights.size();i++)
    {
        Ogre::Light *light = mLights[i];
        // If parent is a scene node, it was created specifically for this light. Destroy it now.
        if(light->isAttached() && !light->isParentTagPoint())
            mSceneMgr->destroySceneNode(light->getParentSceneNode());
        mSceneMgr->destroyLight(light);
    }
    for(size_t i = 0;i < mParticles.size();i++)
        mSceneMgr->destroyParticleSystem(mParticles[i]);
    for(size_t i = 0;i < mEntities.size();i++)
        mSceneMgr->destroyEntity(mEntities[i]);
    mControllers.clear();
    mLights.clear();
    mParticles.clear();
    mEntities.clear();
    mSkelBase = NULL;
}

void ObjectScene::setVisibilityFlags (unsigned int flags)
{
    for (std::vector<Ogre::Entity*>::iterator iter (mEntities.begin()); iter!=mEntities.end();
        ++iter)
        (*iter)->setVisibilityFlags (flags);

    for (std::vector<Ogre::ParticleSystem*>::iterator iter (mParticles.begin());
        iter!=mParticles.end(); ++iter)
        (*iter)->setVisibilityFlags (flags);

    for (std::vector<Ogre::Light*>::iterator iter (mLights.begin()); iter!=mLights.end();
        ++iter)
        (*iter)->setVisibilityFlags (flags);
}

void ObjectScene::rotateBillboardNodes(Ogre::Camera *camera)
{
    for (std::vector<Ogre::Node*>::iterator it = mBillboardNodes.begin(); it != mBillboardNodes.end(); ++it)
    {
        assert(mSkelBase);
        Ogre::Node* node = *it;
        node->_setDerivedOrientation(mSkelBase->getParentNode()->_getDerivedOrientation().Inverse() *
                                     camera->getRealOrientation());
    }
}

void ObjectScene::_notifyAttached()
{
    // convert initial particle positions to world space for world-space particle systems
    // this can't be done on creation because the particle system is not in its correct world space position yet
    for (std::vector<Ogre::ParticleSystem*>::iterator it = mParticles.begin(); it != mParticles.end(); ++it)
    {
        Ogre::ParticleSystem* psys = *it;
        if (psys->getKeepParticlesInLocalSpace())
            continue;
        Ogre::ParticleIterator pi = psys->_getIterator();
        while (!pi.end())
        {
            Ogre::Particle *p = pi.getNext();

#if OGRE_VERSION >= (1 << 16 | 10 << 8 | 0)
            Ogre::Vector3& position = p->mPosition;
            Ogre::Vector3& direction = p->mDirection;
#else
            Ogre::Vector3& position = p->position;
            Ogre::Vector3& direction = p->direction;
#endif

            position =
                (psys->getParentNode()->_getDerivedOrientation() *
                (psys->getParentNode()->_getDerivedScale() * position))
                + psys->getParentNode()->_getDerivedPosition();
            direction =
                (psys->getParentNode()->_getDerivedOrientation() * direction);
        }
    }
}

// Animates a texture
class FlipController
{
public:
    class Value : public Ogre::ControllerValue<Ogre::Real>
    {
    private:
        Ogre::MovableObject* mMovable;
        int mTexSlot;
        float mDelta;
        std::vector<std::string> mTextures;
        MaterialControllerManager* mMaterialControllerMgr;

    public:
        Value(Ogre::MovableObject *movable, const Nif::NiFlipController *ctrl, MaterialControllerManager* materialControllerMgr)
          : mMovable(movable)
          , mMaterialControllerMgr(materialControllerMgr)
        {
            mTexSlot = ctrl->mTexSlot;
            mDelta = ctrl->mDelta;
            for (unsigned int i=0; i<ctrl->mSources.length(); ++i)
            {
                const Nif::NiSourceTexture* tex = ctrl->mSources[i].getPtr();
                if (!tex->external)
                    std::cerr << "Warning: Found internal texture, ignoring." << std::endl;
                mTextures.push_back(Misc::ResourceHelpers::correctTexturePath(tex->filename));
            }
        }

        virtual Ogre::Real getValue() const
        {
            // Should not be called
            return 0.0f;
        }

        virtual void setValue(Ogre::Real time)
        {
            if (mDelta == 0)
                return;
            int curTexture = int(time / mDelta) % mTextures.size();

            Ogre::MaterialPtr mat = mMaterialControllerMgr->getWritableMaterial(mMovable);
            Ogre::Material::TechniqueIterator techs = mat->getTechniqueIterator();
            while(techs.hasMoreElements())
            {
                Ogre::Technique *tech = techs.getNext();
                Ogre::Technique::PassIterator passes = tech->getPassIterator();
                while(passes.hasMoreElements())
                {
                    Ogre::Pass *pass = passes.getNext();
                    Ogre::Pass::TextureUnitStateIterator textures = pass->getTextureUnitStateIterator();
                    while (textures.hasMoreElements())
                    {
                        Ogre::TextureUnitState *texture = textures.getNext();
                        if ((texture->getName() == "diffuseMap" && mTexSlot == Nif::NiTexturingProperty::BaseTexture)
                                || (texture->getName() == "normalMap" && mTexSlot == Nif::NiTexturingProperty::BumpTexture)
                                || (texture->getName() == "detailMap" && mTexSlot == Nif::NiTexturingProperty::DetailTexture)
                                || (texture->getName() == "darkMap" && mTexSlot == Nif::NiTexturingProperty::DarkTexture)
                                || (texture->getName() == "emissiveMap" && mTexSlot == Nif::NiTexturingProperty::GlowTexture))
                            texture->setTextureName(mTextures[curTexture]);
                    }
                }
            }
        }
    };

    typedef DefaultFunction Function;
};

class AlphaController
{
public:
    class Value : public Ogre::ControllerValue<Ogre::Real>, public ValueInterpolator
    {
    private:
        Ogre::MovableObject* mMovable;
        Nif::FloatKeyMap mData;
        MaterialControllerManager* mMaterialControllerMgr;

    public:
        Value(Ogre::MovableObject *movable, const Nif::NiFloatData *data, MaterialControllerManager* materialControllerMgr)
          : mMovable(movable)
          , mData(data->mKeyList)
          , mMaterialControllerMgr(materialControllerMgr)
        {
        }

        virtual Ogre::Real getValue() const
        {
            // Should not be called
            return 0.0f;
        }

        virtual void setValue(Ogre::Real time)
        {
            float value = interpKey(mData.mKeys, time);
            Ogre::MaterialPtr mat = mMaterialControllerMgr->getWritableMaterial(mMovable);
            Ogre::Material::TechniqueIterator techs = mat->getTechniqueIterator();
            while(techs.hasMoreElements())
            {
                Ogre::Technique *tech = techs.getNext();
                Ogre::Technique::PassIterator passes = tech->getPassIterator();
                while(passes.hasMoreElements())
                {
                    Ogre::Pass *pass = passes.getNext();
                    Ogre::ColourValue diffuse = pass->getDiffuse();
                    diffuse.a = value;
                    pass->setDiffuse(diffuse);
                }
            }
        }
    };

    typedef DefaultFunction Function;
};

class MaterialColorController
{
public:
    class Value : public Ogre::ControllerValue<Ogre::Real>, public ValueInterpolator
    {
    private:
        Ogre::MovableObject* mMovable;
        Nif::Vector3KeyMap mData;
        MaterialControllerManager* mMaterialControllerMgr;

    public:
        Value(Ogre::MovableObject *movable, const Nif::NiPosData *data, MaterialControllerManager* materialControllerMgr)
          : mMovable(movable)
          , mData(data->mKeyList)
          , mMaterialControllerMgr(materialControllerMgr)
        {
        }

        virtual Ogre::Real getValue() const
        {
            // Should not be called
            return 0.0f;
        }

        virtual void setValue(Ogre::Real time)
        {
            Ogre::Vector3 value = interpKey(mData.mKeys, time);
            Ogre::MaterialPtr mat = mMaterialControllerMgr->getWritableMaterial(mMovable);
            Ogre::Material::TechniqueIterator techs = mat->getTechniqueIterator();
            while(techs.hasMoreElements())
            {
                Ogre::Technique *tech = techs.getNext();
                Ogre::Technique::PassIterator passes = tech->getPassIterator();
                while(passes.hasMoreElements())
                {
                    Ogre::Pass *pass = passes.getNext();
                    Ogre::ColourValue diffuse = pass->getDiffuse();
                    diffuse.r = value.x;
                    diffuse.g = value.y;
                    diffuse.b = value.z;
                    pass->setDiffuse(diffuse);
                }
            }
        }
    };

    typedef DefaultFunction Function;
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

        static void setVisible(Ogre::Node *node, bool vis)
        {
            // Skinned meshes are attached to the scene node, not the bone.
            // We use the Node's user data to connect it with the mesh.
            Ogre::Any customData = node->getUserObjectBindings().getUserAny();

            if (!customData.isEmpty())
                Ogre::any_cast<Ogre::MovableObject*>(customData)->setVisible(vis);

            Ogre::TagPoint *tag = dynamic_cast<Ogre::TagPoint*>(node);
            if(tag != NULL)
            {
                Ogre::MovableObject *obj = tag->getChildObject();
                if(obj != NULL)
                    obj->setVisible(vis);
            }

            Ogre::Node::ChildNodeIterator iter = node->getChildIterator();
            while(iter.hasMoreElements())
            {
                node = iter.getNext();
                setVisible(node, vis);
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
    class Value : public NodeTargetValue<Ogre::Real>, public ValueInterpolator
    {
    private:
        const Nif::QuaternionKeyMap* mRotations;
        const Nif::FloatKeyMap* mXRotations;
        const Nif::FloatKeyMap* mYRotations;
        const Nif::FloatKeyMap* mZRotations;
        const Nif::Vector3KeyMap* mTranslations;
        const Nif::FloatKeyMap* mScales;
        Nif::NIFFilePtr mNif; // Hold a SharedPtr to make sure key lists stay valid

        using ValueInterpolator::interpKey;

        static Ogre::Quaternion interpKey(const Nif::QuaternionKeyMap::MapType &keys, float time)
        {
            if(time <= keys.begin()->first)
                return keys.begin()->second.mValue;

            Nif::QuaternionKeyMap::MapType::const_iterator it = keys.lower_bound(time);
            if (it != keys.end())
            {
                float aTime = it->first;
                const Nif::QuaternionKey* aKey = &it->second;

                assert (it != keys.begin()); // Shouldn't happen, was checked at beginning of this function

                Nif::QuaternionKeyMap::MapType::const_iterator last = --it;
                float aLastTime = last->first;
                const Nif::QuaternionKey* aLastKey = &last->second;

                float a = (time - aLastTime) / (aTime - aLastTime);
                return Ogre::Quaternion::nlerp(a, aLastKey->mValue, aKey->mValue);
            }
            else
                return keys.rbegin()->second.mValue;
        }

        Ogre::Quaternion getXYZRotation(float time) const
        {
            float xrot = interpKey(mXRotations->mKeys, time);
            float yrot = interpKey(mYRotations->mKeys, time);
            float zrot = interpKey(mZRotations->mKeys, time);
            Ogre::Quaternion xr(Ogre::Radian(xrot), Ogre::Vector3::UNIT_X);
            Ogre::Quaternion yr(Ogre::Radian(yrot), Ogre::Vector3::UNIT_Y);
            Ogre::Quaternion zr(Ogre::Radian(zrot), Ogre::Vector3::UNIT_Z);
            return (zr*yr*xr);
        }

    public:
        /// @note The NiKeyFrameData must be valid as long as this KeyframeController exists.
        Value(Ogre::Node *target, const Nif::NIFFilePtr& nif, const Nif::NiKeyframeData *data)
          : NodeTargetValue<Ogre::Real>(target)
          , mRotations(&data->mRotations)
          , mXRotations(&data->mXRotations)
          , mYRotations(&data->mYRotations)
          , mZRotations(&data->mZRotations)
          , mTranslations(&data->mTranslations)
          , mScales(&data->mScales)
          , mNif(nif)
        { }

        virtual Ogre::Quaternion getRotation(float time) const
        {
            if(mRotations->mKeys.size() > 0)
                return interpKey(mRotations->mKeys, time);
            else if (!mXRotations->mKeys.empty() || !mYRotations->mKeys.empty() || !mZRotations->mKeys.empty())
                return getXYZRotation(time);
            return mNode->getOrientation();
        }

        virtual Ogre::Vector3 getTranslation(float time) const
        {
            if(mTranslations->mKeys.size() > 0)
                return interpKey(mTranslations->mKeys, time);
            return mNode->getPosition();
        }

        virtual Ogre::Vector3 getScale(float time) const
        {
            if(mScales->mKeys.size() > 0)
                return Ogre::Vector3(interpKey(mScales->mKeys, time));
            return mNode->getScale();
        }

        virtual Ogre::Real getValue() const
        {
            // Should not be called
            return 0.0f;
        }

        virtual void setValue(Ogre::Real time)
        {
            if(mRotations->mKeys.size() > 0)
                mNode->setOrientation(interpKey(mRotations->mKeys, time));
            else if (!mXRotations->mKeys.empty() || !mYRotations->mKeys.empty() || !mZRotations->mKeys.empty())
                mNode->setOrientation(getXYZRotation(time));
            if(mTranslations->mKeys.size() > 0)
                mNode->setPosition(interpKey(mTranslations->mKeys, time));
            if(mScales->mKeys.size() > 0)
                mNode->setScale(Ogre::Vector3(interpKey(mScales->mKeys, time)));
        }
    };

    typedef DefaultFunction Function;
};

class UVController
{
public:
    class Value : public Ogre::ControllerValue<Ogre::Real>, public ValueInterpolator
    {
    private:
        Ogre::MovableObject* mMovable;
        Nif::FloatKeyMap mUTrans;
        Nif::FloatKeyMap mVTrans;
        Nif::FloatKeyMap mUScale;
        Nif::FloatKeyMap mVScale;
        MaterialControllerManager* mMaterialControllerMgr;

    public:
        Value(Ogre::MovableObject* movable, const Nif::NiUVData *data, MaterialControllerManager* materialControllerMgr)
          : mMovable(movable)
          , mUTrans(data->mKeyList[0])
          , mVTrans(data->mKeyList[1])
          , mUScale(data->mKeyList[2])
          , mVScale(data->mKeyList[3])
          , mMaterialControllerMgr(materialControllerMgr)
        { }

        virtual Ogre::Real getValue() const
        {
            // Should not be called
            return 1.0f;
        }

        virtual void setValue(Ogre::Real value)
        {
            float uTrans = interpKey(mUTrans.mKeys, value, 0.0f);
            float vTrans = interpKey(mVTrans.mKeys, value, 0.0f);
            float uScale = interpKey(mUScale.mKeys, value, 1.0f);
            float vScale = interpKey(mVScale.mKeys, value, 1.0f);

            Ogre::MaterialPtr material = mMaterialControllerMgr->getWritableMaterial(mMovable);

            Ogre::Material::TechniqueIterator techs = material->getTechniqueIterator();
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
    class Value : public Ogre::ControllerValue<Ogre::Real>, public ValueInterpolator
    {
    private:
        Ogre::Entity *mEntity;
        std::vector<Nif::NiMorphData::MorphData> mMorphs;
        size_t mControllerIndex;

        std::vector<Ogre::Vector3> mVertices;

    public:
        Value(Ogre::Entity *ent, const Nif::NiMorphData *data, size_t controllerIndex)
          : mEntity(ent)
          , mMorphs(data->mMorphs)
          , mControllerIndex(controllerIndex)
        {
        }

        virtual Ogre::Real getValue() const
        {
            // Should not be called
            return 0.0f;
        }

        virtual void setValue(Ogre::Real time)
        {
            if (mMorphs.size() <= 1)
                return;
            int i = 1;
            for (std::vector<Nif::NiMorphData::MorphData>::iterator it = mMorphs.begin()+1; it != mMorphs.end(); ++it,++i)
            {
                float val = 0;
                if (!it->mData.mKeys.empty())
                    val = interpKey(it->mData.mKeys, time);
                val = std::max(0.f, std::min(1.f, val));

                Ogre::String animationID = Ogre::StringConverter::toString(mControllerIndex)
                        + "_" + Ogre::StringConverter::toString(i);

                Ogre::AnimationState* state = mEntity->getAnimationState(animationID);
                state->setEnabled(val > 0);
                state->setWeight(val);
            }
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
    static bool sShowMarkers;
public:
    static void setShowMarkers(bool show)
    {
        sShowMarkers = show;
    }
private:

    static void warn(const std::string &msg)
    {
        std::cerr << "NIFObjectLoader: Warn: " << msg << std::endl;
    }

    static void createEntity(const std::string &name, const std::string &group,
                             Ogre::SceneManager *sceneMgr, ObjectScenePtr scene,
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

#if OGRE_VERSION >= (1 << 16 | 10 << 8 | 0)
        // Enable skeleton-based bounding boxes. With the static bounding box,
        // the animation may cause parts to go outside the box and cause culling problems.
        if (entity->hasSkeleton())
            entity->setUpdateBoundingBoxFromSkeleton(true);
#endif

        entity->setVisible(!(flags&Nif::NiNode::Flag_Hidden));

        scene->mEntities.push_back(entity);
        if(scene->mSkelBase)
        {
            int trgtid = NIFSkeletonLoader::lookupOgreBoneHandle(name, shape->recIndex);
            Ogre::Bone *trgtbone = scene->mSkelBase->getSkeleton()->getBone(trgtid);
            trgtbone->getUserObjectBindings().setUserAny(Ogre::Any(static_cast<Ogre::MovableObject*>(entity)));

            if(entity->hasSkeleton())
                entity->shareSkeletonInstanceWith(scene->mSkelBase);
            else
                scene->mSkelBase->attachObjectToBone(trgtbone->getName(), entity);
        }

        Nif::ControllerPtr ctrl = node->controller;
        while(!ctrl.empty())
        {
            if (ctrl->flags & Nif::NiNode::ControllerFlag_Active)
            {
                bool isAnimationAutoPlay = (animflags & Nif::NiNode::AnimFlag_AutoPlay) != 0;
                if(ctrl->recType == Nif::RC_NiUVController)
                {
                    const Nif::NiUVController *uv = static_cast<const Nif::NiUVController*>(ctrl.getPtr());

                    Ogre::ControllerValueRealPtr srcval(isAnimationAutoPlay ?
                                                        Ogre::ControllerManager::getSingleton().getFrameTimeSource() :
                                                        Ogre::ControllerValueRealPtr());
                    Ogre::ControllerValueRealPtr dstval(OGRE_NEW UVController::Value(entity, uv->data.getPtr(), &scene->mMaterialControllerMgr));

                    UVController::Function* function = OGRE_NEW UVController::Function(uv, isAnimationAutoPlay);
                    scene->mMaxControllerLength = std::max(function->mStopTime, scene->mMaxControllerLength);
                    Ogre::ControllerFunctionRealPtr func(function);

                    scene->mControllers.push_back(Ogre::Controller<Ogre::Real>(srcval, dstval, func));
                }
                else if(ctrl->recType == Nif::RC_NiGeomMorpherController)
                {
                    const Nif::NiGeomMorpherController *geom = static_cast<const Nif::NiGeomMorpherController*>(ctrl.getPtr());

                    Ogre::ControllerValueRealPtr srcval(isAnimationAutoPlay ?
                                                        Ogre::ControllerManager::getSingleton().getFrameTimeSource() :
                                                        Ogre::ControllerValueRealPtr());
                    Ogre::ControllerValueRealPtr dstval(OGRE_NEW GeomMorpherController::Value(
                        entity, geom->data.getPtr(), geom->recIndex));

                    GeomMorpherController::Function* function = OGRE_NEW GeomMorpherController::Function(geom, isAnimationAutoPlay);
                    scene->mMaxControllerLength = std::max(function->mStopTime, scene->mMaxControllerLength);
                    Ogre::ControllerFunctionRealPtr func(function);

                    scene->mControllers.push_back(Ogre::Controller<Ogre::Real>(srcval, dstval, func));
                }
            }
            ctrl = ctrl->next;
        }

        createMaterialControllers(shape, entity, animflags, scene);
    }

    static void createMaterialControllers (const Nif::Node* node, Ogre::MovableObject* movable, int animflags, ObjectScenePtr scene)
    {
        const Nif::NiTexturingProperty *texprop = NULL;
        const Nif::NiMaterialProperty *matprop = NULL;
        const Nif::NiAlphaProperty *alphaprop = NULL;
        const Nif::NiVertexColorProperty *vertprop = NULL;
        const Nif::NiZBufferProperty *zprop = NULL;
        const Nif::NiSpecularProperty *specprop = NULL;
        const Nif::NiWireframeProperty *wireprop = NULL;
        const Nif::NiStencilProperty *stencilprop = NULL;
        node->getProperties(texprop, matprop, alphaprop, vertprop, zprop, specprop, wireprop, stencilprop);

        bool isAnimationAutoPlay = (animflags & Nif::NiNode::AnimFlag_AutoPlay) != 0;
        Ogre::ControllerValueRealPtr srcval(isAnimationAutoPlay ?
                                            Ogre::ControllerManager::getSingleton().getFrameTimeSource() :
                                            Ogre::ControllerValueRealPtr());

        if(matprop)
        {
            Nif::ControllerPtr ctrls = matprop->controller;
            while(!ctrls.empty())
            {
                if (ctrls->recType == Nif::RC_NiAlphaController)
                {
                    const Nif::NiAlphaController *alphaCtrl = static_cast<const Nif::NiAlphaController*>(ctrls.getPtr());
                    Ogre::ControllerValueRealPtr dstval(OGRE_NEW AlphaController::Value(movable, alphaCtrl->data.getPtr(), &scene->mMaterialControllerMgr));
                    AlphaController::Function* function = OGRE_NEW AlphaController::Function(alphaCtrl, isAnimationAutoPlay);
                    scene->mMaxControllerLength = std::max(function->mStopTime, scene->mMaxControllerLength);
                    Ogre::ControllerFunctionRealPtr func(function);
                    scene->mControllers.push_back(Ogre::Controller<Ogre::Real>(srcval, dstval, func));
                }
                else if (ctrls->recType == Nif::RC_NiMaterialColorController)
                {
                    const Nif::NiMaterialColorController *matCtrl = static_cast<const Nif::NiMaterialColorController*>(ctrls.getPtr());
                    Ogre::ControllerValueRealPtr dstval(OGRE_NEW MaterialColorController::Value(movable, matCtrl->data.getPtr(), &scene->mMaterialControllerMgr));
                    MaterialColorController::Function* function = OGRE_NEW MaterialColorController::Function(matCtrl, isAnimationAutoPlay);
                    scene->mMaxControllerLength = std::max(function->mStopTime, scene->mMaxControllerLength);
                    Ogre::ControllerFunctionRealPtr func(function);
                    scene->mControllers.push_back(Ogre::Controller<Ogre::Real>(srcval, dstval, func));
                }

                ctrls = ctrls->next;
            }
        }
        if (texprop)
        {
            Nif::ControllerPtr ctrls = texprop->controller;
            while(!ctrls.empty())
            {
                if (ctrls->recType == Nif::RC_NiFlipController)
                {
                    const Nif::NiFlipController *flipCtrl = static_cast<const Nif::NiFlipController*>(ctrls.getPtr());


                    Ogre::ControllerValueRealPtr dstval(OGRE_NEW FlipController::Value(
                        movable, flipCtrl, &scene->mMaterialControllerMgr));
                    FlipController::Function* function = OGRE_NEW FlipController::Function(flipCtrl, isAnimationAutoPlay);
                    scene->mMaxControllerLength = std::max(function->mStopTime, scene->mMaxControllerLength);
                    Ogre::ControllerFunctionRealPtr func(function);
                    scene->mControllers.push_back(Ogre::Controller<Ogre::Real>(srcval, dstval, func));
                }

                ctrls = ctrls->next;
            }
        }
    }

    static void createParticleEmitterAffectors(Ogre::ParticleSystem *partsys,
                                               const Nif::NiParticleSystemController *partctrl, Ogre::Bone* bone,
                                               const std::string& skelBaseName)
    {
        Ogre::ParticleEmitter *emitter = partsys->addEmitter("Nif");
        emitter->setParticleVelocity(partctrl->velocity - partctrl->velocityRandom*0.5f,
                                     partctrl->velocity + partctrl->velocityRandom*0.5f);

        if (partctrl->emitFlags & Nif::NiParticleSystemController::NoAutoAdjust)
            emitter->setEmissionRate(partctrl->emitRate);
        else
            emitter->setEmissionRate(partctrl->numParticles / (partctrl->lifetime + partctrl->lifetimeRandom/2));

        emitter->setTimeToLive(std::max(0.f, partctrl->lifetime),
                               std::max(0.f, partctrl->lifetime + partctrl->lifetimeRandom));
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
                affector->setParameter("skelbase", skelBaseName);
                affector->setParameter("bone", bone->getName());
            }
            else if(e->recType == Nif::RC_NiParticleColorModifier)
            {
                const Nif::NiParticleColorModifier *cl = static_cast<const Nif::NiParticleColorModifier*>(e.getPtr());
                const Nif::NiColorData *clrdata = cl->data.getPtr();

                Ogre::ParticleAffector *affector = partsys->addAffector("ColourInterpolator");
                size_t num_colors = std::min<size_t>(6, clrdata->mKeyMap.mKeys.size());
                unsigned int i=0;
                for (Nif::Vector4KeyMap::MapType::const_iterator it = clrdata->mKeyMap.mKeys.begin(); it != clrdata->mKeyMap.mKeys.end() && i < num_colors; ++it,++i)
                {
                    Ogre::ColourValue color;
                    color.r = it->second.mValue[0];
                    color.g = it->second.mValue[1];
                    color.b = it->second.mValue[2];
                    color.a = it->second.mValue[3];
                    affector->setParameter("colour"+Ogre::StringConverter::toString(i),
                                           Ogre::StringConverter::toString(color));
                    affector->setParameter("time"+Ogre::StringConverter::toString(i),
                                           Ogre::StringConverter::toString(it->first));
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
                                     Ogre::SceneNode *sceneNode, ObjectScenePtr scene,
                                     const Nif::Node *partnode, int flags, int partflags, int animflags)
    {
        const Nif::NiAutoNormalParticlesData *particledata = NULL;
        if(partnode->recType == Nif::RC_NiAutoNormalParticles)
            particledata = static_cast<const Nif::NiAutoNormalParticles*>(partnode)->data.getPtr();
        else if(partnode->recType == Nif::RC_NiRotatingParticles)
            particledata = static_cast<const Nif::NiRotatingParticles*>(partnode)->data.getPtr();
        else
            throw std::runtime_error("Unexpected particle node type");

        std::string fullname = name+"@index="+Ogre::StringConverter::toString(partnode->recIndex);
        if(partnode->name.length() > 0)
            fullname += "@type="+partnode->name;
        Misc::StringUtils::toLower(fullname);

        Ogre::ParticleSystem *partsys = sceneNode->getCreator()->createParticleSystem();

        const Nif::NiTexturingProperty *texprop = NULL;
        const Nif::NiMaterialProperty *matprop = NULL;
        const Nif::NiAlphaProperty *alphaprop = NULL;
        const Nif::NiVertexColorProperty *vertprop = NULL;
        const Nif::NiZBufferProperty *zprop = NULL;
        const Nif::NiSpecularProperty *specprop = NULL;
        const Nif::NiWireframeProperty *wireprop = NULL;
        const Nif::NiStencilProperty *stencilprop = NULL;
        bool needTangents = false;

        partnode->getProperties(texprop, matprop, alphaprop, vertprop, zprop, specprop, wireprop, stencilprop);
        partsys->setMaterialName(NIFMaterialLoader::getMaterial(particledata, fullname, group,
                                                                texprop, matprop, alphaprop,
                                                                vertprop, zprop, specprop,
                                                                wireprop, stencilprop, needTangents,
                                                                // MW doesn't light particles, but the MaterialProperty
                                                                // used still has lighting, so that must be ignored.
                                                                true));

        partsys->setCullIndividually(false);
        partsys->setParticleQuota(particledata->numParticles);
        partsys->setKeepParticlesInLocalSpace((partflags & Nif::NiNode::ParticleFlag_LocalSpace) != 0);

        int trgtid = NIFSkeletonLoader::lookupOgreBoneHandle(name, partnode->recIndex);
        Ogre::Bone *trgtbone = scene->mSkelBase->getSkeleton()->getBone(trgtid);
        scene->mSkelBase->attachObjectToBone(trgtbone->getName(), partsys);

        Nif::ControllerPtr ctrl = partnode->controller;
        while(!ctrl.empty())
        {
            if((ctrl->recType == Nif::RC_NiParticleSystemController || ctrl->recType == Nif::RC_NiBSPArrayController)
                    && ctrl->flags & Nif::NiNode::ControllerFlag_Active)
            {
                const Nif::NiParticleSystemController *partctrl = static_cast<const Nif::NiParticleSystemController*>(ctrl.getPtr());

                float size = partctrl->size*2;
                // HACK: don't allow zero-sized particles which can rarely cause an AABB assertion in Ogre to fail
                size = std::max(size, 0.00001f);
                partsys->setDefaultDimensions(size, size);

                if(!partctrl->emitter.empty())
                {
                    int trgtid = NIFSkeletonLoader::lookupOgreBoneHandle(name, partctrl->emitter->recIndex);
                    Ogre::Bone *trgtbone = scene->mSkelBase->getSkeleton()->getBone(trgtid);
                    // Set the emitter bone(s) as user data on the particle system
                    // so the emitters/affectors can access it easily.
                    std::vector<Ogre::Bone*> bones;
                    if (partctrl->recType == Nif::RC_NiBSPArrayController)
                    {
                        std::vector<const Nif::NiNode*> nodes;
                        getAllNiNodes(partctrl->emitter.getPtr(), nodes);
                        if (nodes.empty())
                            throw std::runtime_error("Emitter for NiBSPArrayController must be a NiNode");
                        for (unsigned int i=0; i<nodes.size(); ++i)
                        {
                            bones.push_back(scene->mSkelBase->getSkeleton()->getBone(
                                                NIFSkeletonLoader::lookupOgreBoneHandle(name, nodes[i]->recIndex)));
                        }
                    }
                    else
                    {
                        bones.push_back(trgtbone);
                    }
                    NiNodeHolder holder;
                    holder.mBones = bones;
                    partsys->getUserObjectBindings().setUserAny(Ogre::Any(holder));
                    createParticleEmitterAffectors(partsys, partctrl, trgtbone, scene->mSkelBase->getName());
                }

                createParticleInitialState(partsys, particledata, partctrl);

                bool isParticleAutoPlay = (partflags&Nif::NiNode::ParticleFlag_AutoPlay) != 0;
                Ogre::ControllerValueRealPtr srcval(isParticleAutoPlay ?
                                                    Ogre::ControllerManager::getSingleton().getFrameTimeSource() :
                                                    Ogre::ControllerValueRealPtr());
                Ogre::ControllerValueRealPtr dstval(OGRE_NEW ParticleSystemController::Value(partsys, partctrl));

                ParticleSystemController::Function* function =
                        OGRE_NEW ParticleSystemController::Function(partctrl, isParticleAutoPlay);
                scene->mMaxControllerLength = std::max(function->mStopTime, scene->mMaxControllerLength);
                Ogre::ControllerFunctionRealPtr func(function);

                scene->mControllers.push_back(Ogre::Controller<Ogre::Real>(srcval, dstval, func));

                // Emitting state will be overwritten on frame update by the ParticleSystemController,
                // but set up an initial value anyway so the user can fast-forward particle systems
                // immediately after creation if desired.
                partsys->setEmitting(isParticleAutoPlay);
            }
            ctrl = ctrl->next;
        }

        partsys->setVisible(!(flags&Nif::NiNode::Flag_Hidden));
        scene->mParticles.push_back(partsys);

        createMaterialControllers(partnode, partsys, animflags, scene);
    }

    static void createParticleInitialState(Ogre::ParticleSystem* partsys, const Nif::NiAutoNormalParticlesData* particledata,
                                           const Nif::NiParticleSystemController* partctrl)
    {
        partsys->_update(0.f); // seems to be required to allocate mFreeParticles. TODO: patch Ogre to handle this better
        int i=0;
        for (std::vector<Nif::NiParticleSystemController::Particle>::const_iterator it = partctrl->particles.begin();
             i<particledata->activeCount && it != partctrl->particles.end(); ++it, ++i)
        {
            const Nif::NiParticleSystemController::Particle& particle = *it;

            Ogre::Particle* created = partsys->createParticle();
            if (!created)
                break;

#if OGRE_VERSION >= (1 << 16 | 10 << 8 | 0)
            Ogre::Vector3& position = created->mPosition;
            Ogre::Vector3& direction = created->mDirection;
            Ogre::ColourValue& colour = created->mColour;
            float& totalTimeToLive = created->mTotalTimeToLive;
            float& timeToLive = created->mTimeToLive;
#else
            Ogre::Vector3& position = created->position;
            Ogre::Vector3& direction = created->direction;
            Ogre::ColourValue& colour = created->colour;
            float& totalTimeToLive = created->totalTimeToLive;
            float& timeToLive = created->timeToLive;
#endif

            direction = particle.velocity;
            position = particledata->vertices.at(particle.vertex);

            if (particle.vertex < int(particledata->colors.size()))
            {
                Ogre::Vector4 partcolour = particledata->colors.at(particle.vertex);
                colour = Ogre::ColourValue(partcolour.x, partcolour.y, partcolour.z, partcolour.w);
            }
            else
                colour = Ogre::ColourValue(1.f, 1.f, 1.f, 1.f);
            float size = particledata->sizes.at(particle.vertex);
            created->setDimensions(size, size);
            totalTimeToLive = std::max(0.f, particle.lifespan);
            timeToLive = std::max(0.f, particle.lifespan - particle.lifetime);
        }
        partsys->_update(0.f); // now apparently needs another update, otherwise it won't render in the first frame. TODO: patch Ogre to handle this better
    }

    static void createNodeControllers(const Nif::NIFFilePtr& nif, const std::string &name, Nif::ControllerPtr ctrl, ObjectScenePtr scene, int animflags)
    {
        do {
            if (ctrl->flags & Nif::NiNode::ControllerFlag_Active)
            {
                bool isAnimationAutoPlay = (animflags & Nif::NiNode::AnimFlag_AutoPlay) != 0;
                if(ctrl->recType == Nif::RC_NiVisController)
                {
                    const Nif::NiVisController *vis = static_cast<const Nif::NiVisController*>(ctrl.getPtr());

                    int trgtid = NIFSkeletonLoader::lookupOgreBoneHandle(name, ctrl->target->recIndex);
                    Ogre::Bone *trgtbone = scene->mSkelBase->getSkeleton()->getBone(trgtid);
                    Ogre::ControllerValueRealPtr srcval(isAnimationAutoPlay ?
                                                        Ogre::ControllerManager::getSingleton().getFrameTimeSource() :
                                                        Ogre::ControllerValueRealPtr());
                    Ogre::ControllerValueRealPtr dstval(OGRE_NEW VisController::Value(trgtbone, vis->data.getPtr()));

                    VisController::Function* function = OGRE_NEW VisController::Function(vis, isAnimationAutoPlay);
                    scene->mMaxControllerLength = std::max(function->mStopTime, scene->mMaxControllerLength);
                    Ogre::ControllerFunctionRealPtr func(function);

                    scene->mControllers.push_back(Ogre::Controller<Ogre::Real>(srcval, dstval, func));
                }
                else if(ctrl->recType == Nif::RC_NiKeyframeController)
                {
                    const Nif::NiKeyframeController *key = static_cast<const Nif::NiKeyframeController*>(ctrl.getPtr());
                    if(!key->data.empty())
                    {
                        int trgtid = NIFSkeletonLoader::lookupOgreBoneHandle(name, ctrl->target->recIndex);
                        Ogre::Bone *trgtbone = scene->mSkelBase->getSkeleton()->getBone(trgtid);
                        // The keyframe controller will control this bone manually
                        trgtbone->setManuallyControlled(true);
                        Ogre::ControllerValueRealPtr srcval(isAnimationAutoPlay ?
                                                            Ogre::ControllerManager::getSingleton().getFrameTimeSource() :
                                                            Ogre::ControllerValueRealPtr());
                        Ogre::ControllerValueRealPtr dstval(OGRE_NEW KeyframeController::Value(trgtbone, nif, key->data.getPtr()));
                        KeyframeController::Function* function = OGRE_NEW KeyframeController::Function(key, isAnimationAutoPlay);
                        scene->mMaxControllerLength = std::max(function->mStopTime, scene->mMaxControllerLength);
                        Ogre::ControllerFunctionRealPtr func(function);

                        scene->mControllers.push_back(Ogre::Controller<Ogre::Real>(srcval, dstval, func));
                    }
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
                if(nextpos != std::string::npos)
                {
                    do {
                        nextpos--;
                    } while(nextpos > pos && ::isspace(str[nextpos]));
                    nextpos++;
                }
                else if(::isspace(*str.rbegin()))
                {
                    std::string::const_iterator last = str.end();
                    do {
                        --last;
                    } while(last != str.begin() && ::isspace(*last));
                    nextpos = std::distance(str.begin(), ++last);
                }
                std::string result = str.substr(pos, nextpos-pos);
                textkeys.insert(std::make_pair(tk->list[i].time, Misc::StringUtils::toLower(result)));

                pos = nextpos;
            }
        }
    }


    static void createObjects(const Nif::NIFFilePtr& nif, const std::string &name, const std::string &group,
                              Ogre::SceneNode *sceneNode, const Nif::Node *node,
                              ObjectScenePtr scene, int flags, int animflags, int partflags, bool isRootCollisionNode=false)
    {
        // Do not create objects for the collision shape (includes all children)
        if(node->recType == Nif::RC_RootCollisionNode)
            isRootCollisionNode = true;

        if(node->recType == Nif::RC_NiBSAnimationNode)
            animflags |= node->flags;
        else if(node->recType == Nif::RC_NiBSParticleNode)
            partflags |= node->flags;
        else
            flags |= node->flags;

        if (node->recType == Nif::RC_NiBillboardNode)
        {
            // TODO: figure out what the flags mean.
            // NifSkope has names for them, but doesn't implement them.
            // Change mBillboardNodes to map <Bone, billboard type>
            int trgtid = NIFSkeletonLoader::lookupOgreBoneHandle(name, node->recIndex);
            Ogre::Bone* bone = scene->mSkelBase->getSkeleton()->getBone(trgtid);
            bone->setManuallyControlled(true);
            scene->mBillboardNodes.push_back(bone);
        }

        Nif::ExtraPtr e = node->extra;
        while(!e.empty())
        {
            if(e->recType == Nif::RC_NiTextKeyExtraData)
            {
                const Nif::NiTextKeyExtraData *tk = static_cast<const Nif::NiTextKeyExtraData*>(e.getPtr());

                extractTextKeys(tk, scene->mTextKeys);
            }
            else if(e->recType == Nif::RC_NiStringExtraData)
            {
                const Nif::NiStringExtraData *sd = static_cast<const Nif::NiStringExtraData*>(e.getPtr());
                // String markers may contain important information
                // affecting the entire subtree of this obj
                if(sd->string == "MRK" && !sShowMarkers)
                {
                    // Marker objects. These meshes are only visible in the
                    // editor.
                    flags |= 0x80000000;
                }
            }

            e = e->extra;
        }

        if(!node->controller.empty())
            createNodeControllers(nif, name, node->controller, scene, animflags);

        if (!isRootCollisionNode)
        {
            if(node->recType == Nif::RC_NiCamera)
            {
                /* Ignored */
            }

            if(node->recType == Nif::RC_NiTriShape && !(flags&0x80000000))
            {
                createEntity(name, group, sceneNode->getCreator(), scene, node, flags, animflags);
            }

            if((node->recType == Nif::RC_NiAutoNormalParticles ||
                node->recType == Nif::RC_NiRotatingParticles) && !(flags&0x40000000))
            {
                createParticleSystem(name, group, sceneNode, scene, node, flags, partflags, animflags);
            }
        }

        const Nif::NiNode *ninode = dynamic_cast<const Nif::NiNode*>(node);
        if(ninode)
        {
            const Nif::NodeList &children = ninode->children;
            for(size_t i = 0;i < children.length();i++)
            {
                if(!children[i].empty())
                    createObjects(nif, name, group, sceneNode, children[i].getPtr(), scene, flags, animflags, partflags, isRootCollisionNode);
            }
        }
    }

    static void createSkelBase(const std::string &name, const std::string &group,
                               Ogre::SceneManager *sceneMgr, const Nif::Node *node,
                               ObjectScenePtr scene)
    {
        /* This creates an empty mesh to which a skeleton gets attached. This
         * is to ensure we have an entity with a skeleton instance, even if all
         * other entities are attached to bones and not skinned. */
        Ogre::MeshManager &meshMgr = Ogre::MeshManager::getSingleton();
        if(meshMgr.getByName(name).isNull())
            NIFMeshLoader::createMesh(name, name, group, ~(size_t)0);

        scene->mSkelBase = sceneMgr->createEntity(name);
        scene->mEntities.push_back(scene->mSkelBase);
    }

public:
    static void load(Ogre::SceneNode *sceneNode, ObjectScenePtr scene, const std::string &name, const std::string &group, int flags=0)
    {
        Nif::NIFFilePtr nif = Nif::Cache::getInstance().load(name);
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
            createSkelBase(name, group, sceneNode->getCreator(), node, scene);
        }
        createObjects(nif, name, group, sceneNode, node, scene, flags, 0, 0);
    }

    static void loadKf(Ogre::Skeleton *skel, const std::string &name,
                       TextKeyMap &textKeys, std::vector<Ogre::Controller<Ogre::Real> > &ctrls)
    {
        Nif::NIFFilePtr nif = Nif::Cache::getInstance().load(name);
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

            if (!(ctrl->flags & Nif::NiNode::ControllerFlag_Active))
                continue;

            const Nif::NiStringExtraData *strdata = static_cast<const Nif::NiStringExtraData*>(extra.getPtr());
            const Nif::NiKeyframeController *key = static_cast<const Nif::NiKeyframeController*>(ctrl.getPtr());

            if(key->data.empty())
                continue;
            if(!skel->hasBone(strdata->string))
                continue;

            Ogre::Bone *trgtbone = skel->getBone(strdata->string);
            Ogre::ControllerValueRealPtr srcval;
            Ogre::ControllerValueRealPtr dstval(OGRE_NEW KeyframeController::Value(trgtbone, nif, key->data.getPtr()));
            Ogre::ControllerFunctionRealPtr func(OGRE_NEW KeyframeController::Function(key, false));

            ctrls.push_back(Ogre::Controller<Ogre::Real>(srcval, dstval, func));
        }
    }
};


ObjectScenePtr Loader::createObjects(Ogre::SceneNode *parentNode, std::string name, const std::string &group)
{
    ObjectScenePtr scene = ObjectScenePtr (new ObjectScene(parentNode->getCreator()));

    Misc::StringUtils::toLower(name);
    NIFObjectLoader::load(parentNode, scene, name, group);

    for(size_t i = 0;i < scene->mEntities.size();i++)
    {
        Ogre::Entity *entity = scene->mEntities[i];
        if(!entity->isAttached())
            parentNode->attachObject(entity);
    }

    scene->_notifyAttached();

    return scene;
}

ObjectScenePtr Loader::createObjects(Ogre::Entity *parent, const std::string &bonename,
                                     const std::string& bonefilter,
                                 Ogre::SceneNode *parentNode,
                                 std::string name, const std::string &group)
{
    ObjectScenePtr scene = ObjectScenePtr (new ObjectScene(parentNode->getCreator()));

    Misc::StringUtils::toLower(name);
    NIFObjectLoader::load(parentNode, scene, name, group);

    bool isskinned = false;
    for(size_t i = 0;i < scene->mEntities.size();i++)
    {
        Ogre::Entity *ent = scene->mEntities[i];
        if(scene->mSkelBase != ent && ent->hasSkeleton())
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
        // accepts anything named "filter*" or "tri filter*"
        std::string filter = "@shape=tri "+bonefilter;
        std::string filter2 = "@shape="+bonefilter;
        Misc::StringUtils::toLower(filter);
        Misc::StringUtils::toLower(filter2);
        for(size_t i = 0;i < scene->mEntities.size();i++)
        {
            Ogre::Entity *entity = scene->mEntities[i];
            if(entity->hasSkeleton())
            {
                if(entity == scene->mSkelBase ||
                   entity->getMesh()->getName().find(filter) != std::string::npos
                   || entity->getMesh()->getName().find(filter2) != std::string::npos)
                    parentNode->attachObject(entity);
            }
            else
            {
                if(entity->getMesh()->getName().find(filter) == std::string::npos
                        || entity->getMesh()->getName().find(filter2) == std::string::npos)
                    entity->detachFromParent();
            }
        }
    }
    else
    {
        for(size_t i = 0;i < scene->mEntities.size();i++)
        {
            Ogre::Entity *entity = scene->mEntities[i];
            if(!entity->isAttached())
            {
                Ogre::TagPoint *tag = parent->attachObjectToBone(bonename, entity);
                tag->setScale(scale);
            }
        }
    }

    scene->_notifyAttached();

    return scene;
}


ObjectScenePtr Loader::createObjectBase(Ogre::SceneNode *parentNode, std::string name, const std::string &group)
{
    ObjectScenePtr scene = ObjectScenePtr (new ObjectScene(parentNode->getCreator()));

    Misc::StringUtils::toLower(name);
    NIFObjectLoader::load(parentNode, scene, name, group, 0xC0000000);

    if(scene->mSkelBase)
        parentNode->attachObject(scene->mSkelBase);

    return scene;
}


void Loader::createKfControllers(Ogre::Entity *skelBase,
                                 const std::string &name,
                                 TextKeyMap &textKeys,
                                 std::vector<Ogre::Controller<Ogre::Real> > &ctrls)
{
    NIFObjectLoader::loadKf(skelBase->getSkeleton(), name, textKeys, ctrls);
}

bool Loader::sShowMarkers = false;
bool NIFObjectLoader::sShowMarkers = false;

void Loader::setShowMarkers(bool show)
{
    sShowMarkers = show;
    NIFObjectLoader::setShowMarkers(show);
}


} // namespace NifOgre

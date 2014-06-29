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

#include "bulletnifloader.hpp"

#include <cstdio>


#include <components/misc/stringops.hpp>

#include "../nif/niffile.hpp"
#include "../nif/node.hpp"
#include "../nif/data.hpp"
#include "../nif/property.hpp"
#include "../nif/controller.hpp"
#include "../nif/extra.hpp"
#include <libs/platform/strings.h>

#include <vector>
#include <list>
// For warning messages
#include <iostream>

// float infinity
#include <limits>

typedef unsigned char ubyte;

namespace NifBullet
{

ManualBulletShapeLoader::~ManualBulletShapeLoader()
{
}


btVector3 ManualBulletShapeLoader::getbtVector(Ogre::Vector3 const &v)
{
    return btVector3(v[0], v[1], v[2]);
}

void ManualBulletShapeLoader::loadResource(Ogre::Resource *resource)
{
    mShape = static_cast<OEngine::Physic::BulletShape *>(resource);
    mResourceName = mShape->getName();
    mShape->mCollide = false;
    mBoundingBox = NULL;
    mShape->mBoxTranslation = Ogre::Vector3(0,0,0);
    mShape->mBoxRotation = Ogre::Quaternion::IDENTITY;
    mCompoundShape = NULL;
    mStaticMesh = NULL;

    // Load the NIF. TODO: Wrap this in a try-catch block once we're out
    // of the early stages of development. Right now we WANT to catch
    // every error as early and intrusively as possible, as it's most
    // likely a sign of incomplete code rather than faulty input.
    Nif::NIFFile::ptr pnif (Nif::NIFFile::create (mResourceName.substr(0, mResourceName.length()-7)));
    Nif::NIFFile & nif = *pnif.get ();
    if (nif.numRoots() < 1)
    {
        warn("Found no root nodes in NIF.");
        return;
    }

    Nif::Record *r = nif.getRoot(0);
    assert(r != NULL);

    Nif::Node *node = dynamic_cast<Nif::Node*>(r);
    if (node == NULL)
    {
        warn("First root in file was not a node, but a " +
             r->recName + ". Skipping file.");
        return;
    }

    mShape->mHasCollisionNode = hasRootCollisionNode(node);

    //do a first pass
    handleNode(node,0,false,false,false);

    if(mBoundingBox != NULL)
    {
       mShape->mCollisionShape = mBoundingBox;
       delete mStaticMesh;
       if (mCompoundShape)
       {
           int n = mCompoundShape->getNumChildShapes();
           for(int i=0; i <n;i++)
               delete (mCompoundShape->getChildShape(i));
           delete mCompoundShape;
           mShape->mAnimatedShapes.clear();
       }
    }
    else
    {
        if (mCompoundShape)
        {
            mShape->mCollisionShape = mCompoundShape;
            if (mStaticMesh)
            {
                btTransform trans;
                trans.setIdentity();
                mCompoundShape->addChildShape(trans, new TriangleMeshShape(mStaticMesh,true));
            }
        }
        else if (mStaticMesh)
            mShape->mCollisionShape = new TriangleMeshShape(mStaticMesh,true);
    }

    //second pass which create a shape for raycasting.
    mResourceName = mShape->getName();
    mShape->mCollide = false;
    mBoundingBox = NULL;
    mShape->mBoxTranslation = Ogre::Vector3(0,0,0);
    mShape->mBoxRotation = Ogre::Quaternion::IDENTITY;
    mStaticMesh = NULL;
    mCompoundShape = NULL;

    handleNode(node,0,true,true,false);

    if (mCompoundShape)
    {
        mShape->mRaycastingShape = mCompoundShape;
        if (mStaticMesh)
        {
            btTransform trans;
            trans.setIdentity();
            mCompoundShape->addChildShape(trans, new TriangleMeshShape(mStaticMesh,true));
        }
    }
    else if (mStaticMesh)
        mShape->mRaycastingShape = new TriangleMeshShape(mStaticMesh,true);
}

bool ManualBulletShapeLoader::hasRootCollisionNode(Nif::Node const * node)
{
    if(node->recType == Nif::RC_RootCollisionNode)
        return true;

    const Nif::NiNode *ninode = dynamic_cast<const Nif::NiNode*>(node);
    if(ninode)
    {
        const Nif::NodeList &list = ninode->children;
        for(size_t i = 0;i < list.length();i++)
        {
            if(!list[i].empty())
            {
                if(hasRootCollisionNode(list[i].getPtr()))
                    return true;
            }
        }
    }

    return false;
}

void ManualBulletShapeLoader::handleNode(const Nif::Node *node, int flags,
                                         bool isCollisionNode,
                                         bool raycasting, bool isMarker, bool isAnimated)
{
    // Accumulate the flags from all the child nodes. This works for all
    // the flags we currently use, at least.
    flags |= node->flags;

    if (!node->controller.empty() && node->controller->recType == Nif::RC_NiKeyframeController
            && (node->controller->flags & Nif::NiNode::ControllerFlag_Active))
        isAnimated = true;

    if (!raycasting)
        isCollisionNode = isCollisionNode || (node->recType == Nif::RC_RootCollisionNode);
    else
        isCollisionNode = isCollisionNode && (node->recType != Nif::RC_RootCollisionNode);

    // Don't collide with AvoidNode shapes
    if(node->recType == Nif::RC_AvoidNode)
        flags |= 0x800;

    // Marker objects
    /// \todo don't do this in the editor
    std::string nodename = node->name;
    Misc::StringUtils::toLower(nodename);
    if (nodename.find("marker") != std::string::npos)
        isMarker = true;

    // Check for extra data
    Nif::Extra const *e = node;
    while (!e->extra.empty())
    {
        // Get the next extra data in the list
        e = e->extra.getPtr();
        assert(e != NULL);

        if (e->recType == Nif::RC_NiStringExtraData)
        {
            // String markers may contain important information
            // affecting the entire subtree of this node
            Nif::NiStringExtraData *sd = (Nif::NiStringExtraData*)e;

            // not sure what the difference between NCO and NCC is, or if there even is one
            if (sd->string == "NCO" || sd->string == "NCC")
            {
                // No collision. Use an internal flag setting to mark this.
                flags |= 0x800;
            }
            else if (sd->string == "MRK")
                // Marker objects. These are only visible in the
                // editor. Until and unless we add an editor component to
                // the engine, just skip this entire node.
                isMarker = true;
        }
    }

    if ( (isCollisionNode || (!mShape->mHasCollisionNode && !raycasting))
            && (!isMarker || (mShape->mHasCollisionNode && !raycasting)))
    {
        // NOTE: a trishape with hasBounds=true, but no BBoxCollision flag should NOT go through handleNiTriShape!
        // It must be ignored completely.
        // (occurs in tr_ex_imp_wall_arch_04.nif)
        if(node->hasBounds)
        {
            if (flags & Nif::NiNode::Flag_BBoxCollision && !raycasting)
            {
                mShape->mBoxTranslation = node->boundPos;
                mShape->mBoxRotation = node->boundRot;
                mBoundingBox = new btBoxShape(getbtVector(node->boundXYZ));
            }
        }
        else if(node->recType == Nif::RC_NiTriShape)
        {
            mShape->mCollide = !(flags&0x800);
            handleNiTriShape(static_cast<const Nif::NiTriShape*>(node), flags, node->getWorldTransform(), raycasting, isAnimated);
        }
    }

    // For NiNodes, loop through children
    const Nif::NiNode *ninode = dynamic_cast<const Nif::NiNode*>(node);
    if(ninode)
    {
        const Nif::NodeList &list = ninode->children;
        for(size_t i = 0;i < list.length();i++)
        {
            if(!list[i].empty())
                handleNode(list[i].getPtr(), flags, isCollisionNode, raycasting, isMarker, isAnimated);
        }
    }
}

void ManualBulletShapeLoader::handleNiTriShape(const Nif::NiTriShape *shape, int flags, const Ogre::Matrix4 &transform,
                                               bool raycasting, bool isAnimated)
{
    assert(shape != NULL);

    // Interpret flags
    bool hidden    = (flags&Nif::NiNode::Flag_Hidden) != 0;
    bool collide   = (flags&Nif::NiNode::Flag_MeshCollision) != 0;
    bool bbcollide = (flags&Nif::NiNode::Flag_BBoxCollision) != 0;

    // If the object was marked "NCO" earlier, it shouldn't collide with
    // anything. So don't do anything.
    if ((flags & 0x800) && !raycasting)
    {
        collide = false;
        bbcollide = false;
        return;
    }

    if (!collide && !bbcollide && hidden && !raycasting)
        // This mesh apparently isn't being used for anything, so don't
        // bother setting it up.
        return;

    if (!shape->skin.empty())
        isAnimated = false;

    if (isAnimated)
    {
        if (!mCompoundShape)
            mCompoundShape = new btCompoundShape();

        btTriangleMesh* childMesh = new btTriangleMesh();

        const Nif::NiTriShapeData *data = shape->data.getPtr();

        childMesh->preallocateVertices(data->vertices.size());
        childMesh->preallocateIndices(data->triangles.size());

        const std::vector<Ogre::Vector3> &vertices = data->vertices;
        const std::vector<short> &triangles = data->triangles;

        for(size_t i = 0;i < data->triangles.size();i+=3)
        {
            Ogre::Vector3 b1 = vertices[triangles[i+0]];
            Ogre::Vector3 b2 = vertices[triangles[i+1]];
            Ogre::Vector3 b3 = vertices[triangles[i+2]];
            childMesh->addTriangle(btVector3(b1.x,b1.y,b1.z),btVector3(b2.x,b2.y,b2.z),btVector3(b3.x,b3.y,b3.z));
        }

        TriangleMeshShape* childShape = new TriangleMeshShape(childMesh,true);

        childShape->setLocalScaling(btVector3(transform[0][0], transform[1][1], transform[2][2]));

        Ogre::Quaternion q = transform.extractQuaternion();
        Ogre::Vector3 v = transform.getTrans();
        btTransform trans(btQuaternion(q.x, q.y, q.z, q.w), btVector3(v.x, v.y, v.z));

        if (raycasting)
            mShape->mAnimatedRaycastingShapes.insert(std::make_pair(shape->name, mCompoundShape->getNumChildShapes()));
        else
            mShape->mAnimatedShapes.insert(std::make_pair(shape->name, mCompoundShape->getNumChildShapes()));

        mCompoundShape->addChildShape(trans, childShape);
    }
    else
    {
        if (!mStaticMesh)
            mStaticMesh = new btTriangleMesh();

        // Static shape, just transform all vertices into position
        const Nif::NiTriShapeData *data = shape->data.getPtr();
        const std::vector<Ogre::Vector3> &vertices = data->vertices;
        const std::vector<short> &triangles = data->triangles;

        for(size_t i = 0;i < data->triangles.size();i+=3)
        {
            Ogre::Vector3 b1 = transform*vertices[triangles[i+0]];
            Ogre::Vector3 b2 = transform*vertices[triangles[i+1]];
            Ogre::Vector3 b3 = transform*vertices[triangles[i+2]];
            mStaticMesh->addTriangle(btVector3(b1.x,b1.y,b1.z),btVector3(b2.x,b2.y,b2.z),btVector3(b3.x,b3.y,b3.z));
        }
    }
}

void ManualBulletShapeLoader::load(const std::string &name,const std::string &group)
{
    // Check if the resource already exists
    Ogre::ResourcePtr ptr = OEngine::Physic::BulletShapeManager::getSingleton().getByName(name, group);
    if (!ptr.isNull())
        return;
    OEngine::Physic::BulletShapeManager::getSingleton().create(name,group,true,this);
}

bool findBoundingBox (const Nif::Node* node, Ogre::Vector3& halfExtents, Ogre::Vector3& translation, Ogre::Quaternion& orientation)
{
    if(node->hasBounds)
    {
        if (!(node->flags & Nif::NiNode::Flag_Hidden))
        {
            translation = node->boundPos;
            orientation = node->boundRot;
            halfExtents = node->boundXYZ;
            return true;
        }
    }

    const Nif::NiNode *ninode = dynamic_cast<const Nif::NiNode*>(node);
    if(ninode)
    {
        const Nif::NodeList &list = ninode->children;
        for(size_t i = 0;i < list.length();i++)
        {
            if(!list[i].empty())
                if (findBoundingBox(list[i].getPtr(), halfExtents, translation, orientation))
                    return true;
        }
    }
    return false;
}

bool getBoundingBox(const std::string& nifFile, Ogre::Vector3& halfExtents, Ogre::Vector3& translation, Ogre::Quaternion& orientation)
{
    Nif::NIFFile::ptr pnif (Nif::NIFFile::create (nifFile));
    Nif::NIFFile & nif = *pnif.get ();

    if (nif.numRoots() < 1)
    {
        return false;
    }

    Nif::Record *r = nif.getRoot(0);
    assert(r != NULL);

    Nif::Node *node = dynamic_cast<Nif::Node*>(r);
    if (node == NULL)
    {
        return false;
    }

    return findBoundingBox(node, halfExtents, translation, orientation);
}

} // namespace NifBullet

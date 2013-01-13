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

#include "bullet_nif_loader.hpp"
#include <Ogre.h>
#include <cstdio>

#include "../nif/nif_file.hpp"
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

using namespace NifBullet;


ManualBulletShapeLoader::~ManualBulletShapeLoader()
{
}


btQuaternion ManualBulletShapeLoader::getbtQuat(Ogre::Matrix3 const &m)
{
    Ogre::Quaternion oquat(m);
    btQuaternion quat;
    quat.setW(oquat.w);
    quat.setX(oquat.x);
    quat.setY(oquat.y);
    quat.setZ(oquat.z);
    return quat;
}

btVector3 ManualBulletShapeLoader::getbtVector(Ogre::Vector3 const &v)
{
    return btVector3(v[0], v[1], v[2]);
}

void ManualBulletShapeLoader::loadResource(Ogre::Resource *resource)
{
    cShape = static_cast<OEngine::Physic::BulletShape *>(resource);
    resourceName = cShape->getName();
    cShape->mCollide = false;
    mBoundingBox = NULL;
    cShape->boxTranslation = Ogre::Vector3(0,0,0);
    cShape->boxRotation = Ogre::Quaternion::IDENTITY;

    mTriMesh = new btTriangleMesh();

    // Load the NIF. TODO: Wrap this in a try-catch block once we're out
    // of the early stages of development. Right now we WANT to catch
    // every error as early and intrusively as possible, as it's most
    // likely a sign of incomplete code rather than faulty input.
    Nif::NIFFile::ptr pnif (Nif::NIFFile::create (resourceName.substr(0, resourceName.length()-7)));
    Nif::NIFFile & nif = *pnif.get ();
    if (nif.numRecords() < 1)
    {
        warn("Found no records in NIF.");
        return;
    }


    // The first record is assumed to be the root node
    Nif::Record *r = nif.getRecord(0);
    assert(r != NULL);

    Nif::Node *node = dynamic_cast<Nif::Node*>(r);
    if (node == NULL)
    {
        warn("First record in file was not a node, but a " +
                r->recName + ". Skipping file.");
        return;
    }

    bool hasCollisionNode = hasRootCollisionNode(node);

    //do a first pass
    handleNode(node,0,NULL,hasCollisionNode,false,false);

    //if collide = false, then it does a second pass which create a shape for raycasting.
    if(cShape->mCollide == false)
    {
        handleNode(node,0,NULL,hasCollisionNode,false,true);
    }

    //cShape->collide = hasCollisionNode&&cShape->collide;

    struct TriangleMeshShape : public btBvhTriangleMeshShape
    {
        TriangleMeshShape(btStridingMeshInterface* meshInterface, bool useQuantizedAabbCompression)
            : btBvhTriangleMeshShape(meshInterface, useQuantizedAabbCompression)
        {
        }

        virtual ~TriangleMeshShape()
        {
            delete getTriangleInfoMap();
            delete m_meshInterface;
        }
    };
    if(mBoundingBox != NULL)
       cShape->Shape = mBoundingBox;

    else
    {
        currentShape = new TriangleMeshShape(mTriMesh,true);
        cShape->Shape = currentShape;
    }
}

bool ManualBulletShapeLoader::hasRootCollisionNode(Nif::Node const * node)
{
    if (node->recType == Nif::RC_NiNode)
    {
        Nif::NodeList &list = ((Nif::NiNode*)node)->children;
        int n = list.length();
        for (int i=0; i<n; i++)
        {
            if (!list[i].empty())
            {
                if(hasRootCollisionNode(list[i].getPtr())) return true;;
            }
        }
    }
    else if (node->recType == Nif::RC_NiTriShape)
    {
        return false;
    }
    else if(node->recType == Nif::RC_RootCollisionNode)
    {
        return true;
    }

    return false;
}

void ManualBulletShapeLoader::handleNode(Nif::Node const *node, int flags,
   const Nif::Transformation *parentTrafo,bool hasCollisionNode,bool isCollisionNode,bool raycastingOnly)
{

    // Accumulate the flags from all the child nodes. This works for all
    // the flags we currently use, at least.
    flags |= node->flags;

    // Marker objects: no collision
    /// \todo don't do this in the editor
    if (node->name.find("marker") != std::string::npos)
    {
        flags |= 0x800;
        cShape->mIgnore = true;
    }

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
            else if (sd->string == "MRK" && !raycastingOnly)
                // Marker objects. These are only visible in the
                // editor. Until and unless we add an editor component to
                // the engine, just skip this entire node.
                return;
        }
    }

    Nif::Transformation childTrafo = node->trafo;

    if (parentTrafo)
    {

        // Get a non-const reference to the node's data, since we're
        // overwriting it. TODO: Is this necessary?

        // For both position and rotation we have that:
        // final_vector = old_vector + old_rotation*new_vector*old_scale
        childTrafo.pos = parentTrafo->pos + parentTrafo->rotation*childTrafo.pos*parentTrafo->scale;

        // Merge the rotations together
        childTrafo.rotation = parentTrafo->rotation * childTrafo.rotation;

        // Scale
        childTrafo.scale *= parentTrafo->scale;

    }

    if(node->hasBounds)
    {

        
        btVector3 boxsize = getbtVector(node->boundXYZ);
        cShape->boxTranslation = node->boundPos;
        cShape->boxRotation = node->boundRot;

        mBoundingBox = new btBoxShape(boxsize);
    }


    // For NiNodes, loop through children
    if (node->recType == Nif::RC_NiNode)
    {
        Nif::NodeList const &list = ((Nif::NiNode const *)node)->children;
        int n = list.length();
        for (int i=0; i<n; i++)
        {
            if (!list[i].empty())
            {
                handleNode(list[i].getPtr(), flags,&childTrafo,hasCollisionNode,isCollisionNode,raycastingOnly);
            }
        }
    }
    else if (node->recType == Nif::RC_NiTriShape && (isCollisionNode || !hasCollisionNode))
    {
        cShape->mCollide = !(flags&0x800);
        handleNiTriShape(dynamic_cast<Nif::NiTriShape const *>(node), flags,childTrafo.rotation,childTrafo.pos,childTrafo.scale,raycastingOnly);
    }
    else if(node->recType == Nif::RC_RootCollisionNode)
    {
        Nif::NodeList &list = ((Nif::NiNode*)node)->children;
        int n = list.length();
        for (int i=0; i<n; i++)
        {
            if (!list[i].empty())
                handleNode(list[i].getPtr(), flags,&childTrafo, hasCollisionNode,true,raycastingOnly);
        }
    }
}

void ManualBulletShapeLoader::handleNiTriShape(Nif::NiTriShape const *shape, int flags,Ogre::Matrix3 parentRot,Ogre::Vector3 parentPos,float parentScale,
    bool raycastingOnly)
{
    assert(shape != NULL);

    // Interpret flags
    bool hidden    = (flags & 0x01) != 0; // Not displayed
    bool collide   = (flags & 0x02) != 0; // Use mesh for collision
    bool bbcollide = (flags & 0x04) != 0; // Use bounding box for collision

    // If the object was marked "NCO" earlier, it shouldn't collide with
    // anything. So don't do anything.
    if (flags & 0x800 && !raycastingOnly)
    {
        collide = false;
        bbcollide = false;
        return;
    }

    if (!collide && !bbcollide && hidden && !raycastingOnly)
        // This mesh apparently isn't being used for anything, so don't
        // bother setting it up.
        return;


    Nif::NiTriShapeData *data = shape->data.getPtr();

    const std::vector<Ogre::Vector3> &vertices = data->vertices;
    const Ogre::Matrix3 &rot = shape->trafo.rotation;
    const Ogre::Vector3 &pos = shape->trafo.pos;
    float scale = shape->trafo.scale * parentScale;
    short* triangles = &data->triangles[0];
    for(size_t i = 0;i < data->triangles.size();i+=3)
    {
        Ogre::Vector3 b1 = pos + rot*vertices[triangles[i+0]]*scale;
        Ogre::Vector3 b2 = pos + rot*vertices[triangles[i+1]]*scale;
        Ogre::Vector3 b3 = pos + rot*vertices[triangles[i+2]]*scale;
        mTriMesh->addTriangle(btVector3(b1.x,b1.y,b1.z),btVector3(b2.x,b2.y,b2.z),btVector3(b3.x,b3.y,b3.z));
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

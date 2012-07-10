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
#include <stdio.h>

#include <libs/mangle/vfs/servers/ogre_vfs.hpp>
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

using namespace std;
using namespace Ogre;
using namespace Nif;
using namespace Mangle::VFS;

using namespace NifBullet;

ManualBulletShapeLoader::~ManualBulletShapeLoader()
{
    delete vfs;
}

Ogre::Matrix3 ManualBulletShapeLoader::getMatrix(Nif::Transformation* tr)
{
    Ogre::Matrix3 rot(tr->rotation.v[0].array[0],tr->rotation.v[0].array[1],tr->rotation.v[0].array[2],
            tr->rotation.v[1].array[0],tr->rotation.v[1].array[1],tr->rotation.v[1].array[2],
            tr->rotation.v[2].array[0],tr->rotation.v[2].array[1],tr->rotation.v[2].array[2]);
    return rot;
}
Ogre::Vector3 ManualBulletShapeLoader::getVector(Nif::Transformation* tr)
{
    Ogre::Vector3 vect3(tr->pos.array[0],tr->pos.array[1],tr->pos.array[2]);
    return vect3;
}

btQuaternion ManualBulletShapeLoader::getbtQuat(Ogre::Matrix3 m)
{
    Ogre::Quaternion oquat(m);
    btQuaternion quat;
    quat.setW(oquat.w);
    quat.setX(oquat.x);
    quat.setY(oquat.y);
    quat.setZ(oquat.z);
    return quat;
}

btVector3 ManualBulletShapeLoader::getbtVector(Nif::Vector v)
{
    btVector3 a(v.array[0],v.array[1],v.array[2]);
    return a;
}

void ManualBulletShapeLoader::loadResource(Ogre::Resource *resource)
{
    cShape = static_cast<BulletShape *>(resource);
    resourceName = cShape->getName();
    cShape->collide = false;

    mTriMesh = new btTriangleMesh();

    if (!vfs) vfs = new OgreVFS(resourceGroup);

    if (!vfs->isFile(resourceName))
    {
        warn("File not found.");
        return;
    }

    // Load the NIF. TODO: Wrap this in a try-catch block once we're out
    // of the early stages of development. Right now we WANT to catch
    // every error as early and intrusively as possible, as it's most
    // likely a sign of incomplete code rather than faulty input.
    Nif::NIFFile nif(vfs->open(resourceName), resourceName);

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
    handleNode(node,0,Ogre::Matrix3::IDENTITY,Ogre::Vector3::ZERO,1,hasCollisionNode,false,false);

    //if collide = false, then it does a second pass which create a shape for raycasting.
    if(cShape->collide == false)
    {
        handleNode(node,0,Ogre::Matrix3::IDENTITY,Ogre::Vector3::ZERO,1,hasCollisionNode,false,true);
    }

    cShape->collide = hasCollisionNode&&cShape->collide;

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

    currentShape = new TriangleMeshShape(mTriMesh,true);
    cShape->Shape = currentShape;
}

bool ManualBulletShapeLoader::hasRootCollisionNode(Nif::Node* node)
{
    if (node->recType == Nif::RC_NiNode)
    {
        Nif::NodeList &list = ((Nif::NiNode*)node)->children;
        int n = list.length();
        for (int i=0; i<n; i++)
        {
            if (list.has(i))
            {
                if(hasRootCollisionNode(&list[i])) return true;;
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

void ManualBulletShapeLoader::handleNode(Nif::Node *node, int flags,
    Ogre::Matrix3 parentRot,Ogre::Vector3 parentPos,float parentScale,bool hasCollisionNode,bool isCollisionNode,bool raycastingOnly)
{
    // Accumulate the flags from all the child nodes. This works for all
    // the flags we currently use, at least.
    flags |= node->flags;

    // Check for extra data
    Nif::Extra *e = node;
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

            if (sd->string == "NCO" && !raycastingOnly)
            {
                // No collision. Use an internal flag setting to mark this.
                // We ignor this node!
                flags |= 0x800;
                return;
            }
            else if (sd->string == "MRK" && !raycastingOnly)
                // Marker objects. These are only visible in the
                // editor. Until and unless we add an editor component to
                // the engine, just skip this entire node.
                return;
        }
    }

    //transfo of parents node + curent node
    Ogre::Matrix3 finalRot;
    Ogre::Vector3 finalPos;
    float finalScale;

    Nif::Transformation &final = *((Nif::Transformation*)node->trafo);
    Ogre::Vector3 nodePos = getVector(&final);
    Ogre::Matrix3 nodeRot = getMatrix(&final);

    finalPos = nodePos + parentPos;
    finalRot = parentRot*nodeRot;
    finalScale = final.scale*parentScale;


    // For NiNodes, loop through children
    if (node->recType == Nif::RC_NiNode)
    {
        Nif::NodeList &list = ((Nif::NiNode*)node)->children;
        int n = list.length();
        for (int i=0; i<n; i++)
        {
            if (list.has(i))
            {
                handleNode(&list[i], flags,finalRot,finalPos,finalScale,hasCollisionNode,isCollisionNode,raycastingOnly);
            }
        }
    }
    else if (node->recType == Nif::RC_NiTriShape && (isCollisionNode || !hasCollisionNode)) 
    {
        cShape->collide = true;
        handleNiTriShape(dynamic_cast<Nif::NiTriShape*>(node), flags,finalRot,finalPos,parentScale,raycastingOnly);
    }
    else if(node->recType == Nif::RC_RootCollisionNode)
    {
        Nif::NodeList &list = ((Nif::NiNode*)node)->children;
        int n = list.length();
        for (int i=0; i<n; i++)
        {
            if (list.has(i))
                handleNode(&list[i], flags,finalRot,finalPos,finalScale,hasCollisionNode,true,raycastingOnly);
        }
    }
}

void ManualBulletShapeLoader::handleNiTriShape(Nif::NiTriShape *shape, int flags,Ogre::Matrix3 parentRot,Ogre::Vector3 parentPos,float parentScale,
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

    float* vertices = (float*)&data->vertices[0];
    unsigned short* triangles = (unsigned short*)&data->triangles[0];

    for(unsigned int i=0; i < data->triangles.size(); i = i+3)
    {
        Ogre::Vector3 b1(vertices[triangles[i+0]*3]*parentScale,vertices[triangles[i+0]*3+1]*parentScale,vertices[triangles[i+0]*3+2]*parentScale);
        Ogre::Vector3 b2(vertices[triangles[i+1]*3]*parentScale,vertices[triangles[i+1]*3+1]*parentScale,vertices[triangles[i+1]*3+2]*parentScale);
        Ogre::Vector3 b3(vertices[triangles[i+2]*3]*parentScale,vertices[triangles[i+2]*3+1]*parentScale,vertices[triangles[i+2]*3+2]*parentScale);
        b1 = parentRot * b1 + parentPos;
        b2 = parentRot * b2 + parentPos;
        b3 = parentRot * b3 + parentPos;
        mTriMesh->addTriangle(btVector3(b1.x,b1.y,b1.z),btVector3(b2.x,b2.y,b2.z),btVector3(b3.x,b3.y,b3.z));
    }
}

void ManualBulletShapeLoader::load(const std::string &name,const std::string &group)
{
    // Check if the resource already exists
    Ogre::ResourcePtr ptr = BulletShapeManager::getSingleton().getByName(name, group);
    if (!ptr.isNull())
        return;
    BulletShapeManager::getSingleton().create(name,group,true,this);
}

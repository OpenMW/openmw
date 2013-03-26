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

ManualBulletShapeLoader::~ManualBulletShapeLoader()
{
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
    cShape->mBoxTranslation = Ogre::Vector3(0,0,0);
    cShape->mBoxRotation = Ogre::Quaternion::IDENTITY;
    mHasShape = false;

    btTriangleMesh* mesh1 = new btTriangleMesh();

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

    cShape->mHasCollisionNode = hasRootCollisionNode(node);

    //do a first pass
    handleNode(mesh1, node,0,false,false,false);

    if(mBoundingBox != NULL)
    {
       cShape->mCollisionShape = mBoundingBox;
       delete mesh1;
    }
    else if (mHasShape && cShape->mCollide)
    {
        cShape->mCollisionShape = new TriangleMeshShape(mesh1,true);
    }
    else
        delete mesh1;

    //second pass which create a shape for raycasting.
    resourceName = cShape->getName();
    cShape->mCollide = false;
    mBoundingBox = NULL;
    cShape->mBoxTranslation = Ogre::Vector3(0,0,0);
    cShape->mBoxRotation = Ogre::Quaternion::IDENTITY;
    mHasShape = false;

    btTriangleMesh* mesh2 = new btTriangleMesh();

    handleNode(mesh2, node,0,true,true,false);

    if(mBoundingBox != NULL)
    {
       cShape->mRaycastingShape = mBoundingBox;
       delete mesh2;
    }
    else if (mHasShape)
    {
        cShape->mRaycastingShape = new TriangleMeshShape(mesh2,true);
    }
    else
        delete mesh2;
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

void ManualBulletShapeLoader::handleNode(btTriangleMesh* mesh, const Nif::Node *node, int flags,
                                         bool isCollisionNode,
                                         bool raycasting, bool isMarker)
{
    // Accumulate the flags from all the child nodes. This works for all
    // the flags we currently use, at least.
    flags |= node->flags;

    if (!raycasting)
        isCollisionNode = isCollisionNode || (node->recType == Nif::RC_RootCollisionNode);
    else
        isCollisionNode = isCollisionNode && (node->recType != Nif::RC_RootCollisionNode);

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

    if ( (isCollisionNode || (!cShape->mHasCollisionNode && !raycasting))
            && (!isMarker || (cShape->mHasCollisionNode && !raycasting)))
    {
        if(node->hasBounds)
        {
            cShape->mBoxTranslation = node->boundPos;
            cShape->mBoxRotation = node->boundRot;
            mBoundingBox = new btBoxShape(getbtVector(node->boundXYZ));
        }
        else if(node->recType == Nif::RC_NiTriShape)
        {
            cShape->mCollide = !(flags&0x800);
            handleNiTriShape(mesh, static_cast<const Nif::NiTriShape*>(node), flags, node->getWorldTransform(), raycasting);
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
                handleNode(mesh, list[i].getPtr(), flags, isCollisionNode, raycasting, isMarker);
        }
    }
}

void ManualBulletShapeLoader::handleNiTriShape(btTriangleMesh* mesh, const Nif::NiTriShape *shape, int flags, const Ogre::Matrix4 &transform,
                                               bool raycasting)
{
    assert(shape != NULL);

    // Interpret flags
    bool hidden    = (flags & 0x01) != 0; // Not displayed
    bool collide   = (flags & 0x02) != 0; // Use mesh for collision
    bool bbcollide = (flags & 0x04) != 0; // Use bounding box for collision

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

    mHasShape = true;

    const Nif::NiTriShapeData *data = shape->data.getPtr();
    const std::vector<Ogre::Vector3> &vertices = data->vertices;
    const short *triangles = &data->triangles[0];
    for(size_t i = 0;i < data->triangles.size();i+=3)
    {
        Ogre::Vector3 b1 = transform*vertices[triangles[i+0]];
        Ogre::Vector3 b2 = transform*vertices[triangles[i+1]];
        Ogre::Vector3 b3 = transform*vertices[triangles[i+2]];
        mesh->addTriangle(btVector3(b1.x,b1.y,b1.z),btVector3(b2.x,b2.y,b2.z),btVector3(b3.x,b3.y,b3.z));
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

} // namespace NifBullet

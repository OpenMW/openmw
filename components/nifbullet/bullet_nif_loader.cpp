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


BulletShape::BulletShape(Ogre::ResourceManager* creator, const Ogre::String &name, 
	Ogre::ResourceHandle handle, const Ogre::String &group, bool isManual, 
	Ogre::ManualResourceLoader *loader) :
Ogre::Resource(creator, name, handle, group, isManual, loader)
{
	/* If you were storing a pointer to an object, then you would set that pointer to NULL here.
	*/

	/* For consistency with StringInterface, but we don't add any parameters here
	That's because the Resource implementation of StringInterface is to
	list all the options that need to be set before loading, of which 
	we have none as such. Full details can be set through scripts.
	*/ 
	Shape = NULL;
	createParamDictionary("BulletShape");
}

BulletShape::~BulletShape()
{
}

// farm out to BulletShapeLoader
void BulletShape::loadImpl()
{
	mLoader->loadResource(this);
}

void BulletShape::deleteShape(btCollisionShape* mShape)
{
	if(mShape!=NULL)
	{
		if(mShape->isCompound())
		{
			btCompoundShape* ms = static_cast<btCompoundShape*>(Shape);
			btCompoundShapeChild* child = ms->getChildList();
			int a = ms->getNumChildShapes();
			for(int i=0; i <a;i++)
			{
				deleteShape(ms->getChildShape(i));
			}
		}
			delete mShape;
	}
	mShape = NULL;
}

void BulletShape::unloadImpl()
{
	deleteShape(Shape);
}

//TODO:change this?
size_t BulletShape::calculateSize() const
{
	return 1;
}


//=============================================================================================================
template<> BulletShapeManager *Ogre::Singleton<BulletShapeManager>::ms_Singleton = 0;

BulletShapeManager *BulletShapeManager::getSingletonPtr()
{
	return ms_Singleton;
}

BulletShapeManager &BulletShapeManager::getSingleton()
{  
	assert(ms_Singleton);  
	return(*ms_Singleton);
}

BulletShapeManager::BulletShapeManager()
{
	mResourceType = "BulletShape";

	// low, because it will likely reference other resources
	mLoadOrder = 30.0f;

	// this is how we register the ResourceManager with OGRE
	Ogre::ResourceGroupManager::getSingleton()._registerResourceManager(mResourceType, this);
}

BulletShapeManager::~BulletShapeManager()
{
	// and this is how we unregister it
	Ogre::ResourceGroupManager::getSingleton()._unregisterResourceManager(mResourceType);
}

BulletShapePtr BulletShapeManager::load(const Ogre::String &name, const Ogre::String &group)
{
	BulletShapePtr textf = getByName(name);

	if (textf.isNull())
		textf = create(name, group);

	textf->load();
	return textf;
}

Ogre::Resource *BulletShapeManager::createImpl(const Ogre::String &name, Ogre::ResourceHandle handle, 
	const Ogre::String &group, bool isManual, Ogre::ManualResourceLoader *loader, 
	const Ogre::NameValuePairList *createParams)
{
	BulletShape* res = new BulletShape(this, name, handle, group, isManual, loader);
	//if(isManual)
	//{
		//loader->loadResource(res);
	//}
	return res;
}

//====================================================================================================
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

	currentShape = new btCompoundShape();
	cShape->Shape = currentShape;

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
			r->recName.toString() + ". Skipping file.");
		return;
	}

	handleNode(node,0,Ogre::Matrix3::IDENTITY,Ogre::Vector3::ZERO,1,false);
}

void ManualBulletShapeLoader::handleNode(Nif::Node *node, int flags,
	Ogre::Matrix3 parentRot,Ogre::Vector3 parentPos,float parentScale,bool isCollisionNode)
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

			if (sd->string == "NCO")
			{
				// No collision. Use an internal flag setting to mark this.
				// We ignor this node!
				flags |= 0x800;
				return;
			}
			else if (sd->string == "MRK")
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
				handleNode(&list[i], flags,finalRot,finalPos,finalScale,isCollisionNode);
			}
		}
	}
	else if (node->recType == Nif::RC_NiTriShape && isCollisionNode) handleNiTriShape(dynamic_cast<Nif::NiTriShape*>(node), flags,finalRot,finalPos,finalScale);
	else if(node->recType == Nif::RC_RootCollisionNode)
	{
		Nif::NodeList &list = ((Nif::NiNode*)node)->children;
		int n = list.length();
		for (int i=0; i<n; i++)
		{
			if (list.has(i))
				handleNode(&list[i], flags,finalRot,finalPos,finalScale,true);
		}
	}
}

void ManualBulletShapeLoader::handleNiTriShape(Nif::NiTriShape *shape, int flags,Ogre::Matrix3 parentRot,Ogre::Vector3 parentPos,float parentScale)
{
	assert(shape != NULL);
	btCollisionShape* NodeShape;

	// Interpret flags
	bool hidden    = (flags & 0x01) != 0; // Not displayed
	bool collide   = (flags & 0x02) != 0; // Use mesh for collision
	bool bbcollide = (flags & 0x04) != 0; // Use bounding box for collision

	// If the object was marked "NCO" earlier, it shouldn't collide with
	// anything. So don't do anything.
	if (flags & 0x800)
	{
		collide = false;
		bbcollide = false;
		return;
	}

	if (!collide && !bbcollide && hidden)
		// This mesh apparently isn't being used for anything, so don't
		// bother setting it up.
		return;

	btTransform tr;
	tr.setRotation(getbtQuat(parentRot));
	tr.setOrigin(btVector3(parentPos.x,parentPos.y,parentPos.z));

	// Bounding box collision isn't implemented, always use mesh for now.
	/*if (bbcollide)
	{
		return;
		std::cout << "bbcolide?";
		//TODO: check whether it's half box or not (is there a /2?)
		NodeShape = new btBoxShape(btVector3(shape->boundXYZ->array[0]/2.,shape->boundXYZ->array[1]/2.,shape->boundXYZ->array[2]/2.));
		std::cout << "bbcolide12121212121";
		currentShape->addChildShape(tr,NodeShape);
		std::cout << "aaaaaaaaaaaaa";
		return;
		collide = true;
		bbcollide = false;
	}*/

	/* Do in-place transformation.the only needed transfo is the scale. (maybe not in fact)
	*/
	btTriangleMesh *mTriMesh = new btTriangleMesh();

	Nif::NiTriShapeData *data = shape->data.getPtr();
	int numVerts = data->vertices.length / 3;

	float* vertices = (float*)data->vertices.ptr;
	unsigned short* triangles = (unsigned short*)data->triangles.ptr;

	for(int i=0; i < data->triangles.length; i = i+3)
	{
		btVector3 b1(vertices[triangles[i+0]*3]*parentScale,vertices[triangles[i+0]*3+1]*parentScale,vertices[triangles[i+0]*3+2]*parentScale);
		btVector3 b2(vertices[triangles[i+1]*3]*parentScale,vertices[triangles[i+1]*3+1]*parentScale,vertices[triangles[i+1]*3+2]*parentScale);
		btVector3 b3(vertices[triangles[i+2]*3]*parentScale,vertices[triangles[i+2]*3+1]*parentScale,vertices[triangles[i+2]*3+2]*parentScale);
		mTriMesh->addTriangle(b1,b2,b3);
	}
	NodeShape = new btBvhTriangleMeshShape(mTriMesh,true);
	currentShape->addChildShape(tr,NodeShape);
}

void ManualBulletShapeLoader::load(const std::string &name,const std::string &group)
{
	// Check if the resource already exists
	Ogre::ResourcePtr ptr = BulletShapeManager::getSingleton().getByName(name, group);
	if (!ptr.isNull())
		return;
	BulletShapeManager::getSingleton().create(name,group,true,this);
}
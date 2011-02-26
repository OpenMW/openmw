/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008-2010  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: http://openmw.sourceforge.net/

  This file (ogre_nif_loader.h) is part of the OpenMW package.

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

#ifndef _BULLET_NIF_LOADER_H_
#define _BULLET_NIF_LOADER_H_

#include <OgreResource.h>
#include <OgreResourceManager.h>
#include <OgreMesh.h>
#include <assert.h>
#include <string>
#include <BulletCollision/CollisionShapes/btBvhTriangleMeshShape.h>
#include <BulletCollision/CollisionShapes/btConvexTriangleMeshShape.h>
#include <btBulletDynamicsCommon.h>
#include <btBulletCollisionCommon.h>


#include <vector>
#include <list>
// For warning messages
#include <iostream>

// float infinity
#include <limits>

namespace Nif
{
    class Node;
    class Transformation;
    class NiTriShape;
    class Vector;
    class Matrix;
}

namespace Mangle
{
	namespace VFS
	{
		class OgreVFS;
	}
}


/**
*Define a new resource which describe a Shape usable by bullet.See BulletShapeManager for how to get/use them.
*/
class BulletShape : public Ogre::Resource
{
	Ogre::String mString;

protected:
	void loadImpl();
	void unloadImpl();
	size_t calculateSize() const;

	void deleteShape(btCollisionShape* mShape);

public:

	BulletShape(Ogre::ResourceManager *creator, const Ogre::String &name,
		Ogre::ResourceHandle handle, const Ogre::String &group, bool isManual = false,
		Ogre::ManualResourceLoader *loader = 0);

	virtual ~BulletShape();

	btCollisionShape* Shape;
    //this flag indicate if the shape is used for collision or if it's for raycasting only.
    bool collide;
};

/**
*
*/
class BulletShapePtr : public Ogre::SharedPtr<BulletShape>
{
public:
	BulletShapePtr() : Ogre::SharedPtr<BulletShape>() {}
	explicit BulletShapePtr(BulletShape *rep) : Ogre::SharedPtr<BulletShape>(rep) {}
	BulletShapePtr(const BulletShapePtr &r) : Ogre::SharedPtr<BulletShape>(r) {}
	BulletShapePtr(const Ogre::ResourcePtr &r) : Ogre::SharedPtr<BulletShape>()
	{
		if( r.isNull() )
			return;
		// lock & copy other mutex pointer
		OGRE_LOCK_MUTEX(*r.OGRE_AUTO_MUTEX_NAME)
			OGRE_COPY_AUTO_SHARED_MUTEX(r.OGRE_AUTO_MUTEX_NAME)
			pRep = static_cast<BulletShape*>(r.getPointer());
		pUseCount = r.useCountPointer();
		useFreeMethod = r.freeMethod();
		if (pUseCount)
		{
			++(*pUseCount);
		}
	}

	/// Operator used to convert a ResourcePtr to a BulletShapePtr
	BulletShapePtr& operator=(const Ogre::ResourcePtr& r)
	{
		if(pRep == static_cast<BulletShape*>(r.getPointer()))
			return *this;
		release();
		if( r.isNull() )
			return *this; // resource ptr is null, so the call to release above has done all we need to do.
		// lock & copy other mutex pointer
		OGRE_LOCK_MUTEX(*r.OGRE_AUTO_MUTEX_NAME)
			OGRE_COPY_AUTO_SHARED_MUTEX(r.OGRE_AUTO_MUTEX_NAME)
			pRep = static_cast<BulletShape*>(r.getPointer());
		pUseCount = r.useCountPointer();
		useFreeMethod = r.freeMethod();
		if (pUseCount)
		{
			++(*pUseCount);
		}
		return *this;
	}
};

/**
*Hold any BulletShape that was created by the ManualBulletShapeLoader.
*
*To get a bulletShape, you must load it first.
*First, create a manualBulletShapeLoader. Then call ManualBulletShapeManager->load(). This create an "empty" resource.
*Then use BulletShapeManager->load(). This will fill the resource with the required info.
*To get the resource,use BulletShapeManager::getByName.
*When you use the resource no more, just use BulletShapeManager->unload(). It won't completly delete the resource, but it will
*"empty" it.This allow a better management of memory: when you are leaving a cell, just unload every useless shape.
*
*Alternatively, you can call BulletShape->load() in order to actually load the resource.
*When you are finished with it, just call BulletShape->unload().
*
*IMO: prefere the first methode, i am not completly sure about the 2nd.
*
*Important Note: i have no idea of what happen if you try to load two time the same resource without unloading.
*It won't crash, but it might lead to memory leaks(I don't know how Ogre handle this). So don't do it!
*/
class BulletShapeManager : public Ogre::ResourceManager, public Ogre::Singleton<BulletShapeManager>
{
protected:

	// must implement this from ResourceManager's interface
	Ogre::Resource *createImpl(const Ogre::String &name, Ogre::ResourceHandle handle,
		const Ogre::String &group, bool isManual, Ogre::ManualResourceLoader *loader,
		const Ogre::NameValuePairList *createParams);

public:

	BulletShapeManager();
	virtual ~BulletShapeManager();

	virtual BulletShapePtr load(const Ogre::String &name, const Ogre::String &group);

	static BulletShapeManager &getSingleton();
	static BulletShapeManager *getSingletonPtr();
};

/**
*Load bulletShape from NIF files.
*/
class ManualBulletShapeLoader : public Ogre::ManualResourceLoader
{
public:

	ManualBulletShapeLoader():resourceGroup("General"){vfs = 0;}
	virtual ~ManualBulletShapeLoader() {}

	void warn(std::string msg)
	{
		std::cerr << "NIFLoader: Warn:" << msg << "\n";
	}

	void fail(std::string msg)
	{
		std::cerr << "NIFLoader: Fail: "<< msg << std::endl;
		assert(1);
	}

	/**
	*This function should not be called manualy. Use load instead. (this is called by the BulletShapeManager when you use load).
	*/
	void loadResource(Ogre::Resource *resource);

	/**
	*This function load a new bulletShape from a NIF file into the BulletShapeManager.
	*When the file is loaded, you can then use BulletShapeManager::getByName() to retrive the bulletShape.
	*Warning: this function will just crash if the resourceGroup doesn't exist!
	*/
	void load(const std::string &name,const std::string &group);

private:
	Ogre::Matrix3 getMatrix(Nif::Transformation* tr);

	Ogre::Vector3 getVector(Nif::Transformation* tr);

	btQuaternion getbtQuat(Ogre::Matrix3 m);

	btVector3 getbtVector(Nif::Vector v);

	/**
	*Parse a node.
	*/
	void handleNode(Nif::Node *node, int flags,
		Ogre::Matrix3 parentRot,Ogre::Vector3 parentPos,float parentScale,bool isCollisionNode,bool raycastingOnly);

	/**
	*convert a NiTriShape to a bullet trishape.
	*/
	void handleNiTriShape(Nif::NiTriShape *shape, int flags,Ogre::Matrix3 parentRot,Ogre::Vector3 parentPos,float parentScales,bool raycastingOnly);

	Mangle::VFS::OgreVFS *vfs;

	std::string resourceName;
	std::string resourceGroup;

	BulletShape* cShape;//current shape
	btCompoundShape* currentShape;//the shape curently under construction
};

#endif

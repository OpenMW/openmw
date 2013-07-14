#ifndef _BULLET_SHAPE_LOADER_H_
#define _BULLET_SHAPE_LOADER_H_

#include <OgreResource.h>
#include <OgreResourceManager.h>
#include <btBulletCollisionCommon.h>
#include <OgreVector3.h>

namespace OEngine {
namespace Physic
{

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

    void deleteShape(btCollisionShape* shape);

public:

    BulletShape(Ogre::ResourceManager *creator, const Ogre::String &name,
        Ogre::ResourceHandle handle, const Ogre::String &group, bool isManual = false,
        Ogre::ManualResourceLoader *loader = 0);

    virtual ~BulletShape();

    btCollisionShape* mCollisionShape;
    btCollisionShape* mRaycastingShape;

    // Whether or not a NiRootCollisionNode was present in the .nif. If there is none, the collision behaviour
    // depends on object type, so we need to expose this variable.
    bool mHasCollisionNode;

    Ogre::Vector3 mBoxTranslation;
    Ogre::Quaternion mBoxRotation;
    //this flag indicate if the shape is used for collision or if it's for raycasting only.
    bool mCollide;
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
class BulletShapeManager : public Ogre::ResourceManager
{
protected:

    // must implement this from ResourceManager's interface
    Ogre::Resource *createImpl(const Ogre::String &name, Ogre::ResourceHandle handle,
        const Ogre::String &group, bool isManual, Ogre::ManualResourceLoader *loader,
        const Ogre::NameValuePairList *createParams);

    static BulletShapeManager *sThis;

private:
    /** \brief Explicit private copy constructor. This is a forbidden operation.*/
    BulletShapeManager(const BulletShapeManager &);

    /** \brief Private operator= . This is a forbidden operation. */
    BulletShapeManager& operator=(const BulletShapeManager &);


public:

    BulletShapeManager();
    virtual ~BulletShapeManager();

    virtual BulletShapePtr load(const Ogre::String &name, const Ogre::String &group);

    static BulletShapeManager &getSingleton();
    static BulletShapeManager *getSingletonPtr();
};

class BulletShapeLoader : public Ogre::ManualResourceLoader
{
public:

    BulletShapeLoader(){};
    virtual ~BulletShapeLoader() {}

    virtual void loadResource(Ogre::Resource *resource);

    virtual void load(const std::string &name,const std::string &group);
};

}
}

#endif

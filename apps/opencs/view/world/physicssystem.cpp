#include "physicssystem.hpp"

#include <iostream> // FIXME: debug only

#include <OgreRay.h>
#include <OgreCamera.h>
#include <OgreManualObject.h> // FIXME: debug cursor position
#include <OgreEntity.h>              // FIXME: visual highlight, clone
#include <OgreMaterialManager.h>     // FIXME: visual highlight, material
#include <OgreHardwarePixelBuffer.h> // FIXME: visual highlight, texture
#include <OgreRoot.h> // FIXME: renderOneFrame

#include <openengine/bullet/physic.hpp>
#include <components/nifbullet/bulletnifloader.hpp>
#include "../../model/settings/usersettings.hpp"

namespace
{
    void showHitPoint(Ogre::SceneManager *sceneMgr, std::string name, Ogre::Vector3 point)
    {
        sceneMgr->destroyManualObject("manual" + name);
        Ogre::ManualObject* manual = sceneMgr->createManualObject("manual" + name);
        manual->begin("BaseWhite", Ogre::RenderOperation::OT_LINE_LIST);
        manual-> position(point.x,     point.y,     point.z-100);
        manual-> position(point.x,     point.y,     point.z+100);
        manual-> position(point.x,     point.y-100, point.z);
        manual-> position(point.x,     point.y+100, point.z);
        manual-> position(point.x-100, point.y,     point.z);
        manual-> position(point.x+100, point.y,     point.z);
        manual->end();
        sceneMgr->getRootSceneNode()->createChildSceneNode()->attachObject(manual);
    }

    void removeHitPoint(Ogre::SceneManager *sceneMgr, std::string name)
    {
        sceneMgr->destroyManualObject("manual" + name);
    }
}

namespace CSVWorld
{
    PhysicsSystem::PhysicsSystem(Ogre::SceneManager *sceneMgr) : mSceneMgr(sceneMgr)
    {
        // Create physics. shapeLoader is deleted by the physic engine
        NifBullet::ManualBulletShapeLoader* shapeLoader = new NifBullet::ManualBulletShapeLoader();
        mEngine = new OEngine::Physic::PhysicEngine(shapeLoader);

        mEngine->setSceneManager(sceneMgr); // needed for toggleDebugRendering()

        // material for visual cue on selected objects
        Ogre::TexturePtr texture = Ogre::TextureManager::getSingleton().getByName("DynamicTrans");
        if(texture.isNull())
        {
            texture = Ogre::TextureManager::getSingleton().createManual(
                "DynamicTrans", // name
                Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
                Ogre::TEX_TYPE_2D,  // type
                8, 8,               // width & height
                0,                  // number of mipmaps
                Ogre::PF_BYTE_BGRA, // pixel format
                Ogre::TU_DEFAULT);  // usage; should be TU_DYNAMIC_WRITE_ONLY_DISCARDABLE for
                                    // textures updated very often (e.g. each frame)

            Ogre::HardwarePixelBufferSharedPtr pixelBuffer = texture->getBuffer();
            pixelBuffer->lock(Ogre::HardwareBuffer::HBL_NORMAL);
            const Ogre::PixelBox& pixelBox = pixelBuffer->getCurrentLock();

            uint8_t* pDest = static_cast<uint8_t*>(pixelBox.data);

            // Fill in some pixel data. This will give a semi-transparent colour,
            // but this is of course dependent on the chosen pixel format.
            for (size_t j = 0; j < 8; j++)
            {
                for(size_t i = 0; i < 8; i++)
                {
                    *pDest++ = 255; // B
                    *pDest++ = 255; // G
                    *pDest++ = 127; // R
                    *pDest++ =  63; // A
                }

                pDest += pixelBox.getRowSkip() * Ogre::PixelUtil::getNumElemBytes(pixelBox.format);
            }
            pixelBuffer->unlock();
        }
        Ogre::MaterialPtr material = Ogre::MaterialManager::getSingleton().getByName(
                    "TransMaterial");
        if(material.isNull())
        {
            Ogre::MaterialPtr material = Ogre::MaterialManager::getSingleton().create(
                        "TransMaterial",
                        Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, true );
            Ogre::Pass *pass = material->getTechnique( 0 )->getPass( 0 );
            pass->setLightingEnabled( false );
            pass->setDepthWriteEnabled( false );
            pass->setSceneBlending( Ogre::SBT_TRANSPARENT_ALPHA );

            Ogre::TextureUnitState *tex = pass->createTextureUnitState("CustomState", 0);
            tex->setTextureName("DynamicTrans");
            tex->setTextureFiltering( Ogre::TFO_ANISOTROPIC );
            material->load();
        }
    }

    PhysicsSystem::~PhysicsSystem()
    {
        delete mEngine;
    }

    void PhysicsSystem::addObject(const std::string &mesh,
                                  const std::string &name,
                                  float scale,
                                  const Ogre::Vector3 &position,
                                  const Ogre::Quaternion &rotation,
                                  bool placeable)
    {
        //mHandleToMesh[name] = mesh;

        mEngine->createAndAdjustRigidBody(mesh, name, scale, position, rotation,
                                          0,    // scaledBoxTranslation
                                          0,    // boxRotation
                                          true, // raycasting
                                          placeable);
    }

    void PhysicsSystem::removeObject(const std::string& name)
    {
        mEngine->removeRigidBody(name);
        mEngine->deleteRigidBody(name);
    }

    void PhysicsSystem::toggleDebugRendering()
    {
        mEngine->toggleDebugRendering();
        mEngine->stepSimulation(0.0167); // FIXME: DebugDrawer::step() not accessible
        Ogre::Root::getSingleton().renderOneFrame(); // FIXME: temporary workaround for immediate visual feedback
    }

    std::pair<bool, Ogre::Vector3> PhysicsSystem::castRay(float mouseX, float mouseY,
                                        Ogre::Vector3* normal, std::string* hit, Ogre::Camera *camera)
    {
        // using a really small value seems to mess up with the projections
        float nearClipDistance = camera->getNearClipDistance();
        camera->setNearClipDistance(10.0f);  // arbitrary number
        Ogre::Ray ray = camera->getCameraToViewportRay(mouseX, mouseY);
        camera->setNearClipDistance(nearClipDistance);

        Ogre::Vector3 from = ray.getOrigin();
        CSMSettings::UserSettings &userSettings = CSMSettings::UserSettings::instance();
        float farClipDist = userSettings.setting("Scene/far clip distance", QString("300000")).toFloat();
        Ogre::Vector3 to = ray.getPoint(farClipDist);

        btVector3 _from, _to;
        _from = btVector3(from.x, from.y, from.z);
        _to = btVector3(to.x, to.y, to.z);

        bool raycastingObjectOnly = true;
        bool ignoreHeightMap = false;
        Ogre::Vector3 norm;
        std::pair<std::string, float> result =
                    mEngine->rayTest(_from, _to, raycastingObjectOnly, ignoreHeightMap, &norm);

        if ((result.first == "") || !mSceneMgr->hasSceneNode(result.first))
            return std::make_pair(false, Ogre::Vector3());
            //std::cout << "no hit" << std::endl;
        else
        {
            //TODO: Try http://www.ogre3d.org/tikiwiki/Create+outline+around+a+character
            Ogre::SceneNode *scene = mSceneMgr->getSceneNode(result.first);
            std::map<std::string, std::vector<std::string>>::iterator iter =
                                                    mSelectedEntities.find(result.first);
            if(iter != mSelectedEntities.end()) // currently selected
            {
                std::vector<std::string> deletedEntities = mSelectedEntities[result.first];
                while(!deletedEntities.empty())
                {
                    scene->detachObject(deletedEntities.back());
                    mSceneMgr->destroyEntity(deletedEntities.back());
                    deletedEntities.pop_back();
                }
                mSelectedEntities.erase(iter);

                removeHitPoint(mSceneMgr, result.first); // FIXME: for debugging
            }
            else
            {
                std::vector<std::string> clonedEntities;
                Ogre::SceneNode::ObjectIterator iter = scene->getAttachedObjectIterator();
                iter.begin();
                while(iter.hasMoreElements())
                {
                    Ogre::MovableObject * element = iter.getNext();
                    if(element->getMovableType() != "Entity")
                        continue;

                    Ogre::Entity * entity = dynamic_cast<Ogre::Entity *>(element);
                    if(mSceneMgr->hasEntity(entity->getName()+"cover"))
                    {
                        // FIXME: this shouldn't really happen... but does :(
                        scene->detachObject(entity->getName()+"cover");
                        mSceneMgr->destroyEntity(entity->getName()+"cover");
                    }
                    Ogre::Entity * clone = entity->clone(entity->getName()+"cover");

                    Ogre::MaterialPtr mat =
                        Ogre::MaterialManager::getSingleton().getByName("TransMaterial");
                    if(!mat.isNull())
                    {
                        clone->setMaterial(mat);
                        scene->attachObject(clone);
                        clonedEntities.push_back(entity->getName()+"cover");
                    }

                }
                mSelectedEntities[result.first] = clonedEntities;

                // FIXME: show cursor position for debugging
                showHitPoint(mSceneMgr, result.first, ray.getPoint(farClipDist*result.second));
            }
            // FIXME: temporary workaround for immediate visual feedback
            Ogre::Root::getSingleton().renderOneFrame();
#if 0
            std::cout << "hit " << result.first
                      + " result " + std::to_string(result.second*farClipDist) << std::endl;
            std::cout << "normal " + std::to_string(norm.x)
                            + ", " + std::to_string(norm.y)
                            + ", " + std::to_string(norm.z) << std::endl;
            std::cout << "hit pos "+ std::to_string(ray.getPoint(farClipDist*result.second).x)
                            + ", " + std::to_string(ray.getPoint(farClipDist*result.second).y)
                            + ", " + std::to_string(ray.getPoint(farClipDist*result.second).z) << std::endl;
#endif
            return std::make_pair(true, ray.getPoint(farClipDist*result.second));
        }
    }
}

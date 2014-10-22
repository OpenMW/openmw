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

namespace CSVWorld
{
    PhysicsSystem::PhysicsSystem(Ogre::SceneManager *sceneMgr) : mSceneMgr(sceneMgr)
    {
        // Create physics. shapeLoader is deleted by the physic engine
        NifBullet::ManualBulletShapeLoader* shapeLoader = new NifBullet::ManualBulletShapeLoader();
        mEngine = new OEngine::Physic::PhysicEngine(shapeLoader);

        mEngine->setSceneManager(sceneMgr); // needed for toggleDebugRendering()

        // material for visual cue
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
                                  //Ogre::Vector3* scaledBoxTranslation,
                                  //Ogre::Quaternion* boxRotation,
                                  //bool raycasting,
                                  bool placeable)
    {
        //handleToMesh[node->getName()] = mesh;

        mEngine->createAndAdjustRigidBody(mesh, name, scale, position, rotation,
                                          0, 0, true, placeable);
    }

    void PhysicsSystem::toggleDebugRendering()
    {
        mEngine->toggleDebugRendering();
        mEngine->stepSimulation(0.0167); // FIXME: DebugDrawer::step() not accessible
        Ogre::Root::getSingleton().renderOneFrame(); // FIXME: temporary workaround for immediate visual feedback
    }

    /*std::pair<bool, Ogre::Vector3>*/ void PhysicsSystem::castRay(float mouseX, float mouseY,
                                        Ogre::Vector3* normal, std::string* hit, Ogre::Camera *camera)
    {
        Ogre::Ray ray = camera->getCameraToViewportRay(mouseX, mouseY);
        Ogre::Vector3 from = ray.getOrigin();
        CSMSettings::UserSettings &userSettings = CSMSettings::UserSettings::instance();
        float farClipDist = userSettings.setting("Scene/far clip distance", QString("300000")).toFloat();
        Ogre::Vector3 to = ray.getPoint(farClipDist);
        //Ogre::Vector3 to = ray.getDirection();

        btVector3 _from, _to;
        _from = btVector3(from.x, from.y, from.z);
        _to = btVector3(to.x, to.y, to.z);

        bool raycastingObjectOnly = true;
        bool ignoreHeightMap = false;
        Ogre::Vector3 norm;
        std::pair<std::string, float> result =
            mEngine->rayTest(_from, _to, raycastingObjectOnly, ignoreHeightMap, &norm);

        if (result.first == "")
            //return std::make_pair(false, Ogre::Vector3());
            std::cout << "no hit" << std::endl;
        else
        {
            std::cout << "hit " << result.first
                      + " result " + std::to_string(result.second*farClipDist) << std::endl;
            std::cout << "normal " + std::to_string(norm.x)
                            + ", " + std::to_string(norm.y)
                            + ", " + std::to_string(norm.z) << std::endl;
            std::cout << "hit pos "+ std::to_string(ray.getPoint(farClipDist*result.second).x)
                            + ", " + std::to_string(ray.getPoint(farClipDist*result.second).y)
                            + ", " + std::to_string(ray.getPoint(farClipDist*result.second).z) << std::endl;
            if(mSceneMgr->hasSceneNode(result.first))
            {
                // FIXME: for debugging cursor position
                // If using orthographic projection the cursor position is very accurate,
                // but with the default perspective view the cursor position is not properly
                // calculated when using getCameraToViewportRay.
                // See http://www.ogre3d.org/forums/viewtopic.php?p=241933
                mSceneMgr->destroyManualObject("manual" + result.first);
                Ogre::ManualObject* manual = mSceneMgr->createManualObject("manual" + result.first);
                manual->begin("BaseWhite", Ogre::RenderOperation::OT_LINE_LIST);
                manual-> position(ray.getPoint(farClipDist*result.second).x,
                                  ray.getPoint(farClipDist*result.second).y,
                                  ray.getPoint(farClipDist*result.second).z);
                manual-> position(ray.getPoint(farClipDist*result.second).x,
                                  ray.getPoint(farClipDist*result.second).y,
                                  ray.getPoint(farClipDist*result.second).z+2000);
                manual-> position(ray.getPoint(farClipDist*result.second).x,
                                  ray.getPoint(farClipDist*result.second).y,
                                  ray.getPoint(farClipDist*result.second).z);
                manual-> position(ray.getPoint(farClipDist*result.second).x,
                                  ray.getPoint(farClipDist*result.second).y+2000,
                                  ray.getPoint(farClipDist*result.second).z);
                manual-> position(ray.getPoint(farClipDist*result.second).x,
                                  ray.getPoint(farClipDist*result.second).y,
                                  ray.getPoint(farClipDist*result.second).z);
                manual-> position(ray.getPoint(farClipDist*result.second).x+2000,
                                  ray.getPoint(farClipDist*result.second).y,
                                  ray.getPoint(farClipDist*result.second).z);
                manual->end();
                mSceneMgr->getRootSceneNode()->createChildSceneNode()->attachObject(manual);
                // end debugging cursor position

                Ogre::SceneNode *scene = mSceneMgr->getSceneNode(result.first);
                std::map<std::string, std::vector<std::string>>::iterator iter =
                                                        mSelectedEntities.find(result.first);
                if(iter != mSelectedEntities.end()) // currently selected
                {
                    //scene->showBoundingBox(false);
                    std::vector<std::string> deletedEntities = mSelectedEntities[result.first];
                    while(!deletedEntities.empty())
                    {
                        scene->detachObject(deletedEntities.back());
                        mSceneMgr->destroyEntity(deletedEntities.back());
                        deletedEntities.pop_back();
                    }
                    mSelectedEntities.erase(iter);
                    mSceneMgr->destroyManualObject("manual" + result.first);
                }
                else
                {
                    //scene->showBoundingBox(true);
                    std::vector<std::string> clonedEntities;
                    Ogre::SceneNode::ObjectIterator iter = scene->getAttachedObjectIterator();
                    iter.begin();
                    while(iter.hasMoreElements())
                    {
                        Ogre::MovableObject * element = iter.getNext();
                        if(element->getMovableType() != "Entity")
                            continue;

                        Ogre::Entity * e = dynamic_cast<Ogre::Entity *>(element);
                        if(mSceneMgr->hasEntity(e->getName()+"cover"))
                        {
                            // FIXME: this shouldn't really happen...
                            scene->detachObject(e->getName()+"cover");
                            mSceneMgr->destroyEntity(e->getName()+"cover");
                        }
                        Ogre::Entity * clone = e->clone(e->getName()+"cover");

                        Ogre::MaterialPtr mat =
                            Ogre::MaterialManager::getSingleton().getByName("TransMaterial");
                        if(!mat.isNull())
                        {
                            clone->setMaterial(mat);
                            scene->attachObject(clone);
                            clonedEntities.push_back(e->getName()+"cover");
                        }

                    }
                    mSelectedEntities[result.first] = clonedEntities;
                }
                // FIXME: temporary workaround for immediate visual feedback
                Ogre::Root::getSingleton().renderOneFrame();
            }
        }
    }
}

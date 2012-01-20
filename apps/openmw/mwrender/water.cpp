#include "water.hpp"

namespace MWRender {
  Water::Water (Ogre::Camera *camera, int top) : mCamera (camera), mViewport (camera->getViewport()), mSceneManager (camera->getSceneManager()), mTop(top) {
    try {
      Ogre::CompositorManager::getSingleton().addCompositor(mViewport, "Water", -1);
      Ogre::CompositorManager::getSingleton().setCompositorEnabled(mViewport, "Water", false);
    } catch(...) {
    }
    mIsUnderwater = false;
    mCamera->addListener(this);
        
    mWaterPlane = Ogre::Plane(Ogre::Vector3::UNIT_Y, mTop);
    
    Ogre::MeshManager::getSingleton().createPlane("water", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,  mWaterPlane, 14000, 14000, 10, 10, true, 1, 3,5, Ogre::Vector3::UNIT_Z);
    
    
    mWater = mSceneManager->createEntity("water");
    
      mWater->setMaterialName("Examples/Water0");
    
        
      
    
    mWaterNode = mSceneManager->getRootSceneNode()->createChildSceneNode();
    mWaterNode->attachObject(mWater);
   
  }


  Water::~Water() {
      mCamera->removeListener(this);
    
      mWaterNode->detachObject(mWater);
      mSceneManager->destroyEntity(mWater);
      mSceneManager->destroySceneNode(mWaterNode);
      
    Ogre::MeshManager::getSingleton().remove("water");
    //Ogre::TextureManager::getSingleton().remove("refraction");
    //Ogre::TextureManager::getSingleton().remove("reflection");
    Ogre::CompositorManager::getSingleton().removeCompositorChain(mViewport);
  }


  void Water::preRenderTargetUpdate(const Ogre::RenderTargetEvent& evt) {
    mWater->setVisible(false);

    if (evt.source == mReflectionTarget) {
      mCamera->enableReflection(mWaterPlane);
    } else {
    }
  }

  void Water::postRenderTargetUpdate(const Ogre::RenderTargetEvent& evt) {
    mWater->setVisible(true);

    if (evt.source == mReflectionTarget) {
      mCamera->disableReflection();
    } else {
    }
  }

  void Water::checkUnderwater(float  y) {

    if (mIsUnderwater && y > mTop) {
      try {
	Ogre::CompositorManager::getSingleton().setCompositorEnabled(mViewport, "Water", false);
      } catch(...) {
      }
      mIsUnderwater = false;
    } 

    if (!mIsUnderwater && y < mTop) {
      try {
	Ogre::CompositorManager::getSingleton().setCompositorEnabled(mViewport, "Water", true);
      } catch(...) {
      }
      mIsUnderwater = true;
    }
  }


  void Water::cameraPreRenderScene(Ogre::Camera *cam) {
    Ogre::Vector3 pos = cam->getPosition();
    
    if (pos != mOldCameraPos) {
      mWaterNode->setPosition(pos.x, 0, pos.z);
 
      mOldCameraPos = pos;
    }
  }

  void Water::cameraPostRenderScene(Ogre::Camera *cam) {
  }

  void Water::cameraDestroyed(Ogre::Camera *cam) {
  }

}

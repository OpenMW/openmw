#ifndef GAME_MWRENDER_WATER_H
#define GAME_MWRENDER_WATER_H

#include <Ogre.h>


namespace MWRender {

    /// Water rendering 	
  class Water : Ogre::RenderTargetListener, Ogre::Camera::Listener {
    Ogre::Camera *mCamera;
    Ogre::SceneManager *mSceneManager;
    Ogre::Viewport *mViewport;

    Ogre::RenderTarget *mRefractionTarget;
    Ogre::RenderTarget *mReflectionTarget;

    Ogre::Plane mWaterPlane;
    Ogre::SceneNode *mWaterNode;
    Ogre::Entity *mWater;

    Ogre::Vector3 mOldCameraPos;
    bool mIsUnderwater;



    void preRenderTargetUpdate(const Ogre::RenderTargetEvent&);
    void postRenderTargetUpdate(const Ogre::RenderTargetEvent&);

    void cameraPreRenderScene(Ogre::Camera *cam);
    void cameraPostRenderScene(Ogre::Camera *cam);
    void cameraDestroyed(Ogre::Camera *cam);
    
  public:
    
    Water (Ogre::Camera *camera);
    ~Water();

    void checkUnderwater();


  };
}

#endif

#ifndef GAME_MWRENDER_WATER_H
#define GAME_MWRENDER_WATER_H

#include <Ogre.h>
#include <components/esm/loadcell.hpp>

namespace MWRender {

    /// Water rendering 	
  class Water : Ogre::RenderTargetListener, Ogre::Camera::Listener {
      static const int CELL_SIZE = 8192;
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
    int mTop;



    void preRenderTargetUpdate(const Ogre::RenderTargetEvent&);
    void postRenderTargetUpdate(const Ogre::RenderTargetEvent&);

    void cameraPreRenderScene(Ogre::Camera *cam);
    void cameraPostRenderScene(Ogre::Camera *cam);
    void cameraDestroyed(Ogre::Camera *cam);
    Ogre::Vector3 getSceneNodeCoordinates(int gridX, int gridY);
    
  public:
    
    Water (Ogre::Camera *camera, const ESM::Cell* cell);
    ~Water();

    void checkUnderwater(float y);
    void changeCell(const ESM::Cell* cell);


  };
}

#endif

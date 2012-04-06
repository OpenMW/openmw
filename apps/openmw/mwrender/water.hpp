#ifndef GAME_MWRENDER_WATER_H
#define GAME_MWRENDER_WATER_H

#include <Ogre.h>
#include <components/esm/loadcell.hpp>

#include "renderconst.hpp"

namespace MWRender {

    class SkyManager;

    /// Water rendering 	
    class Water : public Ogre::RenderTargetListener
    {
        static const int CELL_SIZE = 8192;
        Ogre::Camera *mCamera;
        Ogre::SceneManager *mSceneManager;
        Ogre::Viewport *mViewport;

        Ogre::Plane mWaterPlane;
        Ogre::SceneNode *mWaterNode;
        Ogre::Entity *mWater;

        bool mIsUnderwater;
        bool mActive;
        int mTop;

        Ogre::Vector3 getSceneNodeCoordinates(int gridX, int gridY);

    protected:
        void preRenderTargetUpdate(const Ogre::RenderTargetEvent& evt);
        void postRenderTargetUpdate(const Ogre::RenderTargetEvent& evt);

        SkyManager* mSky;

        Ogre::MaterialPtr createMaterial();

        Ogre::RenderTarget* mReflectionTarget;

        int mVisibilityFlags;
        int mReflectDistance;
        int mOldCameraFarClip;

    public:
        Water (Ogre::Camera *camera, SkyManager* sky, const ESM::Cell* cell);
        ~Water();

        void setActive(bool active);

        void toggle();

        void setViewportBackground(const Ogre::ColourValue& bg);

        void checkUnderwater(float y);
        void changeCell(const ESM::Cell* cell);
        void setHeight(const float height);

    };

}

#endif

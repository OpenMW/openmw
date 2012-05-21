#ifndef GAME_MWRENDER_WATER_H
#define GAME_MWRENDER_WATER_H

#include <Ogre.h>
#include <components/esm/loadcell.hpp>

#include "renderconst.hpp"

namespace MWRender {

    class SkyManager;
    class RenderingManager;

    /// Water rendering 	
    class Water : public Ogre::RenderTargetListener, public Ogre::RenderQueueListener
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
        bool mToggled;
        int mTop;

        bool mReflectionRenderActive;

        Ogre::Vector3 getSceneNodeCoordinates(int gridX, int gridY);

    protected:
        void preRenderTargetUpdate(const Ogre::RenderTargetEvent& evt);
        void postRenderTargetUpdate(const Ogre::RenderTargetEvent& evt);

        void renderQueueStarted (Ogre::uint8 queueGroupId, const Ogre::String &invocation, bool &skipThisInvocation);
        void renderQueueEnded (Ogre::uint8 queueGroupId, const Ogre::String &invocation, bool &repeatThisInvocation);

        void updateVisible();

        RenderingManager* mRendering;
        SkyManager* mSky;

        std::string mCompositorName;

        void createMaterial();
        Ogre::MaterialPtr mMaterial;

        Ogre::Camera* mReflectionCamera;

        Ogre::RenderTarget* mReflectionTarget;

        bool mUnderwaterEffect;
        int mVisibilityFlags;

    public:
        Water (Ogre::Camera *camera, RenderingManager* rend, const ESM::Cell* cell);
        ~Water();

        void setActive(bool active);

        void toggle();
        void update();

        void setViewportBackground(const Ogre::ColourValue& bg);

        void checkUnderwater(float y);
        void changeCell(const ESM::Cell* cell);
        void setHeight(const float height);

    };

}

#endif

#ifndef GAME_MWRENDER_WATER_H
#define GAME_MWRENDER_WATER_H

#include <OgrePlane.h>
#include <OgreRenderQueue.h>
#include <OgreRenderQueueListener.h>
#include <OgreRenderTargetListener.h>
#include <OgreMaterial.h>
#include <OgreTexture.h>

#include <components/esm/loadcell.hpp>
#include <components/settings/settings.hpp>

#include "renderconst.hpp"

#include <extern/shiny/Main/MaterialInstance.hpp>

namespace Ogre
{
    class Camera;
    class SceneManager;
    class SceneNode;
    class Entity;
    class Vector3;
    struct RenderTargetEvent;
};

namespace MWRender {

    class SkyManager;
    class RenderingManager;

    /// Water rendering
    class Water : public Ogre::RenderTargetListener, public Ogre::RenderQueueListener, public sh::MaterialInstanceListener
    {
        static const int CELL_SIZE = 8192;
        Ogre::Camera *mCamera;
        Ogre::SceneManager *mSceneManager;

        Ogre::Plane mWaterPlane;
        Ogre::Plane mErrorPlane;

        Ogre::SceneNode *mWaterNode;
        Ogre::Entity *mWater;

        //Ogre::SceneNode* mUnderwaterDome;

        bool mIsUnderwater;
        bool mActive;
        bool mToggled;
        int mTop;

        int mOldFarClip;
        int mOldFarClip2;

        float mWaterTimer;

        bool mReflectionRenderActive;

        Ogre::Vector3 getSceneNodeCoordinates(int gridX, int gridY);

    protected:
        void preRenderTargetUpdate(const Ogre::RenderTargetEvent& evt);
        void postRenderTargetUpdate(const Ogre::RenderTargetEvent& evt);

        void renderQueueStarted (Ogre::uint8 queueGroupId, const Ogre::String &invocation, bool &skipThisInvocation);
        void renderQueueEnded (Ogre::uint8 queueGroupId, const Ogre::String &invocation, bool &repeatThisInvocation);

        void applyRTT();
        void applyVisibilityMask();

        void updateVisible();

        RenderingManager* mRendering;
        SkyManager* mSky;

        std::string mCompositorName;

        Ogre::MaterialPtr mMaterial;

        Ogre::Camera* mReflectionCamera;

        Ogre::TexturePtr mReflectionTexture;
        Ogre::RenderTarget* mReflectionTarget;

        bool mUnderwaterEffect;
        int mVisibilityFlags;

    public:
        Water (Ogre::Camera *camera, RenderingManager* rend, const ESM::Cell* cell);
        ~Water();

        void setActive(bool active);

        void toggle();
        void update(float dt);

        void assignTextures();

        void setViewportBackground(const Ogre::ColourValue& bg);

        void processChangedSettings(const Settings::CategorySettingVector& settings);

        void checkUnderwater(float y);
        void changeCell(const ESM::Cell* cell);
        void setHeight(const float height);

        virtual void requestedConfiguration (sh::MaterialInstance* m, const std::string& configuration);
        virtual void createdConfiguration (sh::MaterialInstance* m, const std::string& configuration);

    };

}

#endif

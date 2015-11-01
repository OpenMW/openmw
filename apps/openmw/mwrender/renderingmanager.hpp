#ifndef OPENMW_MWRENDER_RENDERINGMANAGER_H
#define OPENMW_MWRENDER_RENDERINGMANAGER_H

#include <osg/ref_ptr>
#include <osg/Light>

#include <components/settings/settings.hpp>

#include "objects.hpp"

#include "renderinginterface.hpp"
#include "rendermode.hpp"

namespace osg
{
    class Group;
    class PositionAttitudeTransform;
}

namespace Resource
{
    class ResourceSystem;
}

namespace osgViewer
{
    class Viewer;
}

namespace ESM
{
    struct Cell;
}

namespace Terrain
{
    class World;
}

namespace MWWorld
{
    class Fallback;
}

namespace MWRender
{

    class StateUpdater;

    class EffectManager;
    class SkyManager;
    class NpcAnimation;
    class Pathgrid;
    class Camera;
    class Water;

    class RenderingManager : public MWRender::RenderingInterface
    {
    public:
        RenderingManager(osgViewer::Viewer* viewer, osg::ref_ptr<osg::Group> rootNode, Resource::ResourceSystem* resourceSystem,
                         const MWWorld::Fallback* fallback, const std::string& resourcePath);
        ~RenderingManager();

        MWRender::Objects& getObjects();

        Resource::ResourceSystem* getResourceSystem();

        void setNightEyeFactor(float factor);

        void setAmbientColour(const osg::Vec4f& colour);

        void skySetDate(int day, int month);
        int skyGetMasserPhase() const;
        int skyGetSecundaPhase() const;
        void skySetMoonColour(bool red);

        void setSunDirection(const osg::Vec3f& direction);
        void setSunColour(const osg::Vec4f& colour);

        void configureAmbient(const ESM::Cell* cell);
        void configureFog(const ESM::Cell* cell);
        void configureFog(float fogDepth, float underwaterFog, const osg::Vec4f& colour);

        void addCell(const MWWorld::CellStore* store);
        void removeCell(const MWWorld::CellStore* store);

        void updatePtr(const MWWorld::Ptr& old, const MWWorld::Ptr& updated);

        void rotateObject(const MWWorld::Ptr& ptr, const osg::Quat& rot);
        void moveObject(const MWWorld::Ptr& ptr, const osg::Vec3f& pos);
        void scaleObject(const MWWorld::Ptr& ptr, const osg::Vec3f& scale);

        void removeObject(const MWWorld::Ptr& ptr);

        void setWaterEnabled(bool enabled);
        void setWaterHeight(float level);

        /// Take a screenshot of w*h onto the given image, not including the GUI.
        void screenshot(osg::Image* image, int w, int h);

        struct RayResult
        {
            bool mHit;
            osg::Vec3f mHitNormalWorld;
            osg::Vec3f mHitPointWorld;
            MWWorld::Ptr mHitObject;
        };

        RayResult castRay(const osg::Vec3f& origin, const osg::Vec3f& dest, bool ignorePlayer, bool ignoreActors=false);

        /// Return the object under the mouse cursor / crosshair position, given by nX and nY normalized screen coordinates,
        /// where (0,0) is the top left corner.
        RayResult castCameraToViewportRay(const float nX, const float nY, float maxDistance, bool ignorePlayer, bool ignoreActors=false);

        /// Get the bounding box of the given object in screen coordinates as (minX, minY, maxX, maxY), with (0,0) being the top left corner.
        osg::Vec4f getScreenBounds(const MWWorld::Ptr& ptr);

        void setSkyEnabled(bool enabled);

        bool toggleRenderMode(RenderMode mode);

        SkyManager* getSkyManager();

        osg::Vec3f getEyePos();

        void spawnEffect(const std::string &model, const std::string &texture, const osg::Vec3f &worldPosition, float scale = 1.f);

        /// Clear all savegame-specific data
        void clear();

        /// Clear all worldspace-specific data
        void notifyWorldSpaceChanged();

        void update(float dt, bool paused);

        Animation* getAnimation(const MWWorld::Ptr& ptr);
        Animation* getPlayerAnimation();

        void addWaterRippleEmitter(const MWWorld::Ptr& ptr);
        void removeWaterRippleEmitter(const MWWorld::Ptr& ptr);

        void updatePlayerPtr(const MWWorld::Ptr &ptr);

        void removePlayer(const MWWorld::Ptr& player);
        void setupPlayer(const MWWorld::Ptr& player);
        void renderPlayer(const MWWorld::Ptr& player);

        void rebuildPtr(const MWWorld::Ptr& ptr);

        void processChangedSettings(const Settings::CategorySettingVector& settings);

        float getNearClipDistance() const;

        float getTerrainHeightAt(const osg::Vec3f& pos);

        // camera stuff
        bool vanityRotateCamera(const float *rot);
        void setCameraDistance(float dist, bool adjust, bool override);
        void resetCamera();
        float getCameraDistance() const;
        Camera* getCamera();
        void togglePOV();
        void togglePreviewMode(bool enable);
        bool toggleVanityMode(bool enable);
        void allowVanityMode(bool allow);
        void togglePlayerLooking(bool enable);
        void changeVanityModeScale(float factor);

    private:
        void updateProjectionMatrix();
        void updateTextureFiltering();
        void updateAmbient();
        void setFogColor(const osg::Vec4f& color);

        osg::ref_ptr<osgViewer::Viewer> mViewer;
        osg::ref_ptr<osg::Group> mRootNode;
        osg::ref_ptr<osg::Group> mLightRoot;
        Resource::ResourceSystem* mResourceSystem;

        osg::ref_ptr<osg::Light> mSunLight;

        std::auto_ptr<Pathgrid> mPathgrid;
        std::auto_ptr<Objects> mObjects;
        std::auto_ptr<Water> mWater;
        std::auto_ptr<Terrain::World> mTerrain;
        std::auto_ptr<SkyManager> mSky;
        std::auto_ptr<EffectManager> mEffectManager;
        std::auto_ptr<NpcAnimation> mPlayerAnimation;
        osg::ref_ptr<osg::PositionAttitudeTransform> mPlayerNode;
        std::auto_ptr<Camera> mCamera;

        osg::ref_ptr<StateUpdater> mStateUpdater;

        float mFogDepth;
        osg::Vec4f mUnderwaterColor;
        float mUnderwaterWeight;
        float mUnderwaterFog;
        float mUnderwaterIndoorFog;
        osg::Vec4f mFogColor;

        osg::Vec4f mAmbientColor;
        float mNightEyeFactor;

        float mNearClip;
        float mViewDistance;
        float mFieldOfView;

        void operator = (const RenderingManager&);
        RenderingManager(const RenderingManager&);
    };

}

#endif

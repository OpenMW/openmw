#ifndef OPENMW_MWRENDER_RENDERINGMANAGER_H
#define OPENMW_MWRENDER_RENDERINGMANAGER_H

#include <osg/ref_ptr>
#include <osg/Light>
#include <osg/Camera>

#include <components/settings/settings.hpp>

#include "objects.hpp"

#include "renderinginterface.hpp"
#include "rendermode.hpp"

namespace osg
{
    class Group;
    class PositionAttitudeTransform;
}

namespace osgUtil
{
    class IntersectionVisitor;
    class Intersector;
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

namespace Fallback
{
    class Map;
}

namespace SceneUtil
{
    class WorkQueue;
    class UnrefQueue;
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
    class TerrainStorage;
    class LandManager;

    class RenderingManager : public MWRender::RenderingInterface
    {
    public:
        RenderingManager(osgViewer::Viewer* viewer, osg::ref_ptr<osg::Group> rootNode, Resource::ResourceSystem* resourceSystem, SceneUtil::WorkQueue* workQueue,
                         const Fallback::Map* fallback, const std::string& resourcePath);
        ~RenderingManager();

        MWRender::Objects& getObjects();

        Resource::ResourceSystem* getResourceSystem();

        SceneUtil::WorkQueue* getWorkQueue();
        SceneUtil::UnrefQueue* getUnrefQueue();
        Terrain::World* getTerrain();

        osg::Uniform* mUniformNear;
        osg::Uniform* mUniformFar;

        void preloadCommonAssets();

        double getReferenceTime() const;

        osg::Group* getLightRoot();

        void setNightEyeFactor(float factor);

        void setAmbientColour(const osg::Vec4f& colour);

        void skySetDate(int day, int month);
        int skyGetMasserPhase() const;
        int skyGetSecundaPhase() const;
        void skySetMoonColour(bool red);

        void setSunDirection(const osg::Vec3f& direction);
        void setSunColour(const osg::Vec4f& diffuse, const osg::Vec4f& specular);

        void configureAmbient(const ESM::Cell* cell);
        void configureFog(const ESM::Cell* cell);
        void configureFog(float fogDepth, float underwaterFog, float dlFactor, float dlOffset, const osg::Vec4f& colour);

        void addCell(const MWWorld::CellStore* store);
        void removeCell(const MWWorld::CellStore* store);

        void enableTerrain(bool enable);

        void updatePtr(const MWWorld::Ptr& old, const MWWorld::Ptr& updated);

        void rotateObject(const MWWorld::Ptr& ptr, const osg::Quat& rot);
        void moveObject(const MWWorld::Ptr& ptr, const osg::Vec3f& pos);
        void scaleObject(const MWWorld::Ptr& ptr, const osg::Vec3f& scale);

        void removeObject(const MWWorld::Ptr& ptr);

        void setWaterEnabled(bool enabled);
        void setWaterHeight(float level);

        /// Take a screenshot of w*h onto the given image, not including the GUI.
        void screenshot(osg::Image* image, int w, int h, osg::Matrixd cameraTransform=osg::Matrixd());
        bool screenshot360(osg::Image* image, std::string settingStr);

        struct RayResult
        {
            bool mHit;
            osg::Vec3f mHitNormalWorld;
            osg::Vec3f mHitPointWorld;
            MWWorld::Ptr mHitObject;
            float mRatio;
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

        void spawnEffect(const std::string &model, const std::string &texture, const osg::Vec3f &worldPosition, float scale = 1.f, bool isMagicVFX = true);

        /// Clear all savegame-specific data
        void clear();

        /// Clear all worldspace-specific data
        void notifyWorldSpaceChanged();

        void update(float dt, bool paused);

        Animation* getAnimation(const MWWorld::Ptr& ptr);
        const Animation* getAnimation(const MWWorld::ConstPtr& ptr) const;

        void addWaterRippleEmitter(const MWWorld::Ptr& ptr);
        void removeWaterRippleEmitter(const MWWorld::Ptr& ptr);
        void emitWaterRipple(const osg::Vec3f& pos);

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
        const osg::Vec3f& getCameraPosition() const;
        void togglePOV();
        void togglePreviewMode(bool enable);
        bool toggleVanityMode(bool enable);
        void allowVanityMode(bool allow);
        void togglePlayerLooking(bool enable);
        void changeVanityModeScale(float factor);

        /// temporarily override the field of view with given value.
        void overrideFieldOfView(float val);
        /// reset a previous overrideFieldOfView() call, i.e. revert to field of view specified in the settings file.
        void resetFieldOfView();

        void exportSceneGraph(const MWWorld::Ptr& ptr, const std::string& filename, const std::string& format);

        LandManager* getLandManager() const;

        bool toggleBorders();

    private:
        void updateProjectionMatrix();
        void updateTextureFiltering();
        void updateAmbient();
        void setFogColor(const osg::Vec4f& color);

        void reportStats() const;

        void renderCameraToImage(osg::Camera *camera, osg::Image *image, int w, int h);

        osg::ref_ptr<osgUtil::IntersectionVisitor> getIntersectionVisitor(osgUtil::Intersector* intersector, bool ignorePlayer, bool ignoreActors);

        osg::ref_ptr<osgUtil::IntersectionVisitor> mIntersectionVisitor;

        osg::ref_ptr<osgViewer::Viewer> mViewer;
        osg::ref_ptr<osg::Group> mRootNode;
        osg::ref_ptr<osg::Group> mSceneRoot;
        Resource::ResourceSystem* mResourceSystem;

        osg::ref_ptr<SceneUtil::WorkQueue> mWorkQueue;
        osg::ref_ptr<SceneUtil::UnrefQueue> mUnrefQueue;

        osg::ref_ptr<osg::Light> mSunLight;

        std::unique_ptr<Pathgrid> mPathgrid;
        std::unique_ptr<Objects> mObjects;
        std::unique_ptr<Water> mWater;
        std::unique_ptr<Terrain::World> mTerrain;
        TerrainStorage* mTerrainStorage;
        std::unique_ptr<SkyManager> mSky;
        std::unique_ptr<EffectManager> mEffectManager;
        osg::ref_ptr<NpcAnimation> mPlayerAnimation;
        osg::ref_ptr<SceneUtil::PositionAttitudeTransform> mPlayerNode;
        std::unique_ptr<Camera> mCamera;
        osg::Vec3f mCurrentCameraPos;

        osg::ref_ptr<StateUpdater> mStateUpdater;

        float mLandFogStart;
        float mLandFogEnd;
        float mUnderwaterFogStart;
        float mUnderwaterFogEnd;
        osg::Vec4f mUnderwaterColor;
        float mUnderwaterWeight;
        float mUnderwaterIndoorFog;
        osg::Vec4f mFogColor;

        osg::Vec4f mAmbientColor;
        float mNightEyeFactor;

        float mNearClip;
        float mViewDistance;
        bool mDistantFog : 1;
        bool mDistantTerrain : 1;
        bool mFieldOfViewOverridden : 1;
        float mFieldOfViewOverride;
        float mFieldOfView;
        float mFirstPersonFieldOfView;
        bool mBorders;

        void operator = (const RenderingManager&);
        RenderingManager(const RenderingManager&);
    };

}

#endif

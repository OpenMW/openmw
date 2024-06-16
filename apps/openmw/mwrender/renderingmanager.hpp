#ifndef OPENMW_MWRENDER_RENDERINGMANAGER_H
#define OPENMW_MWRENDER_RENDERINGMANAGER_H

#include <span>

#include <osg/Light>
#include <osg/ref_ptr>

#include <components/settings/settings.hpp>

#include <osgUtil/IncrementalCompileOperation>

#include "objects.hpp"
#include "renderinginterface.hpp"
#include "rendermode.hpp"

#include <deque>
#include <memory>
#include <unordered_map>

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
    struct FormId;
    using RefNum = FormId;
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
    class ShadowManager;
    class WorkQueue;
    class LightManager;
    class UnrefQueue;
}

namespace DetourNavigator
{
    struct Navigator;
    struct Settings;
    struct AgentBounds;
}

namespace MWWorld
{
    class GroundcoverStore;
    class Cell;
}

namespace Debug
{
    struct DebugDrawer;
}

namespace MWRender
{
    class StateUpdater;
    class SharedUniformStateUpdater;
    class PerViewUniformStateUpdater;
    class IntersectionVisitorWithIgnoreList;

    class EffectManager;
    class ScreenshotManager;
    class FogManager;
    class SkyManager;
    class NpcAnimation;
    class Pathgrid;
    class Camera;
    class Water;
    class TerrainStorage;
    class LandManager;
    class NavMesh;
    class ActorsPaths;
    class RecastMesh;
    class ObjectPaging;
    class Groundcover;
    class PostProcessor;

    class RenderingManager : public MWRender::RenderingInterface
    {
    public:
        RenderingManager(osgViewer::Viewer* viewer, osg::ref_ptr<osg::Group> rootNode,
            Resource::ResourceSystem* resourceSystem, SceneUtil::WorkQueue* workQueue,
            DetourNavigator::Navigator& navigator, const MWWorld::GroundcoverStore& groundcoverStore,
            SceneUtil::UnrefQueue& unrefQueue);
        ~RenderingManager();

        osgUtil::IncrementalCompileOperation* getIncrementalCompileOperation();

        MWRender::Objects& getObjects() override;

        Resource::ResourceSystem* getResourceSystem();

        SceneUtil::WorkQueue* getWorkQueue();
        Terrain::World* getTerrain();

        void preloadCommonAssets();

        double getReferenceTime() const;

        SceneUtil::LightManager* getLightRoot();

        void setNightEyeFactor(float factor);

        void setAmbientColour(const osg::Vec4f& colour);

        void skySetDate(int day, int month);
        int skyGetMasserPhase() const;
        int skyGetSecundaPhase() const;
        void skySetMoonColour(bool red);

        void setSunDirection(const osg::Vec3f& direction);
        void setSunColour(const osg::Vec4f& diffuse, const osg::Vec4f& specular, float sunVis);
        void setNight(bool isNight) { mNight = isNight; }

        void configureAmbient(const MWWorld::Cell& cell);
        void configureFog(const MWWorld::Cell& cell);
        void configureFog(
            float fogDepth, float underwaterFog, float dlFactor, float dlOffset, const osg::Vec4f& colour);

        void addCell(const MWWorld::CellStore* store);
        void removeCell(const MWWorld::CellStore* store);

        void enableTerrain(bool enable, ESM::RefId worldspace);

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
            ESM::RefNum mHitRefnum;
            float mRatio;
        };

        RayResult castRay(const osg::Vec3f& origin, const osg::Vec3f& dest, bool ignorePlayer,
            bool ignoreActors = false, std::span<const MWWorld::Ptr> ignoreList = {});

        /// Return the object under the mouse cursor / crosshair position, given by nX and nY normalized screen
        /// coordinates, where (0,0) is the top left corner.
        RayResult castCameraToViewportRay(
            const float nX, const float nY, float maxDistance, bool ignorePlayer, bool ignoreActors = false);

        /// Get the bounding box of the given object in screen coordinates as (minX, minY, maxX, maxY), with (0,0) being
        /// the top left corner.
        osg::Vec4f getScreenBounds(const osg::BoundingBox& worldbb);

        void setSkyEnabled(bool enabled);

        bool toggleRenderMode(RenderMode mode);

        SkyManager* getSkyManager();

        void spawnEffect(const std::string& model, std::string_view texture, const osg::Vec3f& worldPosition,
            float scale = 1.f, bool isMagicVFX = true);

        /// Clear all savegame-specific data
        void clear();

        /// Clear all worldspace-specific data
        void notifyWorldSpaceChanged();

        void update(float dt, bool paused);

        Animation* getAnimation(const MWWorld::Ptr& ptr);
        const Animation* getAnimation(const MWWorld::ConstPtr& ptr) const;

        PostProcessor* getPostProcessor();

        void addWaterRippleEmitter(const MWWorld::Ptr& ptr);
        void removeWaterRippleEmitter(const MWWorld::Ptr& ptr);
        void emitWaterRipple(const osg::Vec3f& pos);

        void updatePlayerPtr(const MWWorld::Ptr& ptr);

        void removePlayer(const MWWorld::Ptr& player);
        void setupPlayer(const MWWorld::Ptr& player);
        void renderPlayer(const MWWorld::Ptr& player);

        void rebuildPtr(const MWWorld::Ptr& ptr);

        void processChangedSettings(const Settings::CategorySettingVector& settings);

        float getNearClipDistance() const { return mNearClip; }
        float getViewDistance() const { return mViewDistance; }

        void setViewDistance(float distance, bool delay = false);

        float getTerrainHeightAt(const osg::Vec3f& pos, ESM::RefId worldspace);

        // camera stuff
        Camera* getCamera() { return mCamera.get(); }

        /// temporarily override the field of view with given value.
        void overrideFieldOfView(float val);
        void setFieldOfView(float val);
        float getFieldOfView() const;
        /// reset a previous overrideFieldOfView() call, i.e. revert to field of view specified in the settings file.
        void resetFieldOfView();

        osg::Vec3f getHalfExtents(const MWWorld::ConstPtr& object) const;

        // Return local bounding box. Safe to be called in parallel with cull thread.
        osg::BoundingBox getCullSafeBoundingBox(const MWWorld::Ptr& ptr) const;

        void exportSceneGraph(
            const MWWorld::Ptr& ptr, const std::filesystem::path& filename, const std::string& format);

        Debug::DebugDrawer& getDebugDrawer() const { return *mDebugDraw; }

        LandManager* getLandManager() const;

        bool toggleBorders();

        void updateActorPath(const MWWorld::ConstPtr& actor, const std::deque<osg::Vec3f>& path,
            const DetourNavigator::AgentBounds& agentBounds, const osg::Vec3f& start, const osg::Vec3f& end) const;

        void removeActorPath(const MWWorld::ConstPtr& actor) const;

        void setNavMeshNumber(const std::size_t value);

        void setActiveGrid(const osg::Vec4i& grid);

        bool pagingEnableObject(int type, const MWWorld::ConstPtr& ptr, bool enabled);
        void pagingBlacklistObject(int type, const MWWorld::ConstPtr& ptr);
        bool pagingUnlockCache();
        void getPagedRefnums(const osg::Vec4i& activeGrid, std::vector<ESM::RefNum>& out);

        void updateProjectionMatrix();

        void setScreenRes(int width, int height);

        void setNavMeshMode(Settings::NavMeshRenderMode value);

    private:
        void updateTextureFiltering();
        void updateAmbient();
        void setFogColor(const osg::Vec4f& color);
        void updateThirdPersonViewMode();

        struct WorldspaceChunkMgr
        {
            std::unique_ptr<Terrain::World> mTerrain;
            std::unique_ptr<ObjectPaging> mObjectPaging;
            std::unique_ptr<Terrain::World> mGroundcoverWorld;
            std::unique_ptr<ObjectPaging> mGroundcoverPaging;
            std::unique_ptr<Groundcover> mGroundcover;
        };

        WorldspaceChunkMgr& getWorldspaceChunkMgr(ESM::RefId worldspace);

        void reportStats() const;

        void updateNavMesh();

        void updateRecastMesh();

        const bool mSkyBlending;

        osg::ref_ptr<osgUtil::IntersectionVisitor> getIntersectionVisitor(osgUtil::Intersector* intersector,
            bool ignorePlayer, bool ignoreActors, std::span<const MWWorld::Ptr> ignoreList = {});

        osg::ref_ptr<IntersectionVisitorWithIgnoreList> mIntersectionVisitor;

        osg::ref_ptr<osgViewer::Viewer> mViewer;
        osg::ref_ptr<osg::Group> mRootNode;
        osg::ref_ptr<SceneUtil::LightManager> mSceneRoot;
        Resource::ResourceSystem* mResourceSystem;

        osg::ref_ptr<SceneUtil::WorkQueue> mWorkQueue;

        osg::ref_ptr<osg::Light> mSunLight;

        DetourNavigator::Navigator& mNavigator;
        std::unique_ptr<NavMesh> mNavMesh;
        std::size_t mNavMeshNumber = 0;
        std::unique_ptr<ActorsPaths> mActorsPaths;
        std::unique_ptr<RecastMesh> mRecastMesh;
        std::unique_ptr<Pathgrid> mPathgrid;
        std::unique_ptr<Objects> mObjects;
        std::unique_ptr<Water> mWater;
        std::unordered_map<ESM::RefId, WorldspaceChunkMgr> mWorldspaceChunks;
        Terrain::World* mTerrain;
        std::unique_ptr<TerrainStorage> mTerrainStorage;
        ObjectPaging* mObjectPaging;
        Groundcover* mGroundcover;
        Terrain::World* mGroundcoverWorld;
        ObjectPaging* mGroundcoverPaging;
        std::unique_ptr<SkyManager> mSky;
        std::unique_ptr<FogManager> mFog;
        std::unique_ptr<ScreenshotManager> mScreenshotManager;
        std::unique_ptr<EffectManager> mEffectManager;
        std::unique_ptr<SceneUtil::ShadowManager> mShadowManager;
        osg::ref_ptr<PostProcessor> mPostProcessor;
        osg::ref_ptr<NpcAnimation> mPlayerAnimation;
        osg::ref_ptr<SceneUtil::PositionAttitudeTransform> mPlayerNode;
        std::unique_ptr<Camera> mCamera;
        osg::ref_ptr<Debug::DebugDrawer> mDebugDraw;

        osg::ref_ptr<StateUpdater> mStateUpdater;
        osg::ref_ptr<SharedUniformStateUpdater> mSharedUniformStateUpdater;
        osg::ref_ptr<PerViewUniformStateUpdater> mPerViewUniformStateUpdater;

        osg::Vec4f mAmbientColor;
        float mNightEyeFactor;

        float mNearClip;
        float mViewDistance;
        bool mFieldOfViewOverridden;
        float mFieldOfViewOverride;
        float mFieldOfView;
        float mFirstPersonFieldOfView;
        bool mUpdateProjectionMatrix = false;
        bool mNight = false;
        const MWWorld::GroundcoverStore& mGroundCoverStore;

        void operator=(const RenderingManager&);
        RenderingManager(const RenderingManager&);
    };

}

#endif

#ifndef GAME_RENDERING_MANAGER_H
#define GAME_RENDERING_MANAGER_H

#include "sky.hpp"
#include "debugging.hpp"

#include <components/settings/settings.hpp>

#include <boost/filesystem.hpp>

#include <OgreRenderTargetListener.h>

#include "renderinginterface.hpp"

#include "objects.hpp"
#include "actors.hpp"
#include "camera.hpp"
#include "occlusionquery.hpp"

namespace Ogre
{
    class SceneNode;
}

namespace MWWorld
{
    class Ptr;
    class CellStore;
}

namespace sh
{
    class Factory;
}

namespace Terrain
{
    class World;
}

namespace MWRender
{
    class Shadows;
    class LocalMap;
    class Water;
    class GlobalMap;
    class Animation;
    class EffectManager;

class RenderingManager: private RenderingInterface, public Ogre::RenderTargetListener, public OEngine::Render::WindowSizeListener
{
private:
    virtual MWRender::Objects& getObjects();
    virtual MWRender::Actors& getActors();

public:
    RenderingManager(OEngine::Render::OgreRenderer& _rend, const boost::filesystem::path& resDir,
                     const boost::filesystem::path& cacheDir, OEngine::Physic::PhysicEngine* engine,
                     MWWorld::Fallback* fallback);
    virtual ~RenderingManager();

    void togglePOV()
    { mCamera->toggleViewMode(); }

    void togglePreviewMode(bool enable)
    { mCamera->togglePreviewMode(enable); }

    bool toggleVanityMode(bool enable)
    { return mCamera->toggleVanityMode(enable); }

    void allowVanityMode(bool allow)
    { mCamera->allowVanityMode(allow); }

    void togglePlayerLooking(bool enable)
    { mCamera->togglePlayerLooking(enable); }

    void changeVanityModeScale(float factor)
    {
        if(mCamera->isVanityOrPreviewModeEnabled())
            mCamera->setCameraDistance(-factor/120.f*10, true, true);
    }

    void resetCamera();

    bool vanityRotateCamera(const float *rot);
    void setCameraDistance(float dist, bool adjust = false, bool override = true);
    float getCameraDistance() const;

    void setupPlayer(const MWWorld::Ptr &ptr);
    void renderPlayer(const MWWorld::Ptr &ptr);

    SkyManager* getSkyManager();

    MWRender::Camera* getCamera() const;

    bool toggleRenderMode(int mode);

    void removeCell (MWWorld::CellStore *store);

    /// \todo this function should be removed later. Instead the rendering subsystems should track
    /// when rebatching is needed and update automatically at the end of each frame.
    void cellAdded (MWWorld::CellStore *store);

    /// Clear all savegame-specific data (i.e. fog of war textures)
    void clear();

    void enableTerrain(bool enable);

    void removeWater();

    /// Write current fog of war for this cell to the CellStore
    void writeFog (MWWorld::CellStore* store);

    void addObject (const MWWorld::Ptr& ptr, const std::string& model);
    void removeObject (const MWWorld::Ptr& ptr);

    void moveObject (const MWWorld::Ptr& ptr, const Ogre::Vector3& position);
    void scaleObject (const MWWorld::Ptr& ptr, const Ogre::Vector3& scale);

    /// Updates an object's rotation
    void rotateObject (const MWWorld::Ptr& ptr);

    void setWaterHeight(float height);
    void setWaterEnabled(bool enabled);
    bool toggleWater();
    bool toggleWorld();

    /// Updates object rendering after cell change
    /// \param old Object reference in previous cell
    /// \param cur Object reference in new cell
    void updateObjectCell(const MWWorld::Ptr &old, const MWWorld::Ptr &cur);

    /// Specifies an updated Ptr object for the player (used on cell change).
    void updatePlayerPtr(const MWWorld::Ptr &ptr);

    /// Currently for NPCs only. Rebuilds the NPC, updating their root model, animation sources,
    /// and equipment.
    void rebuildPtr(const MWWorld::Ptr &ptr);

    void update (float duration, bool paused);

    void setAmbientColour(const Ogre::ColourValue& colour);
    void setSunColour(const Ogre::ColourValue& colour);
    void setSunDirection(const Ogre::Vector3& direction, bool is_night);
    void sunEnable(bool real); ///< @param real whether or not to really disable the sunlight (otherwise just set diffuse to 0)
    void sunDisable(bool real);

    void disableLights(bool sun); ///< @param sun whether or not to really disable the sunlight (otherwise just set diffuse to 0)
    void enableLights(bool sun);


    void preRenderTargetUpdate(const Ogre::RenderTargetEvent& evt);
    void postRenderTargetUpdate(const Ogre::RenderTargetEvent& evt);

    bool occlusionQuerySupported() { return mOcclusionQuery->supported(); }
    OcclusionQuery* getOcclusionQuery() { return mOcclusionQuery; }

    float getTerrainHeightAt (Ogre::Vector3 worldPos);

    void notifyWorldSpaceChanged();

    void getTriangleBatchCount(unsigned int &triangles, unsigned int &batches);

    void setGlare(bool glare);
    void skyEnable ();
    void skyDisable ();
    void skySetHour (double hour);
    void skySetDate (int day, int month);
    int skyGetMasserPhase() const;
    int skyGetSecundaPhase() const;
    void skySetMoonColour (bool red);
    void configureAmbient(MWWorld::CellStore &mCell);

    void addWaterRippleEmitter (const MWWorld::Ptr& ptr, float scale = 1.f, float force = 1.f);
    void removeWaterRippleEmitter (const MWWorld::Ptr& ptr);
    void updateWaterRippleEmitterPtr (const MWWorld::Ptr& old, const MWWorld::Ptr& ptr);

    void updateTerrain ();
    ///< update the terrain according to the player position. Usually done automatically, but should be done manually
    /// before calling requestMap

    void requestMap (MWWorld::CellStore* cell);
    ///< request the local map for a cell

    /// configure fog according to cell
    void configureFog(const MWWorld::CellStore &mCell);

    /// configure fog manually
    void configureFog(const float density, const Ogre::ColourValue& colour);

    Ogre::Vector4 boundingBoxToScreen(Ogre::AxisAlignedBox bounds);
    ///< transform the specified bounding box (in world coordinates) into screen coordinates.
    /// @return packed vector4 (min_x, min_y, max_x, max_y)

    void processChangedSettings(const Settings::CategorySettingVector& settings);

    Ogre::Viewport* getViewport() { return mRendering.getViewport(); }

    void worldToInteriorMapPosition (Ogre::Vector2 position, float& nX, float& nY, int &x, int& y);
    ///< see MWRender::LocalMap::worldToInteriorMapPosition

    Ogre::Vector2 interiorMapToWorldPosition (float nX, float nY, int x, int y);
    ///< see MWRender::LocalMap::interiorMapToWorldPosition

    bool isPositionExplored (float nX, float nY, int x, int y, bool interior);
    ///< see MWRender::LocalMap::isPositionExplored

    Animation* getAnimation(const MWWorld::Ptr &ptr);

    void frameStarted(float dt, bool paused);
    void screenshot(Ogre::Image& image, int w, int h);

    void spawnEffect (const std::string& model, const std::string& texture, const Ogre::Vector3& worldPosition, float scale=1.f);

protected:
    virtual void windowResized(int x, int y);

private:
    sh::Factory* mFactory;

    void setAmbientMode();
    void applyFog(bool underwater);

    void attachCameraTo(const MWWorld::Ptr& ptr);

    void setMenuTransparency(float val);

    bool mSunEnabled;

    MWWorld::Fallback* mFallback;

    SkyManager* mSkyManager;

    OcclusionQuery* mOcclusionQuery;

    Terrain::World* mTerrain;

    MWRender::Water *mWater;

    GlobalMap* mGlobalMap;

    OEngine::Render::OgreRenderer &mRendering;

    MWRender::Objects* mObjects;
    MWRender::Actors* mActors;

    MWRender::EffectManager* mEffectManager;

    MWRender::NpcAnimation *mPlayerAnimation;

    // 0 normal, 1 more bright, 2 max
    int mAmbientMode;

    Ogre::ColourValue mAmbientColor;
    Ogre::Light* mSun;

    Ogre::SceneNode *mRootNode;

    Ogre::ColourValue mFogColour;
    float mFogStart;
    float mFogEnd;

    OEngine::Physic::PhysicEngine* mPhysicsEngine;

    MWRender::Camera *mCamera;

    MWRender::Debugging *mDebugging;

    MWRender::LocalMap* mLocalMap;

    MWRender::Shadows* mShadows;

    bool mRenderWorld;
};

}

#endif

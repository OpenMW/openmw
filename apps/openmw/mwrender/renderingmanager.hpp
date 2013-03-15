#ifndef _GAME_RENDERING_MANAGER_H
#define _GAME_RENDERING_MANAGER_H

#include "sky.hpp"
#include "terrain.hpp"
#include "debugging.hpp"

#include <openengine/ogre/fader.hpp>

#include <components/settings/settings.hpp>

#include <boost/filesystem.hpp>

#include <OgreRenderTargetListener.h>

#include "renderinginterface.hpp"

#include "objects.hpp"
#include "actors.hpp"
#include "player.hpp"
#include "occlusionquery.hpp"

namespace Ogre
{
    class SceneManager;
    class SceneNode;
    class Quaternion;
    class Vector3;
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

namespace MWRender
{
    class Shadows;
    class LocalMap;
    class Water;
    class Compositors;
    class ExternalRendering;
    class GlobalMap;
    class VideoPlayer;
    class Animation;

class RenderingManager: private RenderingInterface, public Ogre::WindowEventListener, public Ogre::RenderTargetListener {

  private:


    virtual MWRender::Objects& getObjects();
    virtual MWRender::Actors& getActors();

  public:
    RenderingManager(OEngine::Render::OgreRenderer& _rend, const boost::filesystem::path& resDir,
                     const boost::filesystem::path& cacheDir, OEngine::Physic::PhysicEngine* engine,MWWorld::Fallback* fallback);
    virtual ~RenderingManager();

    void togglePOV() {
        mPlayer->toggleViewMode();
    }

    void togglePreviewMode(bool enable) {
        mPlayer->togglePreviewMode(enable);
    }

    bool toggleVanityMode(bool enable, bool force) {
        return mPlayer->toggleVanityMode(enable, force);
    }

    void allowVanityMode(bool allow) {
        mPlayer->allowVanityMode(allow);
    }

    void togglePlayerLooking(bool enable) {
        mPlayer->togglePlayerLooking(enable);
    }

    void changeVanityModeScale(float factor) {
        if (mPlayer->isVanityOrPreviewModeEnabled())
        mPlayer->setCameraDistance(-factor/120.f*10, true, true);
    }

    void getPlayerData(Ogre::Vector3 &eyepos, float &pitch, float &yaw);

    void attachCameraTo(const MWWorld::Ptr &ptr);
    void renderPlayer(const MWWorld::Ptr &ptr);

    SkyManager* getSkyManager();
    Compositors* getCompositors();

    void toggleLight();
    bool toggleRenderMode(int mode);

    OEngine::Render::Fader* getFader();

    void removeCell (MWWorld::CellStore *store);

    /// \todo this function should be removed later. Instead the rendering subsystems should track
    /// when rebatching is needed and update automatically at the end of each frame.
    void cellAdded (MWWorld::CellStore *store);
    void waterAdded(MWWorld::CellStore *store);

    void removeWater();

    void preCellChange (MWWorld::CellStore* store);
    ///< this event is fired immediately before changing cell

    void addObject (const MWWorld::Ptr& ptr);
    void removeObject (const MWWorld::Ptr& ptr);

    void moveObject (const MWWorld::Ptr& ptr, const Ogre::Vector3& position);
    void scaleObject (const MWWorld::Ptr& ptr, const Ogre::Vector3& scale);

    /// Rotates object accordingly to its type
    /// \param rot euler angles in radians
    /// \param adjust indicates should rotation be set or adjusted
    /// \return true if object needs to be rotated physically
    bool rotateObject (const MWWorld::Ptr& ptr, Ogre::Vector3 &rot, bool adjust = false);

    void setWaterHeight(const float height);
    void toggleWater();

    /// Updates object rendering after cell change
    /// \param old Object reference in previous cell
    /// \param cur Object reference in new cell
    void updateObjectCell(const MWWorld::Ptr &old, const MWWorld::Ptr &cur);

    void update (float duration, bool paused);

    void setAmbientColour(const Ogre::ColourValue& colour);
    void setSunColour(const Ogre::ColourValue& colour);
    void setSunDirection(const Ogre::Vector3& direction);
    void sunEnable(bool real); ///< @param real whether or not to really disable the sunlight (otherwise just set diffuse to 0)
    void sunDisable(bool real);

    void disableLights(bool sun); ///< @param sun whether or not to really disable the sunlight (otherwise just set diffuse to 0)
    void enableLights(bool sun);


    void preRenderTargetUpdate(const Ogre::RenderTargetEvent& evt);
    void postRenderTargetUpdate(const Ogre::RenderTargetEvent& evt);

    bool occlusionQuerySupported() { return mOcclusionQuery->supported(); }
    OcclusionQuery* getOcclusionQuery() { return mOcclusionQuery; }

    Shadows* getShadows();

    void switchToInterior();
    void switchToExterior();

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

    void requestMap (MWWorld::CellStore* cell);
    ///< request the local map for a cell

    /// configure fog according to cell
    void configureFog(MWWorld::CellStore &mCell);

    /// configure fog manually
    void configureFog(const float density, const Ogre::ColourValue& colour);

    Ogre::Vector4 boundingBoxToScreen(Ogre::AxisAlignedBox bounds);
    ///< transform the specified bounding box (in world coordinates) into screen coordinates.
    /// @return packed vector4 (min_x, min_y, max_x, max_y)

    void processChangedSettings(const Settings::CategorySettingVector& settings);

    Ogre::Viewport* getViewport() { return mRendering.getViewport(); }

    void getInteriorMapPosition (Ogre::Vector2 position, float& nX, float& nY, int &x, int& y);
    ///< see MWRender::LocalMap::getInteriorMapPosition

    bool isPositionExplored (float nX, float nY, int x, int y, bool interior);
    ///< see MWRender::LocalMap::isPositionExplored

    void setupExternalRendering (MWRender::ExternalRendering& rendering);

    Animation* getAnimation(const MWWorld::Ptr &ptr);

    void playVideo(const std::string& name, bool allowSkipping);
    void stopVideo();
    void frameStarted(float dt);

  protected:
	virtual void windowResized(Ogre::RenderWindow* rw);
    virtual void windowClosed(Ogre::RenderWindow* rw);

  private:

    sh::Factory* mFactory;

    void setAmbientMode();
    void applyFog(bool underwater);

    void setMenuTransparency(float val);

    void applyCompositors();

    bool mSunEnabled;

    MWWorld::Fallback* mFallback;

    SkyManager* mSkyManager;

    OcclusionQuery* mOcclusionQuery;

    TerrainManager* mTerrainManager;

    MWRender::Water *mWater;

    GlobalMap* mGlobalMap;

    OEngine::Render::OgreRenderer &mRendering;

    MWRender::Objects mObjects;
    MWRender::Actors mActors;

    // 0 normal, 1 more bright, 2 max
    int mAmbientMode;

    Ogre::ColourValue mAmbientColor;
    Ogre::Light* mSun;

    Ogre::SceneNode *mRootNode;

    Ogre::ColourValue mFogColour;
    float mFogStart;
    float mFogEnd;

    OEngine::Physic::PhysicEngine* mPhysicsEngine;

    MWRender::Player *mPlayer;

    MWRender::Debugging *mDebugging;

    MWRender::LocalMap* mLocalMap;

    MWRender::Shadows* mShadows;

    MWRender::Compositors* mCompositors;

    VideoPlayer* mVideoPlayer;
};

}

#endif

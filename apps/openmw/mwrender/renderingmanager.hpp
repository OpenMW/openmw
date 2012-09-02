#ifndef _GAME_RENDERING_MANAGER_H
#define _GAME_RENDERING_MANAGER_H

#include "sky.hpp"
#include "terrain.hpp"
#include "debugging.hpp"

#include <openengine/ogre/fader.hpp>

#include <components/settings/settings.hpp>

#include <boost/filesystem.hpp>

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

class RenderingManager: private RenderingInterface, public Ogre::WindowEventListener {

  private:


    virtual MWRender::Objects& getObjects();
    virtual MWRender::Actors& getActors();

  public:
    RenderingManager(OEngine::Render::OgreRenderer& _rend, const boost::filesystem::path& resDir,
                     const boost::filesystem::path& cacheDir, OEngine::Physic::PhysicEngine* engine);
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

    static const bool useMRT();

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

    /// Moves object rendering part to proper container
    /// \param store Cell the object was in previously (\a ptr has already been updated to the new cell).
    void moveObjectToCell (const MWWorld::Ptr& ptr, const Ogre::Vector3& position, MWWorld::CellStore *store);

    void update (float duration);

    void setAmbientColour(const Ogre::ColourValue& colour);
    void setSunColour(const Ogre::ColourValue& colour);
    void setSunDirection(const Ogre::Vector3& direction);
    void sunEnable();
    void sunDisable();

    void disableLights();
    void enableLights();

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

    void requestMap (MWWorld::CellStore* cell);
    ///< request the local map for a cell

    /// configure fog according to cell
    void configureFog(MWWorld::CellStore &mCell);

    /// configure fog manually
    void configureFog(const float density, const Ogre::ColourValue& colour);

    void playAnimationGroup (const MWWorld::Ptr& ptr, const std::string& groupName, int mode,
        int number = 1);
    ///< Run animation for a MW-reference. Calls to this function for references that are currently not
    /// in the rendered scene should be ignored.
    ///
    /// \param mode: 0 normal, 1 immediate start, 2 immediate loop
    /// \param number How offen the animation should be run

    void skipAnimation (const MWWorld::Ptr& ptr);
    ///< Skip the animation for the given MW-reference for one frame. Calls to this function for
    /// references that are currently not in the rendered scene should be ignored.

    Ogre::Vector4 boundingBoxToScreen(Ogre::AxisAlignedBox bounds);
    ///< transform the specified bounding box (in world coordinates) into screen coordinates.
    /// @return packed vector4 (min_x, min_y, max_x, max_y)

    void processChangedSettings(const Settings::CategorySettingVector& settings);

    Ogre::Viewport* getViewport() { return mRendering.getViewport(); }

    static bool waterShaderSupported();

    virtual void getInteriorMapPosition (Ogre::Vector2 position, float& nX, float& nY, int &x, int& y);
    ///< see MWRender::LocalMap::getInteriorMapPosition

    virtual bool isPositionExplored (float nX, float nY, int x, int y, bool interior);
    ///< see MWRender::LocalMap::isPositionExplored

  protected:
	virtual void windowResized(Ogre::RenderWindow* rw);
    virtual void windowClosed(Ogre::RenderWindow* rw);

  private:

    sh::Factory* mFactory;

    void setAmbientMode();

    void setMenuTransparency(float val);

    void applyCompositors();

    bool mSunEnabled;

    SkyManager* mSkyManager;

    OcclusionQuery* mOcclusionQuery;

    TerrainManager* mTerrainManager;

    MWRender::Water *mWater;

    OEngine::Render::OgreRenderer &mRendering;

    MWRender::Objects mObjects;
    MWRender::Actors mActors;

    // 0 normal, 1 more bright, 2 max
    int mAmbientMode;

    Ogre::ColourValue mAmbientColor;
    Ogre::Light* mSun;

    /// Root node for all objects added to the scene. This is rotated so
    /// that the OGRE coordinate system matches that used internally in
    /// Morrowind.
    Ogre::SceneNode *mMwRoot;

    OEngine::Physic::PhysicEngine* mPhysicsEngine;

    MWRender::Player *mPlayer;

    MWRender::Debugging *mDebugging;

    MWRender::LocalMap* mLocalMap;

    MWRender::Shadows* mShadows;

    MWRender::Compositors* mCompositors;
};

}

#endif

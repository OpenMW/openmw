#ifndef _GAME_RENDERING_MANAGER_H
#define _GAME_RENDERING_MANAGER_H


#include "sky.hpp"
#include "debugging.hpp"

#include "../mwworld/class.hpp"

#include <utility>
#include <openengine/ogre/renderer.hpp>
#include <openengine/ogre/fader.hpp>
#include <openengine/bullet/physic.hpp>

#include <vector>
#include <string>

#include "../mwworld/ptr.hpp"

#include <boost/filesystem.hpp>

#include "renderinginterface.hpp"

#include "objects.hpp"
#include "actors.hpp"
#include "player.hpp"
#include "water.hpp"
#include "localmap.hpp"

namespace Ogre
{
    class Camera;
    class Viewport;
    class SceneManager;
    class SceneNode;
    class RaySceneQuery;
    class Quaternion;
    class Vector3;
}

namespace MWWorld
{
    class World;
}

namespace MWRender
{



class RenderingManager: private RenderingInterface {

  private:


    virtual MWRender::Objects& getObjects();
    virtual MWRender::Actors& getActors();

  public:
    RenderingManager(OEngine::Render::OgreRenderer& _rend, const boost::filesystem::path& resDir, OEngine::Physic::PhysicEngine* engine, MWWorld::Environment& environment);
    virtual ~RenderingManager();



    virtual MWRender::Player& getPlayer(); /// \todo move this to private again as soon as
                                            /// MWWorld::Player has been rewritten to not need access
                                            /// to internal details of the rendering system anymore

    SkyManager* getSkyManager();

    void toggleLight();
    bool toggleRenderMode(int mode);

    OEngine::Render::Fader* getFader();

    void removeCell (MWWorld::Ptr::CellStore *store);

    /// \todo this function should be removed later. Instead the rendering subsystems should track
    /// when rebatching is needed and update automatically at the end of each frame.
    void cellAdded (MWWorld::Ptr::CellStore *store);
    void waterAdded(MWWorld::Ptr::CellStore *store);

    void removeWater();
    
    void preCellChange (MWWorld::Ptr::CellStore* store);
    ///< this event is fired immediately before changing cell

    void addObject (const MWWorld::Ptr& ptr);
    void removeObject (const MWWorld::Ptr& ptr);

    void moveObject (const MWWorld::Ptr& ptr, const Ogre::Vector3& position);
    void scaleObject (const MWWorld::Ptr& ptr, const Ogre::Vector3& scale);
    void rotateObject (const MWWorld::Ptr& ptr, const::Ogre::Quaternion& orientation);

    void checkUnderwater();
    void setWaterHeight(const float height);
    void toggleWater();

    /// \param store Cell the object was in previously (\a ptr has already been updated to the new cell).
    void moveObjectToCell (const MWWorld::Ptr& ptr, const Ogre::Vector3& position, MWWorld::Ptr::CellStore *store);

    void update (float duration);

    void setAmbientColour(const Ogre::ColourValue& colour);
    void setSunColour(const Ogre::ColourValue& colour);
    void setSunDirection(const Ogre::Vector3& direction);
    void sunEnable();
    void sunDisable();

    void setGlare(bool glare);
    void skyEnable ();
    void skyDisable ();
    void skySetHour (double hour);
    void skySetDate (int day, int month);
    int skyGetMasserPhase() const;
    int skyGetSecundaPhase() const;
    void skySetMoonColour (bool red);
    void configureAmbient(ESMS::CellStore<MWWorld::RefData> &mCell);

    void requestMap (MWWorld::Ptr::CellStore* cell);
    ///< request the local map for a cell

    /// configure fog according to cell
    void configureFog(ESMS::CellStore<MWWorld::RefData> &mCell);

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

  private:

    void setAmbientMode();

    SkyManager* mSkyManager;

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
    Ogre::RaySceneQuery *mRaySceneQuery;

    OEngine::Physic::PhysicEngine* mPhysicsEngine;

    MWRender::Player *mPlayer;

    MWRender::Debugging *mDebugging;

    MWRender::LocalMap* mLocalMap;
};

}

#endif

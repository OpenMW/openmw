#ifndef _GAME_RENDERING_MANAGER_H
#define _GAME_RENDERING_MANAGER_H


#include "sky.hpp"
#include "debugging.hpp"

#include <utility>
#include <openengine/ogre/renderer.hpp>
#include <openengine/bullet/physic.hpp>

#include <vector>
#include <string>

#include "../mwworld/ptr.hpp"

#include <boost/filesystem.hpp>
#include "objects.hpp"
#include "npcs.hpp"
#include "creatures.hpp"

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
    class Player;
    class RenderingInterface{
    public:
        virtual MWRender::Npcs& getNPCs();
        virtual MWRender::Creatures& getCreatures();
        virtual MWRender::Objects& getObjects();
	    virtual MWRender::Player* getPlayer();
    };

class RenderingManager: private RenderingInterface {

	 OEngine::Render::OgreRenderer &rend;
	 Ogre::Camera* camera;
	 MWRender::Npcs npcs;
	 MWRender::Creatures creatures;
	 MWRender::Objects objects;

        /// Root node for all objects added to the scene. This is rotated so
        /// that the OGRE coordinate system matches that used internally in
        /// Morrowind.
        Ogre::SceneNode *mwRoot;
        Ogre::RaySceneQuery *mRaySceneQuery;

        OEngine::Physic::PhysicEngine* eng;

        MWRender::Player *mPlayer;
		MWRender::Debugging mDebugging;

  public:
    RenderingManager(OEngine::Render::OgreRenderer& _rend, const boost::filesystem::path& resDir, OEngine::Physic::PhysicEngine* engine);
    ~RenderingManager();
	virtual MWRender::Npcs& getNPCs();
    virtual MWRender::Creatures& getCreatures();
    virtual MWRender::Objects& getObjects();
    virtual MWRender::Player* getPlayer();
	bool toggleRenderMode(int mode);

    void removeCell (MWWorld::Ptr::CellStore *store); // TODO do we want this?
    
    void addObject (const MWWorld::Ptr& ptr, MWWorld::Ptr::CellStore *store);
    void removeObject (const MWWorld::Ptr& ptr, MWWorld::Ptr::CellStore *store);
    
    void moveObject (const MWWorld::Ptr& ptr, const Ogre::Vector3& position);
    void scaleObject (const MWWorld::Ptr& ptr, const Ogre::Vector3& scale);
    void rotateObject (const MWWorld::Ptr& ptr, const::Ogre::Quaternion& orientation);
    void moveObjectToCell (const MWWorld::Ptr& ptr, const Ogre::Vector3& position, MWWorld::Ptr::CellStore *store);
    
    void setPhysicsDebugRendering (bool);
    bool getPhysicsDebugRendering() const;
    
    void update (float duration);
    
    void skyEnable ();
    void skyDisable ();
    void skySetHour (double hour);
    void skySetDate (int day, int month);
    int skyGetMasserPhase() const;
    int skyGetSecundaPhase() const;
    void skySetMoonColour (bool red);
	OEngine::Render::OgreRenderer& getOgreRenderer(){return rend;}
	Ogre::SceneManager *getMgr() { return rend.getScene(); }
	Ogre::SceneNode *getRoot() { return mwRoot; }
    
  private:
    
    SkyManager* mSkyManager;
    
    
};

}

#endif

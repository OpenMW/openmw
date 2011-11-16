#ifndef _GAME_RENDERING_MANAGER_H
#define _GAME_RENDERING_MANAGER_H


#include "sky.hpp"
#include "debugging.hpp"
//Commented out for now, possibly do an ifdef if necessary, cla

#include "../mwworld/class.hpp"

#include <utility>
#include <openengine/ogre/renderer.hpp>
#include <openengine/bullet/physic.hpp>

#include <vector>
#include <string>

#include "../mwworld/ptr.hpp"

#include <boost/filesystem.hpp>

#include "renderinginterface.hpp"
#include "npcs.hpp"
#include "creatures.hpp"
#include "objects.hpp"
#include "player.hpp"

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

	

  public:
    RenderingManager(OEngine::Render::OgreRenderer& _rend, const boost::filesystem::path& resDir, OEngine::Physic::PhysicEngine* engine);
    virtual ~RenderingManager();
	virtual MWRender::Npcs& getNPCs();
    virtual MWRender::Creatures& getCreatures();
    virtual MWRender::Objects& getObjects();
    virtual MWRender::Player& getPlayer();

	void toggleLight();
	bool toggleRenderMode(int mode);

    void removeCell (MWWorld::Ptr::CellStore *store); // TODO do we want this?
    
    void addObject (const MWWorld::Ptr& ptr);
    void removeObject (const MWWorld::Ptr& ptr);
    
    void moveObject (const MWWorld::Ptr& ptr, const Ogre::Vector3& position);
    void scaleObject (const MWWorld::Ptr& ptr, const Ogre::Vector3& scale);
    void rotateObject (const MWWorld::Ptr& ptr, const::Ogre::Quaternion& orientation);

    /// \param store Cell the object was in previously (\a ptr has already been updated to the new cell).
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
    void configureAmbient(ESMS::CellStore<MWWorld::RefData> &mCell);
		 /// configure fog according to cell
    void configureFog(ESMS::CellStore<MWWorld::RefData> &mCell);
    void setAmbientMode();
    SkyManager* mSkyManager;
	 OEngine::Render::OgreRenderer &rend;
	 Ogre::Camera* camera;
	MWRender::Npcs npcs;
	 MWRender::Creatures creatures;
	 MWRender::Objects objects;

	// 0 normal, 1 more bright, 2 max
    int mAmbientMode;

    Ogre::ColourValue mAmbientColor;

        /// Root node for all objects added to the scene. This is rotated so
        /// that the OGRE coordinate system matches that used internally in
        /// Morrowind.
        Ogre::SceneNode *mwRoot;
        Ogre::RaySceneQuery *mRaySceneQuery;

        OEngine::Physic::PhysicEngine* eng;

        MWRender::Player *mPlayer;
		MWRender::Debugging mDebugging;
    
    
};

}

#endif

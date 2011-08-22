#ifndef _GAME_RENDERING_MANAGER_H
#define _GAME_RENDERING_MANAGER_H


#include "sky.hpp"

#include "../mwworld/ptr.hpp"
#include <openengine/ogre/renderer.hpp>
#include <openengine/bullet/physic.hpp>

namespace MWRender
{

class RenderingManager {
  public:
    RenderingManager(SkyManager *skyManager);
    ~RenderingManager();

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
    
  private:
    
    SkyManager* mSkyManager;
    
    
};

}

#endif

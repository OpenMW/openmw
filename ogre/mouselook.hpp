#ifndef OENGINE_OGRE_MOUSELOOK_H
#define OENGINE_OGRE_MOUSELOOK_H

/*
  A mouse-look class for Ogre. Accepts input events from Mangle::Input
  and translates them.

  You can adjust the mouse sensibility and switch to a different
  camera. The mouselook class also has an optional wrap protection
  that keeps the camera from flipping upside down.

  You can disable the mouse looker at any time by calling
  setCamera(NULL), and reenable it by setting the camera back.

  NOTE: The current implementation will ONLY work for native OIS
  events.
 */

#include <mangle/input/event.hpp>

namespace Ogre
{
    class Camera;
}

namespace OEngine {
namespace Render
{
  class MouseLookEvent : public Mangle::Input::Event
  {
    Ogre::Camera* camera;
    float sensX, sensY; // Mouse sensibility
    bool flipProt;      // Flip protection

  public:
    MouseLookEvent(Ogre::Camera *cam=NULL,
                   float sX=0.2, float sY=0.2,
                   bool prot=true)
      : camera(cam)
      , sensX(sX)
      , sensY(sy)
      , flipProt(prot)
    {}

    void setCamera(Ogre::Camera *cam)
    { camera = cam; }
    void setSens(float sX, float sY)
    { sensX = sX; sensY = sY; }
    void setProt(bool p) { flipProt = p; }

    void event(Type type, int index, const void *p);
  };
}}
#endif

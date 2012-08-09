#ifndef _MWINPUT_MOUSELOOKEVENT_H
#define _MWINPUT_MOUSELOOKEVENT_H

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

namespace MWInput
{
    class MouseLookEvent : public Mangle::Input::Event
    {
        float sensX, sensY; // Mouse sensibility
        bool flipProt;      // Flip protection
        bool mDisabled;

    public:
        MouseLookEvent(float sX = 0.2, float sY = 0.2, bool prot=true)
          : sensX(sX), sensY(sY), flipProt(prot)
        {}

        void setSens(float sX, float sY) {
            sensX = sX;
            sensY = sY;
        }

        void setProt(bool p) {
            flipProt = p;
        }

        void disable() {
            mDisabled = true;
        }

        void enable() {
            mDisabled = false;
        }

        void event(Type type, int index, const void *p);
  };

  typedef boost::shared_ptr<MouseLookEvent> MouseLookEventPtr;
}

#endif

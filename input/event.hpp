#ifndef MANGLE_INPUT_EVENT_H
#define MANGLE_INPUT_EVENT_H

#include "../tools/shared_ptr.hpp"

namespace Mangle
{
  namespace Input
  {
    /** Generic callback for input events. The meaning of the
        parameters depend on the system producing the events.
    */
    struct Event
    {
      /// Event types
      enum Type
        {
          EV_Unknown    = 1,    // Unknown event type
          EV_KeyDown    = 2,    // Keyboard button was pressed
          EV_KeyUp      = 4,    // Keyboard button was released
          EV_MouseMove  = 8,    // Mouse movement
          EV_MouseDown  = 16,   // Mouse button pressed
          EV_MouseUp    = 32,   // Mouse button released

          EV_ALL        = 63    // All events
        };

      /**
         Called upon all events. The first parameter give the event
         type, the second gives additional data (usually the local
         keysym or button index as defined by the driver), and the
         pointer points to the full custom event structure provided by
         the driver (the type may vary depending on the EventType,
         this is defined in the Driver documentation.)
       */
      virtual void event(Type type, int index, const void *p) = 0;
      virtual ~Event() {}
    };

    typedef boost::shared_ptr<Event> EventPtr;
  }
}
#endif

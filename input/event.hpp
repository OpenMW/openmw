#ifndef MANGLE_INPUT_EVENT_H
#define MANGLE_INPUT_EVENT_H

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
          EV_Unknown    = -1,   // Unknown event type
          EV_KeyDown    = 1,    // Key, mouse or other button was pressed
          EV_KeyUp      = 2,    // Key, mouse or other button was released
          EV_MouseMove  = 3,    // Mouse movement (all axis movement?)
          EV_Other      = 4     // Other event
        };

      /**
         Called upon all events. The first parameter give the event
         type, the second gives additional data (usually the local
         keysym as defined by the driver), and the pointer points to
         the full custom event structure provided by the driver (the
         type may vary depending on the EventType, this is defined in
         the Driver documentation.)
       */
      virtual void event(Type type, int index, const void *p) = 0;
      virtual ~Event() {}
    };
  }
}
#endif

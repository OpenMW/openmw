#ifndef MANGLE_INPUT_DRIVER_H
#define MANGLE_INPUT_DRIVER_H

#include "../tools/shared_ptr.hpp"
#include "event.hpp"

namespace Mangle
{
  namespace Input
  {
    /** Input::Driver is the main interface to any input system that
        handles keyboard and/or mouse input, along with any other
        input source like joysticks.

        It is really a generalized event system, and could also be
        used for non-input related events. The definition of the event
        codes and structures are entirely dependent on the
        implementation.

        A system-independent key code list will be found in keys.hpp,
        and input drivers should privide optional translations to/from
        this list for full compatibility.
     */
    struct Driver
    {
      Driver() : event(NULL) {}
      virtual ~Driver() {}

      /** Captures input and produces the relevant events from it. An
          event callback must be set with setEvent(), or all events
          will be ignored.
       */
      virtual void capture() = 0;

      /** Check the state of a given key or button. The key/button
          definitions depends on the driver.
       */
      virtual bool isDown(int index) = 0;

      /** Show or hide system mouse cursor
       */
      virtual void showMouse(bool show) = 0;

      /** Set the event handler for input events. The evt->event()
          function is called for each event. The meaning of the index
          and *p parameters will be specific to each driver and to
          each input system.
       */
      void setEvent(Event *evt)
      { event = evt; }

      /** Instigate an event. Is used internally for all events, but
          can also be called from the outside to "fake" events from
          this driver.
      */
      void makeEvent(Event::Type type, int index, const void *p=NULL)
      {
        if(event)
          event->event(type,index,p);
      }

    private:
      /// Holds the event callback set byt setEvent()
      Event *event;
    };

    typedef boost::shared_ptr<Driver> DriverPtr;
  }
}
#endif

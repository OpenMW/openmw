#ifndef _INPUT_EVENT_DISPATCHER_H
#define _INPUT_EVENT_DISPATCHER_H

#include "dispatch_map.hpp"
#include <boost/function.hpp>

namespace Input {

  /**
     The EventDispatcher translates an input event (as given by an
     identifying index and an optional void pointer) into an output
     event function call.
  */
  class EventDispatcher
  {
    DispatchMap map;

    /*
      The event callback function that is called for all events. The
      first parameter is the input event. The second parameter is the
      resolved output event. The third is an optional user-defined
      parameter passed to call().
    */
    typedef boost::function<void(int,int,void*)> EventCallback;

    EventCallback callback;

    // Carpal-tunnel prevention
    typedef DispatchMap::OutList _OL;

  public:

    /// Create an event binding connection
    void bind(int in, int out)
    { map.bind(in,out); }

    /// Dissolve an event binding connection
    void unbind(int in, int out)
    { map.unbind(in, out); }

    /// Check if a given input is bound to anything
    bool isBound(int in)
    { return map.isBound(in); }

    /// Register the callback you want to use to handle events.
    void setCallback(EventCallback cb)
    { callback = cb; }

    /**
       Instigate an event.

       This will look up the input event number (first parameter), and
       call the event callback for each output number associated with
       (bound to) that input.

       The optional second paramter is also passed to the callback.

       If no output is bound to the given event number, the callback
       is never called.
    */
    void call(int event, void *p = NULL)
    {
      // You have to set the callback before using call().
      assert(!callback.empty());

      // Not bound? Exit.
      if(!isBound(event)) return;

      // Dispatch to all events.
      const _OL &list = map.getList(event);
      for(_OL::const_iterator it = list.begin();
          it != list.end(); it++)
        callback(event, *it, p);
    }
  };

}
#endif

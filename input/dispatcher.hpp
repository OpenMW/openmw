#ifndef OENGINE_INPUT_DISPATCHER_H
#define OENGINE_INPUT_DISPATCHER_H

#include "dispatch_map.hpp"
#include "func_binder.hpp"
#include <mangle/input/event.hpp>

namespace Input {

struct Dispatcher : Mangle::Input::Event
{
  DispatchMap map;
  FuncBinder funcs;

  /**
     Constructor. Takes the number of actions and passes it to
     FuncBinder.
  */
  Dispatcher(int actions) : funcs(actions) {}

  void bind(int action, int key) { map.bind(key, action); }
  void unbind(int action, int key) { map.unbind(key, action); }
  bool isBound(int key) const { return map.isBound(key); }

  /**
     Instigate an event. It is translated through the dispatch map and
     sent to the function bindings.
   */
  typedef DispatchMap::OutList _O;
  void event(Type type, int index, const void* p)
  {
    // No bindings, nothing happens
    if(!isBound(index))
      return;

    // Only treat key-down events for now
    if(type != EV_KeyDown)
      return;

    // Get the mapped actions and execute them
    const _O &list = map.getList(index);
    _O::const_iterator it;
    for(it = list.begin(); it != list.end(); it++)
      funcs.call(*it, p);
  }
};

}
#endif

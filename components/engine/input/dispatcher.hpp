#ifndef _INPUT_DISPATCHER_H
#define _INPUT_DISPATCHER_H

#include "dispatch_map.hpp"
#include "func_binder.hpp"

namespace Input {

struct Dispatcher
{
  DispatchMap map;
  FuncBinder funcs;

  /**
     Constructor. Takes the number of actions and passes it to
     FuncBinder.
  */
  Dispatcher(int actions) : funcs(actions) {}

  void bind(int in, int out) { map.bind(in, out); }
  void unbind(int in, int out) { map.unbind(in, out); }
  bool isBound(int in) const { return map.isBound(in); }

  /**
     Instigate an event. It is translated through the dispatch map and
     sent to the function bindings.
   */
  typedef DispatchMap::OutList _O;
  void event(int index, const void* p=NULL) const
  {
    // No bindings, nothing happens
    if(!isBound(index))
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

#ifndef OENGINE_INPUT_DISPATCHER_H
#define OENGINE_INPUT_DISPATCHER_H

#include <cassert>
#include <iostream>

#include <mangle/input/event.hpp>

#include "dispatch_map.hpp"
#include "func_binder.hpp"

namespace OEngine {
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

  void bind(unsigned int action, int key)
  {
    assert(action < funcs.getSize());
    map.bind(key, action);
  }
  void unbind(unsigned int action, int key)
  {
    assert(action < funcs.getSize());
    map.unbind(key, action);
  }
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

    // Get the mapped actions and execute them
    const _O &list = map.getList(index);
    _O::const_iterator it;
    for(it = list.begin(); it != list.end(); it++)
    {
      //catch exceptions thrown in the input handlers so that pressing a key
      //doesn't cause OpenMw to crash
      try
      {
          funcs.call(*it, p);
      }
      catch(const std::exception& e)
      {
          std::cerr << "Exception in input handler: " << e.what() << std::endl;
      }
      catch(...)
      {
          std::cerr << "Unknown exception in input handler" << std::endl;
      }

    }
  }
};

// This helps us play nice with Mangle's EventPtr, but it should
// really be defined for all the classes in OEngine.
  typedef boost::shared_ptr<Dispatcher> DispatcherPtr;

}}
#endif

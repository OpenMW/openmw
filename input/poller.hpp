#ifndef _INPUT_POLLER_H
#define _INPUT_POLLER_H

#include "dispatch_map.hpp"
#include "oismanager.hpp"

namespace Input {

/** The poller is used to check (poll) for keys rather than waiting
    for events. TODO: Might make this OIS-independent later. */
struct Poller
{
  DispatchMap map;
  OIS::Keyboard *keyboard;

  Poller(Input::OISManager &ois)
  {
    keyboard = ois.keyboard;
    assert(keyboard);
  }

  /** Bind or unbind a given action with a key. The action is the first
      parameter, the key is the second.
  */
  void bind(int in, int out) { map.bind(in, out); }
  void unbind(int in, int out) { map.unbind(in, out); }
  bool isBound(int in) const { return map.isBound(in); }

  /// Check whether a given action button is currently pressed.
  typedef DispatchMap::OutList _O;
  bool isDown(int index) const
  {
    assert(keyboard);

    // No bindings, no action
    if(!isBound(index))
      return false;

    // Get all the keys bound to this action, and check them.
    const _O &list = map.getList(index);
    _O::const_iterator it;
    for(it = list.begin(); it != list.end(); it++)
      // If there's any match, we're good to go.
      if(keyboard->isKeyDown((OIS::KeyCode)*it)) return true;

    return false;
  }
};

}
#endif

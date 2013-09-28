#ifndef OENGINE_INPUT_DISPATCHMAP_H
#define OENGINE_INPUT_DISPATCHMAP_H

#include <set>
#include <map>
#include <cassert>

namespace OEngine {
namespace Input {

/**
   DispatchMap is a simple connection system that connects incomming
   signals with outgoing signals.

   The signals can be connected one-to-one, many-to-one, one-to-many
   or many-to-many.

   The dispatch map is completely system agnostic. It is a pure data
   structure and all signals are just integer indices. It does not
   delegate any actions, but used together with Dispatcher it can be
   used to build an event system.
 */
struct DispatchMap
{
  typedef std::set<int> OutList;
  typedef std::map<int, OutList> InMap;

  typedef OutList::iterator Oit;
  typedef InMap::iterator Iit;

  InMap map;

  void bind(int in, int out)
  {
    map[in].insert(out);
  }

  void unbind(int in, int out)
  {
    Iit it = map.find(in);
    if(it != map.end())
      {
        it->second.erase(out);

        // If there are no more elements, then remove the entire list
        if(it->second.empty())
          map.erase(it);
      }
  }

  /// Check if a given input is bound to anything
  bool isBound(int in) const
  {
    return map.find(in) != map.end();
  }

  /**
     Get the list of outputs bound to the given input. Only call this
     on inputs that you know are bound to something.

     The returned set is only intended for immediate iteration. Do not
     store references to it.
  */
  const OutList &getList(int in) const
  {
    assert(isBound(in));
    InMap::const_iterator it = map.find(in);
    assert(it != map.end());
    const OutList &out = it->second;
    assert(!out.empty());
    return out;
  }
};
}}
#endif

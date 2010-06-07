#ifndef _INPUT_FUNCBINDER_H
#define _INPUT_FUNCBINDER_H

#include <string>
#include <vector>
#include <boost/function.hpp>
#include <assert.h>

namespace Input {

/**
   An Action defines the user defined action corresponding to a
   binding.

   The first parameter is the action index that invoked this call. You
   can assign the same function to multiple actions, and this can help
   you keep track of which action was invoked.

   The second parameter is an optional user-defined parameter,
   represented by a void pointer. In many cases it is practical to
   point this to temporaries (stack values), so make sure not to store
   permanent references to it unless you've planning for this on the
   calling side as well.
 */
typedef boost::function<void(int,void*)> Action;

/**
   The FuncBinder is a simple struct that binds user-defined indices
   to functions. It is useful for binding eg. keyboard events to
   specific actions in your program, but can potentially have many
   other uses as well.
 */
class FuncBinder
{
  struct FuncBinding
  {
    std::string name;
    Action action;
  };

  std::vector<FuncBinding> bindings;

public:
  /**
     Constructor. Initialize the struct by telling it how many action
     indices you intend to bind.

     The indices you use should be 0 <= i < number.
  */
  FuncBinder(int number) : bindings(number) {}

  /**
     Bind an action to an index.
   */
  void bind(int index, Action action, const std::string &name="")
  {
    assert(index >= 0 && index < bindings.size());
    FuncBinding &fb = bindings[index];
    fb.action = action;
    fb.name = name;
  }

  /**
     Unbind an index, reverting a previous bind().
   */
  void unbind(int index)
  {
    assert(index >= 0 && index < bindings.size());
    bindings[index] = FuncBinding();
  }

  /**
     Call a specific action. Takes an optional parameter that is
     passed to the action.
   */
  void call(int index, void *p=NULL)
  {
    assert(index >= 0 && index < bindings.size());
    FuncBinding &fb = bindings[index];
    if(fb.action) fb.action(index, p);
  }

  /// Check if a given index is bound to anything
  bool isBound(int index)
  {
    assert(index >= 0 && index < bindings.size());
    return !bindings[index].action.empty();
  }

  /// Return the name associated with an action (empty if not bound)
  const std::string &getName(int index)
  {
    assert(index >= 0 && index < bindings.size());
    return bindings[index].name;
  }
};

}
#endif

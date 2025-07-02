# MWLua

This folder contains the C++ implementation of the Lua scripting system.

For user-facing documentation, see
[Lua scripting](https://openmw.readthedocs.io/en/latest/reference/lua-scripting/index.html).
The documentation is generated from
[/docs/source/reference/lua-scripting](/docs/source/reference/lua-scripting).
You can find instructions for generating the documentation at the
root of the [docs folder](/docs/README.md).

The Lua API reference is generated from the specifications in
[/files/lua_api](/files/lua_api/). They are written in the
Lua Development Tool [Documentation Language](https://wiki.eclipse.org/LDT/User_Area/Documentation_Language),
and also enable autocompletion for ([LDT](https://www.eclipse.org/ldt/)) users.
Please update them to reflect any changes you make.

## MWLua::LuaManager

The Lua manager is the central interface through which information flows
from the engine to the scripts and back.

Lua is executed in a separate [thread](/apps/openmw/mwlua/worker.hpp) by
[default](https://openmw.readthedocs.io/en/latest/reference/modding/settings/lua.html#lua-num-threads).
This thread executes `update()` in parallel with rendering logic (specifically with OSG Cull traversal).
Because of this, Lua must not synchronously mutate anything that can directly or indirectly affect the scene graph.
Instead such changes are queued to `mActionQueue`. They are then processed by
`synchronizedUpdate()`, which is executed by the main thread.
The Lua thread is paused while other updates of the game state take place,
which means that state that doesn't affect the scene graph
can be mutated immediately. There is no easy way to characterize
which things affect the graph, you'll need to inspect the code.

## Bindings

The bulk of the code in this folder consists of bindings that expose C++ data to Lua.

As explained in the [scripting overview](https://openmw.readthedocs.io/en/latest/reference/lua-scripting/overview.html),
there are Global and Local scripts, and they have different capabilities.
A Local script has read-only access to objects other the one it is attached to.
The bindings use the types `MWLua::GObject`, `MWLua::LObject`, `MWLua::SelfObject` to enforce this behaviour.

* `MWLua::GObject` is used in global scripts
* `MWLua::LObject` is used in local scripts (readonly),
* `MWLua::SelfObject` is the object the local script is attached to.
* `MWLua::Object` is the common base of all 3.

Functions that don't change objects are usually available in both local and global scripts so they accept `MWLua::Object`.
Some (for example `setEquipment` in [actor.cpp](https://gitlab.com/OpenMW/openmw/-/blob/master/apps/openmw/mwlua/types/actor.cpp))
should work only on self and because of this have argument of type `SelfObject`.
There are also cases where a function is available in both local and global scripts, but has different effects in different cases.
For example see the binding `actor["inventory"]` in 'MWLua::addActorBindings` in [actor.cpp](https://gitlab.com/OpenMW/openmw/-/blob/master/apps/openmw/mwlua/types/actor.cpp):

```cpp
actor["inventory"] = sol::overload([](const LObject& o) { return Inventory<LObject>{ o }; },
    [](const GObject& o) { return Inventory<GObject>{ o }; });
```

The difference is that `Inventory<LObject>` is readonly and `Inventory<GObject>` is mutable.
The read-only bindings are defined for both, but some functions are exclusive for `Inventory<GObject>`.

### Mutations that affect the scene graph

Because of the threading issues mentioned under `MWLua::LuaManager`,
bindings that mutate things that affect the scene graph
must be implemented by queuing an action with `LuaManager::addAction`.

Here is an example that illustrates action queuing,
along with the differences between `GObject` and `LObject`:

```cpp
// We can always read the value because OSG Cull doesn't modify `RefData`.
auto isEnabled = [](const Object& o) { return o.ptr().getRefData().isEnabled(); };

// Changing the value must be queued because `World::enable`/`World::disable` aside of
// changing `RefData` also adds/removes the object to the scene graph.
auto setEnabled = [context](const Object& object, bool enable) {
    // It is important that the lambda state stores `object` and not the result of
    // `object.ptr()` because when delayed will be executed the old Ptr can potentially
    // be already invalidated.
    context.mLuaManager->addAction([object, enable] {
        if (enable)
            MWBase::Environment::get().getWorld()->enable(object.ptr());
        else
            MWBase::Environment::get().getWorld()->disable(object.ptr());
    });
};

// Local scripts can only view the value (because in multiplayer local scripts
// will be client-side and we want to avoid synchronization issues).
LObjectMetatable["enabled"] = sol::readonly_property(isEnabled);

// Global scripts can both read and modify the value.
GObjectMetatable["enabled"] = sol::property(isEnabled, setEnabled);
```

Please note that queueing means changes scripts make won't be visible to other scripts before the
next frame. If you want to avoid that, you can implement a cache in the bindings.
The first write will create the cache and queue the value to be synchronized from the
cache to the engine in the next synchronization. Later writes will update the cache.
Reads will read the cache if it exists. See [LocalScripts::SelfObject::mStatsCache](/apps/openmw/mwlua/localscripts.hpp)
for an example.

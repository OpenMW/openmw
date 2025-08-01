Engine handlers reference
=========================

.. include:: version.rst

Engine handler is a function defined by a script, that can be called by the engine.

**Can be defined by any script**

|bdg-ctx-all|

.. list-table::
  :widths: 20 80

  * - onInterfaceOverride(base)
    - | Called if the current script has an interface and overrides an interface
      | (``base``) of another script.

**Can be defined by any non-menu script**

|bdg-ctx-global| |bdg-ctx-local|

.. list-table::
  :widths: 20 80

  * - onInit(initData)
    - | Called once when the script is created (not loaded). `InitData can be`
      | `assigned to a script in openmw-cs (not yet implemented).`
      | ``onInterfaceOverride`` can be called before ``onInit``.
  * - onUpdate(dt)
    - | Called every frame in the Lua thread (even if the game is paused). `dt` is
      | the simulation time from the last update in seconds.
  * - onSave() -> savedData
    - | Called when the game is saving. May be called in inactive state,
      | so it shouldn't use `openmw.nearby`.
  * - onLoad(savedData, initData)
    - | Called on loading with the data previosly returned by
      | ``onSave``. During loading the object is always inactive. ``initData`` is
      | the same as in ``onInit``.
      | Note that ``onLoad`` means loading a script rather than loading a game.
      | If a script did not exist when a game was saved onLoad will not be
      | called, but ``onInit`` will.

**Only for global scripts**

|bdg-ctx-global|

.. list-table::
  :widths: 20 80

  * - onNewGame()
    - New game is started.
  * - onPlayerAdded(player)
    - | Player added to the game world. The argument is a `Game object`. 
      | Note that this is triggered at the start of a game, and when a game is loaded.
  * - onObjectActive(object)
    - Object becomes active.
  * - onActorActive(actor)
    - Actor (NPC or Creature) becomes active.
  * - onItemActive(item)
    - | Item (Weapon, Potion, ...) becomes active in a cell.
      | Does not apply to items in inventories or containers.
  * - onActivate(object, actor)
    - Object is activated by an actor.
  * - onNewExterior(cell)
    - A new exterior cell not defined by a content file has been generated.

**Only for local scripts**

|bdg-ctx-local|

.. list-table::
  :widths: 20 80

  * - onActive()
    - | Called when the object becomes active
      | (either a player came to this cell again, or a save was loaded).
  * - onInactive()
    - | Object became inactive. Since it is inactive the handler
      | can not access anything nearby, but it is possible to send
      | an event to global scripts.
  * - onTeleported()
    - Object was teleported.
  * - onActivated(actor)
    - | Called on an object when an actor activates it. Note that picking
      | up an item is also an activation and works this way: (1) a copy of
      | the item is placed to the actor's inventory, (2) count of
      | the original item is set to zero, (3) and only then onActivated is
      | called on the original item, so self.count is already zero.
  * - onConsume(item)
    - | Called on an actor when they consume an item (e.g. a potion).
      | Similarly to onActivated, the item has already been removed
      | from the actor's inventory, and the count was set to zero.

**Only menu scripts and local scripts attached to a player**

|bdg-ctx-menu| |bdg-ctx-player|

.. list-table::
  :widths: 20 80

  * - onFrame(dt)
    - | Called every frame (even if the game is paused) right after
      | processing user input. Use it only for latency-critical stuff
      | and for UI that should work on pause.
      | `dt` is simulation time delta (0 when on pause).
  * - onKeyPress(key)
    - | `Key <openmw_input.html##(KeyboardEvent)>`_ is pressed.
      | Usage example:
      | ``if key.symbol == 'z' and key.withShift then ...``
  * - onKeyRelease(key)
    - | `Key <openmw_input.html##(KeyboardEvent)>`_ is released.
      | Usage example:
      | ``if key.symbol == 'z' and key.withShift then ...``
  * - onControllerButtonPress(id)
    - | A `button <openmw_input.html##(CONTROLLER_BUTTON)>`_ on a game controller is pressed.
      | Usage example:
      | ``if id == input.CONTROLLER_BUTTON.LeftStick then ...``
  * - onControllerButtonRelease(id)
    - | A `button <openmw_input.html##(CONTROLLER_BUTTON)>`_ on a game controller is released.
      | Usage example:
      | ``if id == input.CONTROLLER_BUTTON.LeftStick then ...``
  * - onInputAction(id)
    - | (DEPRECATED, use `registerActionHandler <openmw_input.html##(registerActionHandler)>`_)
      | `Game control <openmw_input.html##(ACTION)>`_ is pressed.
      | Usage example:
      | ``if id == input.ACTION.ToggleWeapon then ...``
  * - onTouchPress(touchEvent)
    - | A finger pressed on a touch device.
      | `Touch event <openmw_input.html##(TouchEvent)>`_.
  * - onTouchRelease(touchEvent)
    - | A finger released a touch device.
      | `Touch event <openmw_input.html##(TouchEvent)>`_.
  * - onTouchMove(touchEvent)
    - | A finger moved on a touch device.
      | `Touch event <openmw_input.html##(TouchEvent)>`_.
  * - onMouseButtonPress(button)
    - | A mouse button was pressed
      | Button id
  * - onMouseButtonRelease(button)
    - | A mouse button was released
      | Button id
  * - onMouseWheel(vertical, horizontal)
    - | Mouse wheel was scrolled
      | vertical and horizontal mouse wheel change
  * - | onConsoleCommand(
      |     mode, command, selectedObject)
    - | User entered `command` in in-game console. Called if either
      | `mode` is not default or `command` starts with prefix `lua`.

**Only for local scripts attached to a player**

|bdg-ctx-player|

.. list-table::
  :widths: 20 80

  * - onKeyPress(key)
    - | `Key <openmw_input.html##(KeyboardEvent)>`_ is pressed.
      | Usage example:
      | ``if key.symbol == 'z' and key.withShift then ...``
  * - onQuestUpdate(questId, stage)
    - | Called when a quest is updated.

**Only for menu scripts**

|bdg-ctx-menu|

.. list-table::
  :widths: 20 80

  * - onStateChanged()
    - | Called whenever the current game changes
      | (i. e. the result of `getState <openmw_menu.html##(menu).getState>`_ changes)

Engine handlers reference
=========================

Engine handler is a function defined by a script, that can be called by the engine.



**Can be defined by any script**

.. list-table::
  :widths: 20 80

  * - onInit(initData)
    - | Called once when the script is created (not loaded). `InitData can be`
      | `assigned to a script in openmw-cs (not yet implemented).`
      | ``onInterfaceOverride`` can be called before ``onInit``.
  * - onUpdate(dt)
    - | Called every frame if the game is not paused. `dt` is the time
      | from the last update in seconds.
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
  * - onInterfaceOverride(base)
    - | Called if the current script has an interface and overrides an interface
      | (``base``) of another script.

**Only for global scripts**

.. list-table::
  :widths: 20 80

  * - onNewGame()
    - New game is started
  * - onPlayerAdded(player)
    - Player added to the game world. The argument is a `Game object`.
  * - onActorActive(actor)
    - Actor (NPC or Creature) becomes active.

**Only for local scripts**

.. list-table::
  :widths: 20 80

  * - onActive()
    - | Called when the object becomes active
      | (either a player came to this cell again, or a save was loaded).
  * - onInactive()
    - | Object became inactive. Since it is inactive the handler
      | can not access anything nearby, but it is possible to send
      | an event to global scripts.
  * - onActivated(actor)
    - | Called on an object when an actor activates it. Note that picking
      | up an item is also an activation and works this way: (1) a copy of
      | the item is placed to the actor's inventory, (2) count of
      | the original item is set to zero, (3) and only then onActivated is
      | called on the original item, so self.count is already zero.
  * - onConsume(recordId)
    - Called if `recordId` (e.g. a potion) is consumed.

**Only for local scripts attached to a player**

.. list-table::
  :widths: 20 80

  * - onInputUpdate(dt)
    - | Called every frame (if the game is not paused) right after
      | processing user input. Use it only for latency-critical stuff.
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
    - | `Game control <openmw_input.html##(ACTION)>`_ is pressed.
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
  * - | onConsoleCommand(
      |     mode, command, selectedObject)
    - | User entered `command` in in-game console. Called if either
      | `mode` is not default or `command` starts with prefix `lua`.


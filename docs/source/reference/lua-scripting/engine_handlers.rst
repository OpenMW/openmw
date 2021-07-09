Engine handlers reference
=========================

Engine handler is a function defined by a script, that can be called by the engine.

+---------------------------------------------------------------------------------------------------------+
| **Can be defined by any script**                                                                        |
+----------------------------------+----------------------------------------------------------------------+
| onUpdate(dt)                     | | Called every frame if game not paused. `dt` is the time            |
|                                  | | from the last update in seconds.                                   |
+----------------------------------+----------------------------------------------------------------------+
| onSave() -> data                 | | Called when the game is saving. May be called in inactive          |
|                                  | | state, so it shouldn't use `openmw.nearby`.                        |
+----------------------------------+----------------------------------------------------------------------+
| onLoad(data)                     | | Called on loading with the data previosly returned by              |
|                                  | | onSave. During loading the object is always inactive.              |
+----------------------------------+----------------------------------------------------------------------+
| **Only for global scripts**                                                                             |
+----------------------------------+----------------------------------------------------------------------+
| onNewGame()                      | New game is started                                                  |
+----------------------------------+----------------------------------------------------------------------+
| onPlayerAdded(player)            | Player added to the game world. The argument is a `Game object`.     |
+----------------------------------+----------------------------------------------------------------------+
| onActorActive(actor)             | Actor (NPC or Creature) becomes active.                              |
+----------------------------------+----------------------------------------------------------------------+
| **Only for local scripts**                                                                              |
+----------------------------------+----------------------------------------------------------------------+
| onActive()                       | | Called when the object becomes active (either a player             |
|                                  | | came to this cell again, or a save was loaded).                    |
+----------------------------------+----------------------------------------------------------------------+
| onInactive()                     | | Object became inactive. Since it is inactive the handler           |
|                                  | | can not access anything nearby, but it is possible to send         |
|                                  | | an event to global scripts.                                        |
+----------------------------------+----------------------------------------------------------------------+
| onConsume(recordId)              | | Called if `recordId` (e.g. a potion) is consumed.                  |
+----------------------------------+----------------------------------------------------------------------+
| **Only for local scripts attached to a player**                                                         |
+----------------------------------+----------------------------------------------------------------------+
| onKeyPress(key)                  | | `Key <openmw_core.html##(KeyboardEvent)>`_ pressed. Usage example: |
|                                  | | ``if key.symbol == 'z' and key.withShift then ...``                |
+----------------------------------+----------------------------------------------------------------------+



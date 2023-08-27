Built-in events
===============

.. include:: version.rst

Actor events
------------

Any script can send to any actor (except player, for player will be ignored) events ``StartAIPackage`` and ``RemoveAIPackages``.
The effect is equivalent to calling ``interfaces.AI.startPackage`` or ``interfaces.AI.removePackages`` in a local script on this actor.

Examples:

.. code-block:: Lua

    actor:sendEvent('StartAIPackage', {type='Combat', target=self.object})
    actor:sendEvent('RemoveAIPackages', 'Pursue')

UI events
---------

Every time UI mode is changed built-in scripts send to player the event ``UiModeChanged`` with arguments ``oldMode, ``newMode`` (same as ``I.UI.getMode()``)
and ``arg`` (for example in the mode ``Book`` the argument is the book the player is reading).

.. code-block:: Lua

    eventHandlers = {
        UiModeChanged = function(data)
            print('UiModeChanged from', data.oldMode , 'to', data.newMode, '('..tostring(data.arg)..')')
        end
    }

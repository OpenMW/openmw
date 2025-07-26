AI packages
============

.. include:: version.rst

.. toctree::
    :hidden:
    :glob:

    ai/*

Starting an AI package
----------------------

There are two ways to start AI package:

.. code-block:: Lua

    -- from local script add package to self
    local AI = require('openmw.interfaces').AI
    AI.startPackage(options)

    -- via event to any actor
    actor:sendEvent('StartAIPackage', options)

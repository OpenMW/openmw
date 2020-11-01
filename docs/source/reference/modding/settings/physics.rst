Physics Settings
################

async num threads
-----------------

:Type:		integer
:Range:		>= 0
:Default:	0

Determines how many threads will be spawned to compute physics update in the background (that is, process actors movement). A value of 0 means that the update will be performed in the main thread.
A value greater than 1 requires the Bullet library be compiled with multithreading support. If that's not the case, a warning will be written in ``openmw.log`` and a value of 1 will be used.

lineofsight keep inactive cache
-------------------------------

:Type:		integer
:Range:		>= -1
:Default:	0

The line of sight determines if 2 actors can see each other (without taking into account game mechanics such as invisibility or sneaking). It is used by some scripts (the getLOS function), by the AI (to determine if an actor should start combat or chase an opponent) and for functionnalities such as greetings or turning NPC head toward an object.
This parameters determine for how long a cache of request should be kept warm. It depends on :ref:`async num threads` being > 0, otherwise a value of -1 will be used. If a request is not found in the cache, it is always fulfilled immediately. In case Bullet is compiled without multithreading support, non-cached requests involve blocking the async thread(s), which might hurt performance.
A value of -1 means no caching.
A value of 0 means that for as long as a request is made (after the first one), it will be preemptively "refreshed" in the async thread, without blocking neither the main thread nor the async thread.
Any value > 0 is the number of frames for which the values are kept in cache even if the results was not requested again.
If Bullet is compiled with multithreading support, requests are non blocking, it is better to set this parameter to -1.

defer aabb update
-----------------

:Type:		boolean
:Range:		True/False
:Default:	True

Axis-aligned bounding box (aabb for short) are used by Bullet for collision detection. They should be updated anytime a physical object is modified (for instance moved) for collision detection to be correct.
This parameter control wether the update should be done as soon as the object is modified (the default), which involves blocking the async thread(s), or queue the modifications to update them as a batch before the collision detections. It depends on :ref:`async num threads` being > 0, otherwise it will be disabled.
Disabling this parameter is intended as an aid for debugging collisions detection issues.

Physics Settings
################

async num threads
-----------------

:Type:		integer
:Range:		>= 0
:Default:	1

Determines how many threads will be spawned to compute physics update in the background (that is, process actors movement). A value of 0 means that the update will be performed in the main thread.
A value greater than 1 requires the Bullet library be compiled with multithreading support. If that's not the case, a warning will be written in ``openmw.log`` and a value of 1 will be used.

lineofsight keep inactive cache
-------------------------------

:Type:		integer
:Range:		>= -1
:Default:	0

The line of sight determines if 2 actors can see each other (without taking into account game mechanics such as invisibility or sneaking). It is used by some scripts (the getLOS function), by the AI (to determine if an actor should start combat or chase an opponent) and for functionnalities such as greetings or turning NPC head toward an object.
This parameters determine for how long a cache of request should be kept warm.
A value of 0 means that the cache is kept only for the current frame, that is if a request is done 2 times in the same frame, the second request will be in cache.
Any value > 0 is the number of frames for which the values are kept in cache even if the results was not requested again.
If :ref:`async num threads` is 0, a value of 0 will be used.
If a request is not found in the cache, it is always fulfilled immediately. In case Bullet is compiled without multithreading support, non-cached requests involve blocking the async thread, which might hurt performance.
If Bullet is compiled with multithreading support, requests are non blocking, it is better to set this parameter to 0.

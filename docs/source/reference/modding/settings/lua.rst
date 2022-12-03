Lua Settings
############

lua debug
---------

:Type:		boolean
:Range:		True/False
:Default:	False

Enables debug tracebacks for Lua actions.
It adds significant performance overhead, don't enable if you don't need it.

This setting can only be configured by editing the settings configuration file.

lua num threads
---------------

:Type:		integer
:Range:		0, 1
:Default:	1

The maximum number of threads used for Lua scripts.
If zero, Lua scripts are processed in the main thread.
If one, a separate thread is used.
Values >1 are not yet supported.

This setting can only be configured by editing the settings configuration file.

gc steps per frame
------------------

:Type:		integer
:Range:		>= 0
:Default:	100

Lua garbage collector steps per frame. The higher the value the more time Lua runtime can spend on freeing unused memory.

This setting can only be configured by editing the settings configuration file.


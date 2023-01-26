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

lua profiler
------------

:Type:		boolean
:Range:		True/False
:Default:	True

Enables Lua profiler.

This setting can only be configured by editing the settings configuration file.

small alloc max size
--------------------

:Type:		unsigned 64-bit integer
:Range:		>= 0
:Default:	1024

No ownership tracking for memory allocations below or equal this size (in bytes).
This setting is used only if ``lua profiler = true``.
With the default value (1024) the lua profiler will show almost no memory usage because allocation more than 1KB are rare.
Decrease the value of this setting (e.g. set it to 64) to have better memory tracking by the cost of higher overhead.

This setting can only be configured by editing the settings configuration file.

memory limit
------------

:Type:		unsigned 64-bit integer
:Range:		> 0
:Default:	2147483648 (2GB)

Memory limit for Lua runtime (only if ``lua profiler = true``). If exceeded then only small allocations are allowed.
Small allocations are always allowed, so e.g. Lua console can function.

This setting can only be configured by editing the settings configuration file.

log memory usage
----------------

:Type:		boolean
:Range:		True/False
:Default:	False

Print debug info about memory usage (only if ``lua profiler = true``).

This setting can only be configured by editing the settings configuration file.

instruction limit per call
--------------------------

:Type:		unsigned 64-bit integer
:Range:		> 1000
:Default:	100000000

The maximal number of Lua instructions per function call (only if ``lua profiler = true``).
If exceeded (e.g. because of an infinite loop) the function will be terminated.

This setting can only be configured by editing the settings configuration file.

gc steps per frame
------------------

:Type:		integer
:Range:		>= 0
:Default:	100

Lua garbage collector steps per frame. The higher the value the more time Lua runtime can spend on freeing unused memory.

This setting can only be configured by editing the settings configuration file.


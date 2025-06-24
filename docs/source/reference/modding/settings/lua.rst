Lua Settings
############

.. omw-setting::
   :title: lua debug
   :type: boolean
   :range: true, false
   :default: false

   Enables debug tracebacks for Lua actions.
   Causes significant performance overhead.

.. omw-setting::
   :title: lua num threads
   :type: int
   :range: 0, 1
   :default: 1

   Maximum number of threads used for Lua scripts.
   0 = main thread only, 1 = separate thread.
   Values >1 not supported.

.. omw-setting::
   :title: lua profiler
   :type: boolean
   :range: true, false
   :default: true

   Enables Lua profiler.

.. omw-setting::
   :title: small alloc max size
   :type: int
   :range: ≥ 0
   :default: 1024

   Max size in bytes for allocations without ownership tracking.
   Used only if lua profiler is true.
   Lower values increase memory tracking detail at cost of overhead.

.. omw-setting::
   :title: memory limit
   :type: int
   :range: > 0
   :default: 2147483648

   Memory limit for Lua runtime (if lua profiler is true).
   If exceeded, only small allocations are allowed.

.. omw-setting::
   :title: log memory usage
   :type: boolean
   :range: true, false
   :default: false

   Prints debug info about memory usage (if lua profiler is true).

.. omw-setting::
   :title: instruction limit per call
   :type: int
   :range: > 1000
   :default: 100000000

   Max number of Lua instructions per function call (if lua profiler is true).
   Functions exceeding this limit will be terminated.

.. omw-setting::
   :title: gc steps per frame
   :type: int
   :range: ≥ 0
   :default: 100

   Lua garbage collector steps per frame.
   Higher values allow more memory to be freed per frame.

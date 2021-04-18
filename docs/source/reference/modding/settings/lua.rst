Lua Settings
############

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


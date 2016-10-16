Cells Settings
##############

exterior cell load distance
---------------------------

:Type:		integer
:Range:		>= 1
:Default:	1

This integer setting determines the number of exterior cells adjacent to the character that will be loaded for rendering. Values greater than one may significantly affect loading times when exiting interior spaces or loading additional exterior cells. Caution is advised when increasing this setting.

This setting interacts with viewing distance and field of view settings.

It is generally very wasteful for this value to load geometry than will almost never be visible due to viewing distance and fog. For low frame rate screen shots of scenic vistas, this setting should be set high, and viewing distances adjusted accordingly.

The default value is 1. This value must be greater than or equal to 1. This setting can only be configured by editing the settings configuration file.
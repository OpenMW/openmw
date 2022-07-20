Sound Settings
##############

device
------

:Type:		string
:Range:
:Default:	""

This setting determines which audio device to use. A blank or missing setting means to use the default device,
which should usually be sufficient, but if you need to explicitly specify a device use this setting.

The names of detected devices can be found in the openmw.log file in your configuration directory.

This setting can be configured by editing the settings configuration file, or in the Audio tab of the OpenMW Launcher.

master volume
-------------

:Type:		floating point
:Range:		0.0 (silent) to 1.0 (maximum volume)
:Default:	1.0

This setting controls the overall volume.
The master volume is multiplied with specific volume settings to determine the final volume.

This setting can be changed in game using the Master slider from the Audio panel of the Options menu.

footsteps volume
----------------

:Type:		floating point
:Range:		0.0 (silent) to 1.0 (maximum volume)
:Default:	0.2

This setting controls the volume of footsteps from the character and other actors.

This setting can be changed in game using the Footsteps slider from the Audio panel of the Options menu.

music volume
------------

:Type:		floating point
:Range:		0.0 (silent) to 1.0 (maximum volume)
:Default:	0.5

This setting controls the volume for music tracks.

This setting can be changed in game using the Music slider from the Audio panel of the Options menu.

sfx volume
----------

:Type:		floating point
:Range:		0.0 (silent) to 1.0 (maximum volume)
:Default:	1.0

This setting controls the volume for special sound effects such as combat noises.

This setting can be changed in game using the Effects slider from the Audio panel of the Options menu.

voice volume
------------

:Type:		floating point
:Range:		0.0 (silent) to 1.0 (maximum volume)
:Default:	0.8

This setting controls the volume for spoken dialogue from NPCs.

This setting can be changed in game using the Voice slider from the Audio panel of the Options menu.

buffer cache min
----------------

:Type:		integer
:Range:		> 0
:Default:	14

This setting determines the minimum size of the sound buffer cache in megabytes.
When the cache reaches the size specified by the buffer cache max setting,
old buffers will be unloaded until it's using no more memory than specified by this setting.
This setting must be less than or equal to the buffer cache max setting.

This setting can only be configured by editing the settings configuration file.

buffer cache max
----------------

:Type:		integer
:Range:		> 0
:Default:	16

This setting determines the maximum size of the sound buffer cache in megabytes. When the cache reaches this size,
old buffers will be unloaded until it reaches the size specified by the buffer cache min setting.
This setting must be greater than or equal to the buffer cache min setting.

This setting can only be configured by editing the settings configuration file.

hrtf enable
-----------

:Type:		integer
:Range:		-1, 0, 1
:Default:	-1

This setting determines whether to enable head-related transfer function (HRTF) audio processing.
HRTF audio processing creates the perception of sounds occurring in a three dimensional space when wearing headphones.
Enabling HRTF may also require an OpenAL Soft version greater than 1.17.0,
and possibly some operating system configuration.
A value of 0 disables HRTF processing, while a value of 1 explicitly enables HRTF processing.
The default value is -1, which should enable the feature automatically for most users when possible.
This setting can be configured by editing the settings configuration file, or in the Audio tab of the OpenMW Launcher.

hrtf
----

:Type:		string
:Range:
:Default:	""

This setting specifies which HRTF profile to use when HRTF is enabled. Blank means use the default.
This setting has no effect if HRTF is not enabled based on the hrtf enable setting.
Allowed values for this field are enumerated in openmw.log file is an HRTF enabled audio system is installed.

The default value is empty, which uses the default profile.
This setting can be configured by editing the settings configuration file, or in the Audio tab of the OpenMW Launcher.

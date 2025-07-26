Sound Settings
##############

.. omw-setting::
   :title: device
   :type: string
   :range:
   :default: ""
   :location: :bdg-success:`Launcher > Settings > Audio`

   This setting determines which audio device to use. A blank or missing setting means to use the default device,
   which should usually be sufficient, but if you need to explicitly specify a device use this setting.

   The names of detected devices can be found in the openmw.log file in your configuration directory.

.. omw-setting::
   :title: master volume
   :type: float32
   :range: 0.0 (silent) to 1.0 (maximum volume)
   :default: 1.0
   :location: :bdg-info:`In Game > Options > Audio`

   This setting controls the overall volume.
   The master volume is multiplied with specific volume settings to determine the final volume.

.. omw-setting::
   :title: footsteps volume
   :type: float32
   :range: 0.0 (silent) to 1.0 (maximum volume)
   :default: 0.2
   :location: :bdg-info:`In Game > Options > Audio`

   This setting controls the volume of footsteps from the character and other actors.

.. omw-setting::
   :title: music volume
   :type: float32
   :range: 0.0 (silent) to 1.0 (maximum volume)
   :default: 0.5
   :location: :bdg-info:`In Game > Options > Audio`

   This setting controls the volume for music tracks.

.. omw-setting::
   :title: sfx volume
   :type: float32
   :range: 0.0 (silent) to 1.0 (maximum volume)
   :default: 1.0
   :location: :bdg-info:`In Game > Options > Audio`

   This setting controls the volume for special sound effects such as combat noises.

.. omw-setting::
   :title: voice volume
   :type: float32
   :range: 0.0 (silent) to 1.0 (maximum volume)
   :default: 0.8
   :location: :bdg-info:`In Game > Options > Audio`

   This setting controls the volume for spoken dialogue from NPCs.


.. omw-setting::
   :title: buffer cache min
   :type: int
   :range: > 0
   :default: 14

   This setting determines the minimum size of the sound buffer cache in megabytes.
   When the cache reaches the size specified by the buffer cache max setting,
   old buffers will be unloaded until it's using no more memory than specified by this setting.
   This setting must be less than or equal to the buffer cache max setting.


.. omw-setting::
   :title: buffer cache max
   :type: int
   :range: > 0
   :default: 16

   This setting determines the maximum size of the sound buffer cache in megabytes. When the cache reaches this size,
   old buffers will be unloaded until it reaches the size specified by the buffer cache min setting.
   This setting must be greater than or equal to the buffer cache min setting.


.. omw-setting::
   :title: hrtf enable
   :type: int
   :range: -1, 0, 1
   :default: -1
   :location: :bdg-success:`Launcher > Settings > Audio`

   This setting determines whether to enable head-related transfer function (HRTF) audio processing.
   HRTF audio processing creates the perception of sounds occurring in a three dimensional space when wearing headphones.
   Enabling HRTF may also require an OpenAL Soft version greater than 1.17.0,
   and possibly some operating system configuration.
   A value of 0 disables HRTF processing, while a value of 1 explicitly enables HRTF processing.
   The default value is -1, which should enable the feature automatically for most users when possible.


.. omw-setting::
   :title: hrtf
   :type: string
   :range:
   :default: ""
   :location: :bdg-success:`Launcher > Settings > Audio`

   This setting specifies which HRTF profile to use when HRTF is enabled. Blank means use the default.
   This setting has no effect if HRTF is not enabled based on the hrtf enable setting.
   Allowed values for this field are enumerated in openmw.log file is an HRTF enabled audio system is installed.
   The default value is empty, which uses the default profile.


.. omw-setting::
   :title: camera listener
   :type: boolean
   :range: true, false
   :default: false
   :location: :bdg-success:`Launcher > Settings > Audio`

   When true, uses the camera position and direction for audio instead of the player position.
   This makes audio in third person sound relative to camera instead of the player.
   False is vanilla Morrowind behaviour.

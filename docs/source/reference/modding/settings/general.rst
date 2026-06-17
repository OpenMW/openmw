General
#######

.. omw-setting::
   :title: anisotropy
   :type: int
   :range: 0 to 16
   :default: 4
   :location: :bdg-info:`In Game > Options > Video > Detail Level`

   Set the maximum anisotropic filtering on textures.
   Anisotropic filtering enhances the image quality of textures on surfaces at oblique viewing angles.
   Valid values range from 0 to 16.
   Modern video cards can often perform 8 or 16 anisotropic filtering with minimal performance impact.

   This setting's effect can be seen by finding locations with straight lines
   (striped rugs or Balmora cobblestones) radiating into the distance,
   and adjusting the anisotropy slider.

.. omw-setting::
   :title: screenshot format
   :type: string
   :range: jpg, png, tga
   :default: png
   :location: :bdg-success:`Launcher > Settings > Miscellaneous`

   Specify the format for screenshots taken using the screenshot key (F12 by default).
   This should be the file extension commonly associated with the format.
   Supported formats depend on compilation, but typically include "jpg", "png", and "tga".

.. omw-setting::
   :title: texture mag filter
   :type: string
   :range: nearest, linear
   :default: linear

   Set the texture magnification filter type.

.. omw-setting::
   :title: texture min filter
   :type: string
   :range: nearest, linear
   :default: linear

   Set the texture minification filter type.

.. omw-setting::
   :title: texture mipmap
   :type: string
   :range: none, nearest, linear
   :default: nearest

   Set the texture mipmap type to control the method mipmaps are created.
   Mipmapping reduces processing power needed during minification by pre-generating a series of smaller textures.

.. omw-setting::
   :title: notify on saved screenshot
   :type: boolean
   :range: true, false
   :default: false
   :location: :bdg-success:`Launcher > Settings > Miscellaneous`

   Show a message box when a screenshot is saved to a file.

.. omw-setting::
   :title: preferred locales
   :type: string
   :default: en
   :location: :bdg-info:`In Game > Options > Language`

   Comma-separated list of preferred locales (e.g. "de,en").
   Each locale must consist of a two-letter language code and can optionally include a two-letter country code (e.g. "en_US").
   Country codes improve accuracy, but partial matches are allowed.

.. omw-setting::
   :title: gmst overrides l10n
   :type: boolean
   :range: true, false
   :default: true
   :location: :bdg-info:`In Game > Options > Language`

   If true, localization GMSTs in content take priority over l10n files.
   Set to false if your preferred locale does not match the content language.

.. omw-setting::
   :title: log buffer size
   :type: int
   :range: ≥ 0
   :default: 65536
   
   Buffer size for the in-game log viewer (F10).
   If the log exceeds the buffer size, only the end will be visible.
   Setting this to zero disables the log viewer.

.. omw-setting::
   :title: console history buffer size
   :type: int
   :range: ≥ 0
   :default: 4096


   Number of console history entries retrieved from the previous session.
   Older entries are discarded when the file exceeds this value.
   See :doc:`../paths` for the location of the history file.

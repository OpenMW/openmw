Stereo Settings
###############

.. omw-setting::
   :title: stereo enabled
   :type: boolean
   :range: true, false
   :default: false

   Enable/disable stereo view. This setting is ignored in VR.

.. omw-setting::
   :title: multiview
   :type: boolean
   :range: true, false
   :default: false

   If enabled, OpenMW will use the :code:`GL_OVR_MultiView` and :code:`GL_OVR_MultiView2` extensions where possible.

.. omw-setting::
   :title: shared shadow maps
   :type: boolean
   :range: true, false
   :default: true

   Use one set of shadow maps for both eyes.
   Will likely be significantly faster than the brute-force approach of rendering a separate copy for each eye with no or imperceptible quality loss.

.. omw-setting::
   :title: allow display lists for multiview
   :type: boolean
   :range: true, false
   :default: true

   If false, OpenMW-VR will disable display lists when using multiview. Necessary on some buggy drivers, but may incur a slight performance penalty.

.. omw-setting::
   :title: use custom view
   :type: boolean
   :range: true, false
   :default: false

   If false, the default OSG horizontal split will be used for stereo.
   If true, the config defined in the :ref:`[Stereo View]<Stereo View Settings>` settings category will be used.

   .. note::
      This option is ignored in VR, and exists primarily for debugging purposes

.. omw-setting::
   :title: use custom eye resolution
   :type: boolean
   :range: true, false
   :default: false

   If true, overrides rendering resolution for each eye.

   .. note::
      This option is ignored in VR, and exists primarily for debugging purposes

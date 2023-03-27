Stereo Settings
###############

stereo enabled
--------------

:Type:		boolean
:Range:		True/False
:Default:	False

Enable/disable stereo view. This setting is ignored in VR.

multiview
---------

:Type:		boolean
:Range:		True/False
:Default:	False

If enabled, OpenMW will use the :code:`GL_OVR_MultiView` and :code:`GL_OVR_MultiView2` extensions where possible.

shared shadow maps
------------------

:Type:		boolean
:Range:		True/False
:Default:	True

Use one set of shadow maps for both eyes.
Will likely be significantly faster than the brute-force approach of rendering a separate copy for each eye with no or imperceptible quality loss.

allow display lists for multiview
---------------------------------

:Type:		boolean
:Range:		True/False
:Default:	True

If false, OpenMW-VR will disable display lists when using multiview. Necessary on some buggy drivers, but may incur a slight performance penalty.

use custom view
---------------

:Type:		boolean
:Range:		True/False
:Default:	False

If false, the default OSG horizontal split will be used for stereo.
If true, the config defined in the :ref:`[Stereo View]<Stereo View Settings>` settings category will be used.

.. note::
	This option is ignored in VR, and exists primarily for debugging purposes

use custom eye resolution
-------------------------

:Type:		boolean
:Range:		True/False
:Default:	False

If true, overrides rendering resolution for each eye.

.. note::
	This option is ignored in VR, and exists primarily for debugging purposes

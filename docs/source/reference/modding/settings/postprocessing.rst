Post Processing Settings
########################

enabled
-------

:Type:		boolean
:Range:		True/False
:Default:	False

Enable or disable post processing.
This enables use of post processing shaders, which must be installed.

chain
-----

:Type:		string list

Controls which post process effects are active and their order.
It is recommended to configure the settings and order of shaders through the in game HUD. By default this is available with the F2 key.
Note, an empty chain will not disable post processing.

This setting has no effect if :ref:`enabled` is set to false.

live reload
-----------

:Type:		boolean
:Range:		True/False
:Default:	False

Automatically reloads a shader if the file has been changed. This is useful for debugging and writing shaders yourself.

.. warning::
    This should be disabled for normal gameplay

hdr exposure time
-----------------

:Type:      float
:Range:     0.0 to 1.0
:Default:   0.05

Use for eye adaptation to control speed at which average scene luminance can change from one frame to the next.
Average scene luminance is used in some shader effects for features such as dynamic eye adaptation.
Smaller values will cause slower changes in scene luminance. This is most impactful when the brightness
drastically changes quickly, like when entering a dark cave or exiting an interior and looking into a bright sun.

This settings has no effect when HDR is disabled or :ref:`enabled` is set to false.

transparent postpass
--------------------

:Type:      boolean
:Range:     True/False
:Default:   True

Re-renders transparent objects with alpha-clipping forced with a fixed threshold. This is particularly important with vanilla content, where blended
objects usually have depth writes disabled and massive margins between the geometry and texture alpha.


.. warning::
    This can be quite costly with vanilla assets. For best performance it is recommended to use a mod replacer which
    uses alpha tested foliage and disable this setting. Morrowind Optimizaton Patch is a great option. 
    If you are not using any shaders which utilize the depth buffer this setting should be disabled.

Custom Shader Effects
#####################

OpenMW leverages the `NiStringExtraData` node to inject special shader flags and effects.
This node must have the prefix `omw:data` and have a valid JSON object that follows.

.. note::

    This is a new feature to inject OpenMW-specific shader effects. Only a single
    effect is currently supported. By default, the shader effects will propogate
    to all a node's children. Other propogation modes and effects will come with
    future releases.


Soft Effect
-----------

This effect softens the intersection of alpha-blended planes with other opaque
geometry. This effect is automatically applied to all particle systems, but can
be applied to any mesh or node. This is useful when layering many alpha-blended
planes for various effects like steam over a hotspring or low hanging fog for
dungeons.

To use this feature the :ref:`soft particles` setting must be enabled.
This setting can either be activated in the OpenMW launcher or changed in `settings.cfg`:

::

    [Shaders]
    soft particles = true

Variables.

+---------+--------------------------------------------------------------------------------------------------------+---------+---------+
| Name    | Description                                                                                            | Type    | Default |
+---------+--------------------------------------------------------------------------------------------------------+---------+---------+
| size    | Scaling ratio. Larger values will make a softer fade effect. Larger geometry requires higher values.   | integer | 45      |
+---------+--------------------------------------------------------------------------------------------------------+---------+---------+
| falloff | Fades away geometry as camera gets closer. Geometry full fades when parallel to camera.                | boolean | false   |
+---------+--------------------------------------------------------------------------------------------------------+---------+---------+

Example usage.

::

    omw:data {
        "shader" : {
            "soft_effect" : {
                "size": 250,
                "falloff" : false,
            }
        }
    }

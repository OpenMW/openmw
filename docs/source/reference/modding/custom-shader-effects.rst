Custom Shader Effects
#####################

OpenMW leverages the `NiStringExtraData` node to inject special shader flags and effects.
This node must have the prefix `omw:data` and have a valid JSON object that follows.

.. note::

    This is a new feature to inject OpenMW-specific shader effects. By default,
    the shader effects will propagate to all of a node's children.
    Other propagation modes and effects will come with future releases.


Soft Effect
-----------

This effect softens the intersection of alpha-blended planes with other opaque
geometry. This effect is automatically applied to all particle systems, but can
be applied to any mesh or node. This is useful when layering many alpha-blended
planes for various effects like steam over a hotspring or low hanging fog for
dungeons.

To use this feature the :ref:`soft particles` setting must be enabled.
This setting can either be activated in the OpenMW launcher or changed in `settings.cfg`:

.. code-block:: ini
    :caption: settings.cfg

    [Shaders]
    soft particles = true

Variables.

+--------------+--------------------------------------------------------------------------------------------------------+---------+---------+
| Name         | Description                                                                                            | Type    | Default |
+==============+========================================================================================================+=========+=========+
| size         | Scaling ratio. Larger values will make a softer fade effect. Larger geometry requires higher values.   | integer | 45      |
+--------------+--------------------------------------------------------------------------------------------------------+---------+---------+
| falloff      | Fades away geometry as camera gets closer. Geometry full fades when parallel to camera.                | boolean | false   |
+--------------+--------------------------------------------------------------------------------------------------------+---------+---------+
| falloffDepth | The units at which geometry starts to fade.                                                            | float   | 300     |
+--------------+--------------------------------------------------------------------------------------------------------+---------+---------+

Example usage.

::

    omw:data {
        "shader" : {
            "soft_effect" : {
                "size": 250,
                "falloff" : false,
                "falloffDepth": 5,
            }
        }
    }

Distortion
----------

This effect is used to imitate effects such as refraction and heat distortion. A common use case is to assign a normal map to the
diffuse slot to a material and add uv scrolling. The red and green channels of the texture are used to offset the final scene texture.
Blue and alpha channels are ignored.

To use this feature the :ref:`post processing <Post Processing>` setting must be enabled.
This setting can either be activated in the OpenMW launcher, in-game, or changed in `settings.cfg`:

.. code-block:: ini
    :caption: settings.cfg

    [Post Processing]
    enabled = true

Variables.

+---------+--------------------------------------------------------------------------------------------------------+---------+---------+
| Name    | Description                                                                                            | Type    | Default |
+=========+========================================================================================================+=========+=========+
| strength| The strength of the distortion effect. Scales linearly.                                                | float   | 0.1     |
+---------+--------------------------------------------------------------------------------------------------------+---------+---------+

Example usage.

::

    omw:data {
        "shader" : {
            "distortion" : {
                "strength": 0.12,
            }
        }
    }

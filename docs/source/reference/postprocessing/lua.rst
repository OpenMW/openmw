###############
Lua Integration
###############

Overview
########

Every shader that is marked as ``dynamic`` can be controlled through the Lua scripting system. Shaders can be disabled and enabled,
and their uniforms can be controlled via scripts. For details, reference the API documentation :doc:`here<../lua-scripting/openmw_postprocessing>`.

Toggling Shaders With a Keybind
###############################

In this example, we use the desaturation shader created in the previous section and bind the ``x`` key to toggle it on and off.
It is assumed the shader has the filename ``desaturate.omwfx`` in this example.

.. code-block:: none

    uniform_float uDesaturationFactor {
        default = 0.5;
        min = 0.0;
        max = 1.0;
        step = 0.05;
        description = "Desaturation factor. A value of 1.0 is full grayscale.";
    }

    fragment desaturate {
        omw_In vec2 omw_TexCoord;

        void main()
        {
            // fetch scene texture from last shader
            vec4 scene = omw_GetLastShader(omw_TexCoord);

            // desaturate RGB component
            const vec3 luminance = vec3(0.299, 0.587, 0.144);
            float gray = dot(luminance, scene.rgb);

            omw_FragColor = vec4(mix(scene.rgb, vec3(gray), uDesaturationFactor), scene.a);
        }
    }

    technique {
        description = "Desaturates scene";
        version = "1.0";
        author = "Fargoth";
        passes = desaturate;
        dynamic = true;
    }

.. note::
    The ``dynamic`` flag here is very important, otherwise Lua will not be able to interact with this shader.

Next, a script that is attached to the player is needed. The shader is loaded first, then toggled on and off in response to key presses.
Below is a working example to illustrate this.

.. code-block:: Lua

    local input = require('openmw.input')
    local postprocessing = require('openmw.postprocessing')

    local shader = postprocessing.load('desaturate')

    return {
        engineHandlers = {
            onKeyPress = function(key)
                if key.code == input.KEY.X then
                    if shader:isEnabled() then
                        shader:disable()
                    else
                        shader:enable()
                    end
                end
            end
        }
    }

Hiding Shader From the HUD
##########################

If the HUD is opened (default with ``F2``) you will notice it lists all available shaders. If you want your shader to be completely
hidden in this HUD, this can done by adding the ``hidden`` flag to the main technique block.

.. code-block:: none

    technique {
        description = "Desaturates scene";
        version = "1.0";
        author = "Fargoth";
        passes = desaturate;
        flags = hidden;
        dynamic = true;
    }

This flag is usually used when the shader is associated with something special, like special weather, spell, or alcohol effects.

Controlling Uniforms
####################

By default, any uniform you defined will not be exposed to Lua, you must set the ``static`` flag to ``false`` in every uniform block for which you want exposed.
For example, to set the ``uDesaturationFactor`` uniform from a Lua script, we must define it as follows.

.. code-block:: none

    uniform_float uDesaturationFactor {
        default = 0.5;
        min = 0.0;
        max = 1.0;
        step = 0.05;
        description = "Desaturation factor. A value of 1.0 is full grayscale.";
        static = false;
    }

In some player Lua script, this uniform can then be freely set. When a uniform is set to ``static`` it will no longer show up in the HUD.
Here, instead of disabling and enabling the shader we set the factor to ``0`` or ``1``, respectively.

.. code-block:: Lua

    local input = require('openmw.input')
    local postprocessing = require('openmw.postprocessing')

    local shader = postprocessing.load('desaturate')
    local factor = 0

    return {
        engineHandlers = {
            onKeyPress = function(key)
                if key.code == input.KEY.X then
                    if factor == 0 then
                        factor = 1
                    else
                        factor = 0
                    end

                    shader:setFloat('uDesaturationFactor', factor)
                end
            end
        }
    }
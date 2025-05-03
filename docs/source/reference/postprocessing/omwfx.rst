#########################
OMWFX Language Reference
#########################

Overview
########

Shaders are written in a OpenMW specific ``*.omwfx`` format. This is a light
wrapper around GLSL, so a basic understanding of GLSL should be acquired before
attempting to write any shaders. Every shader must be contained within a single
``*.omwfx`` file, ``#include`` directives are currently unsupported.

By default, all shaders only guarantee support of GLSL 120 features. To target a
newer GLSL version, you must specify it in the `technique`_ block properties. If
the specified version is not supported on the target machine, the shader will
not load.

Reserved Keywords
#################

GLSL doesn't support namespaces, instead reserved prefixes are used. Do not
attempt to name anything starting with ``_`` or ``omw``, this will cause
name clashes.


Builtin Samplers
################

+------------------+---------------------------+---------------------------------------------+
| GLSL Type        | Name                      | Description                                 |
+==================+===========================+=============================================+
| sampler2D[Array] |``omw_SamplerLastShader``  | Color output of the last shader             |
+------------------+---------------------------+---------------------------------------------+
| sampler2D[Array] |``omw_SamplerLastPass``    | Color output of the last pass               |
+------------------+---------------------------+---------------------------------------------+
| sampler2D[Array] |``omw_SamplerDepth``       | Non-linear normalized depth                 |
+------------------+---------------------------+---------------------------------------------+
| sampler2D[Array] |``omw_SamplerNormals``     | Normalized world-space normals [0, 1]       |
+------------------+---------------------------+---------------------------------------------+

These are included in a common header in every pass, they do not need to be re-defined.
It is recommended to use the accessor functions to retrieve the sampler value.
OpenMW supports multiview rendering, so these samplers will either be a
``sampler2D`` or ``sampler2DArray``. If you want more control over how you
sample textures, use the ``OMW_MULTIVIEW`` macro to determine the appropriate functions to use.


Builtin Uniforms
################

+-------------+------------------------------+--------------------------------------------------+
| GLSL Type   | Name                         | Description                                      |
+=============+==============================+==================================================+
| mat4        | ``omw.projectionMatrix``     | The camera's projection matrix                   |
+-------------+------------------------------+--------------------------------------------------+
| mat4        | ``omw.invProjectionMatrix``  | The inverse of the camera's projection matrix    |
+-------------+------------------------------+--------------------------------------------------+
| mat4        | ``omw.viewMatrix``           | The camera's view matrix                         |
+-------------+------------------------------+--------------------------------------------------+
| mat4        | ``omw.prevViewMatrix``       | The camera's previous frame view matrix          |
+-------------+------------------------------+--------------------------------------------------+
| mat4        | ``omw.invViewMatrix``        | The inverse of the camera's view matrix          |
+-------------+------------------------------+--------------------------------------------------+
| vec4        | ``omw.eyePos``               | The camera's eye position                        |
+-------------+------------------------------+--------------------------------------------------+
| vec4        | ``omw.eyeVec``               | The normalized camera's eye vector               |
+-------------+------------------------------+--------------------------------------------------+
| vec4        | ``omw.fogColor``             | The RGBA color of fog                            |
+-------------+------------------------------+--------------------------------------------------+
| vec4        | ``omw.ambientColor``         | The RGBA color of scene ambient                  |
+-------------+------------------------------+--------------------------------------------------+
| vec4        | ``omw.skyColor``             | The RGBA color of sky                            |
+-------------+------------------------------+--------------------------------------------------+
| vec4        | ``omw.sunColor``             | The RGBA color of sun                            |
+-------------+------------------------------+--------------------------------------------------+
| vec4        | ``omw.sunPos``               | The normalized sun direction                     |
|             |                              |                                                  |
|             |                              | When the sun is set `omw.sunpos.z` is negated    |
+-------------+------------------------------+--------------------------------------------------+
| vec2        | ``omw.resolution``           | The render target's resolution                   |
+-------------+------------------------------+--------------------------------------------------+
| vec2        | ``omw.rcpResolution``        | Reciprocal of the render target resolution       |
+-------------+------------------------------+--------------------------------------------------+
| float       | ``omw.fogNear``              | The units at which the fog begins to render      |
+-------------+------------------------------+--------------------------------------------------+
| float       | ``omw.fogFar``               | The units at which the fog ends                  |
+-------------+------------------------------+--------------------------------------------------+
| float       | ``omw.near``                 | The camera's near clip                           |
+-------------+------------------------------+--------------------------------------------------+
| float       | ``omw.far``                  | The camera's far clip                            |
+-------------+------------------------------+--------------------------------------------------+
| float       | ``omw.gameHour``             | The game hour in range [0,24)                    |
+-------------+------------------------------+--------------------------------------------------+
| float       | ``omw.sunVis``               | The sun's visibility between [0, 1]              |
|             |                              |                                                  |
|             |                              | Influenced by types of weather                   |
|             |                              |                                                  |
|             |                              | Closer to zero during overcast weathers          |
+-------------+------------------------------+--------------------------------------------------+
| float       | ``omw.waterHeight``          | The water height of current cell                 |
|             |                              |                                                  |
|             |                              | Exterior water level is always rendered at -1.0  |
+-------------+------------------------------+--------------------------------------------------+
| bool        | ``omw.isWaterEnabled``       | True if water is enabled for current cell        |
+-------------+------------------------------+--------------------------------------------------+
| float       | ``omw.simulationTime``       | The time in milliseconds since simulation began  |
+-------------+------------------------------+--------------------------------------------------+
| float       | ``omw.deltaSimulationTime``  | The change in `omw.simulationTime`               |
+-------------+------------------------------+--------------------------------------------------+
| int         | ``omw.frameNumber``          | The current frame number                         |
+-------------+------------------------------+--------------------------------------------------+
| float       | ``omw.windSpeed``            | The current wind speed                           |
+-------------+------------------------------+--------------------------------------------------+
| float       | ``omw.weatherTransition``    | The transition factor between weathers [0, 1]    |
+-------------+------------------------------+--------------------------------------------------+
| int         | ``omw.weatherID``            | The current weather ID                           |
+-------------+------------------------------+--------------------------------------------------+
| int         | ``omw.nextWeatherID``        | The next weather ID                              |
+-------------+------------------------------+--------------------------------------------------+
| bool        | ``omw.isUnderwater``         | True if player is submerged underwater           |
+-------------+------------------------------+--------------------------------------------------+
| bool        | ``omw.isInterior``           | True if player is in an interior                 |
|             |                              |                                                  |
|             |                              | False for interiors that behave like exteriors   |
+-------------+------------------------------+--------------------------------------------------+


Builtin Macros
##############

+-----------------------+-----------------+----------------------------------------------------------------------+
| Macro                 | Definition      | Description                                                          |
+=======================+=================+======================================================================+
|``OMW_REVERSE_Z``      | ``0`` or ``1``  | Whether a reversed depth buffer is in use.                           |
|                       |                 |                                                                      |
|                       |                 | ``0``  Depth sampler will be in range [1, 0]                         |
|                       |                 |                                                                      |
|                       |                 | ``1``  Depth sampler will be in range [0, 1]                         |
+-----------------------+-----------------+----------------------------------------------------------------------+
|``OMW_RADIAL_FOG``     | ``0`` or ``1``  | Whether radial fog is in use.                                        |
|                       |                 |                                                                      |
|                       |                 | ``0``  Fog is linear                                                 |
|                       |                 |                                                                      |
|                       |                 | ``1``  Fog is radial                                                 |
+-----------------------+-----------------+----------------------------------------------------------------------+
|``OMW_EXPONENTIAL_FOG``| ``0`` or ``1``  | Whether exponential fog is in use.                                   |
|                       |                 |                                                                      |
|                       |                 | ``0``  Fog is linear                                                 |
|                       |                 |                                                                      |
|                       |                 | ``1``  Fog is exponential                                            |
+-----------------------+-----------------+----------------------------------------------------------------------+
| ``OMW_HDR``           | ``0`` or ``1``  | Whether average scene luminance is computed every frame.             |
|                       |                 |                                                                      |
|                       |                 | ``0``  Average scene luminance is not computed                       |
|                       |                 |                                                                      |
|                       |                 | ``1``  Average scene luminance is computed                           |
+-----------------------+-----------------+----------------------------------------------------------------------+
|  ``OMW_NORMALS``      | ``0`` or ``1``  | Whether normals are available as a sampler in the technique.         |
|                       |                 |                                                                      |
|                       |                 | ``0``  Normals are not available                                     |
|                       |                 |                                                                      |
|                       |                 | ``1``  Normals are available.                                        |
+-----------------------+-----------------+----------------------------------------------------------------------+
| ``OMW_MULTIVIEW``     | ``0`` or ``1``  | Whether multiview rendering is in use.                               |
|                       |                 |                                                                      |
|                       |                 | ``0``  Multiview not in use                                          |
|                       |                 |                                                                      |
|                       |                 | ``1``  Multiview in use.                                             |
+-----------------------+-----------------+----------------------------------------------------------------------+
| ``OMW_API_VERSION``   | |ppApiRevision| | The revision of OpenMW postprocessing API.                           |
|                       |                 | It is an integer that is incremented every time the API is changed.  |
|                       |                 | This was added in 0.49, so it will be undefined in 0.48.             |
+-----------------------+-----------------+----------------------------------------------------------------------+


Builtin Functions
#################

The following functions can be accessed in any fragment or vertex shader.

+--------------------------------------------------+-------------------------------------------------------------------------------+
| Function                                         | Description                                                                   |
+==================================================+===============================================================================+
| ``float omw_GetDepth(vec2)``                     | Returns the depth value from a sampler given a uv coordinate.                 |
|                                                  |                                                                               |
|                                                  | Reverses sampled value when ``OMW_REVERSE_Z`` is set.                         |
+--------------------------------------------------+-------------------------------------------------------------------------------+
| ``float omw_GetEyeAdaptation()``                 | Returns the average scene luminance in range [0, 1].                          |
|                                                  |                                                                               |
|                                                  | If HDR is not in use, this returns `1.0`                                      |
|                                                  |                                                                               |
|                                                  | Scene luminance is always calculated on original scene texture.               |
+--------------------------------------------------+-------------------------------------------------------------------------------+
| ``vec4 omw_GetLastShader(vec2 uv)``              | Returns RGBA color output of the last shader                                  |
+--------------------------------------------------+-------------------------------------------------------------------------------+
| ``vec4 omw_GetLastPass(vec2 uv)``                | Returns RGBA color output of the last pass                                    |
+--------------------------------------------------+-------------------------------------------------------------------------------+
| ``vec3 omw_GetNormals(vec2 uv)``                 | Returns normalized view-space normals [-1, 1]                                 |
+--------------------------------------------------+-------------------------------------------------------------------------------+
| ``vec3 omw_GetNormalsWorldSpace(vec2 uv)``       | Returns normalized world-space normals [-1, 1]                                |
+--------------------------------------------------+-------------------------------------------------------------------------------+
| ``vec3 omw_GetWorldPosFromUV(vec2 uv)``          | Returns world position for given uv coordinate.                               |
+--------------------------------------------------+-------------------------------------------------------------------------------+
| ``float omw_GetLinearDepth(vec2 uv)``            | Returns the depth in game units for given uv coordinate.                      |
+--------------------------------------------------+-------------------------------------------------------------------------------+
| ``float omw_EstimateFogCoverageFromUV(vec2 uv)`` | Returns a fog coverage in the range from 0.0 (no fog) and 1.0 (full fog)      |
|                                                  |                                                                               |
|                                                  | Calculates an estimated fog coverage for given uv coordinate.                 |
+--------------------------------------------------+-------------------------------------------------------------------------------+
| ``int omw_GetPointLightCount()``                 | Returns the number of point lights available to sample from in the scene.     |
+--------------------------------------------------+-------------------------------------------------------------------------------+
| ``vec3 omw_GetPointLightWorldPos(int index)``    | Returns the world space position of a point light.                            |
+--------------------------------------------------+-------------------------------------------------------------------------------+
| ``vec3 omw_GetPointLightDiffuse(int index)``     | Returns the diffuse color of the point light.                                 |
+--------------------------------------------------+-------------------------------------------------------------------------------+
| ``int omw_GetPointLightAttenuation(int index)``  | Returns the attenuation values of the point light.                            |
|                                                  |                                                                               |
|                                                  | The XYZ channels hold the constant, linear, and quadratic components.         |
+--------------------------------------------------+-------------------------------------------------------------------------------+
| ``float omw_GetPointLightRadius(int index)``     | Returns the radius of the point light, in game units.                         |
+--------------------------------------------------+-------------------------------------------------------------------------------+

Special Attributes
##################

To maintain maximum compatibility with future releases, OpenMW defines specific keywords, attributes, and functions for you to use. These should be used instead of their
GLSL equivalent. Refer to the table below to view these mappings.

+-------------------+---------------------------------------------------------+
| .omwfx            | Description                                             |
+===================+=========================================================+
| omw_In            |  use in place of ``in`` and ``varying``                 |
+-------------------+---------------------------------------------------------+
| omw_Out           |  use in place of ``out`` and ``varying``                |
+-------------------+---------------------------------------------------------+
| omw_Position      |  use in place of ``gl_Position``                        |
+-------------------+---------------------------------------------------------+
| omw_Vertex        |  use in place of ``gl_Vertex``                          |
+-------------------+---------------------------------------------------------+
| omw_Fragment      |  use in place of ``gl_FragData[*]`` and ``gl_FragColor``|
+-------------------+---------------------------------------------------------+
| omw_Texture1D()   |  use in place of ``texture1D()`` or ``texture()``       |
+-------------------+---------------------------------------------------------+
| omw_Texture2D()   |  use in place of ``texture2D()`` or ``texture()``       |
+-------------------+---------------------------------------------------------+
| omw_Texture3D()   |  use in place of ``texture3D()`` or ``texture()``       |
+-------------------+---------------------------------------------------------+

Blocks
######

``fragment``
*************

Declare your passes with ``fragment`` followed by a unique name. We will define the order of these passes later on.
Each ``fragment`` block must contain valid GLSL. Below is a simple example of defining two passes.

.. code-block:: none

    fragment pass {
        void main()
        {
            omw_FragColor = vec4(1.0);
        }
    }

    fragment otherPass {

        omw_In vec2 omw_TexCoord;

        void main()
        {
            omw_FragColor = omw_GetLastPass(omw_TexCoord);
        }
    }

``vertex``
***********

For every ``fragment`` block you declare, OpenMW generates a default vertex shader if you do not define one. This is used to draw the fullscreen triangle used in postprocessing.
This means you rarely need to use a custom vertex shader. Using a vertex shader can sometimes be useful when you need to do lots of complicated calculations that don't rely on pixel location.
The vertex shader only invocates on the `3` vertices of the fullscreen triangle.
Below is an example of passing a value through a custom vertex shader to the fragment shader.

.. code-block:: none

    vertex pass {
        #if OMW_USE_BINDINGS
            omw_In vec2 omw_Vertex;
        #endif

        uniform sampler2D noiseSampler;

        omw_Out vec2 omw_TexCoord;

        // custom output from vertex shader
        omw_Out float noise;

        void main()
        {
            omw_Position = vec4(omw_Vertex.xy, 0.0, 1.0);
            omw_TexCoord = omw_Position.xy * 0.5 + 0.5;

            noise = sqrt(omw_Texture2D(noiseSampler, vec2(0.5, 0.5)).r);
        }
    }

    fragment pass {
        omw_In vec2 omw_TexCoord;

        // our custom output from the vertex shader is available
        omw_In float noise;

        void main()
        {
            omw_FragColor = vec4(1.0);
        }
    }


``technique``
*************

Exactly one ``technique`` block is required for every shader file. In this we define important traits like author, description, requirements, and flags.


+------------------+--------------------+---------------------------------------------------+
| Property         | Type               | Description                                       |
+==================+====================+===================================================+
| passes           | literal list       | ``,`` separated list of pass names                |
+------------------+--------------------+---------------------------------------------------+
| version          | string             | Shader version that shows in HUD                  |
+------------------+--------------------+---------------------------------------------------+
| description      | string             | Shader description that shows in HUD              |
+------------------+--------------------+---------------------------------------------------+
| author           | string             | Shader authors that shows in HUD                  |
+------------------+--------------------+---------------------------------------------------+
| glsl_version     | integer            | GLSL version                                      |
+------------------+--------------------+---------------------------------------------------+
| glsl_profile     | string             | GLSL profile, like ``compatibility``              |
+------------------+--------------------+---------------------------------------------------+
| glsl_extensions  | literal list       | ``,`` separated list of required GLSL extensions  |
+------------------+--------------------+---------------------------------------------------+
| hdr              | boolean            | Whether HDR eye adaptation is required.           |
+------------------+--------------------+---------------------------------------------------+
| pass_normals     | boolean            | Pass normals from the forward passes.             |
|                  |                    |                                                   |
|                  |                    | If unsupported, `OMW_NORMALS` will be set to `0`  |
+------------------+--------------------+---------------------------------------------------+
| flags            | `SHADER_FLAG`_     | ``,`` separated list of shader flags              |
+------------------+--------------------+---------------------------------------------------+
| dynamic          | boolean            | Whether shader is exposed to Lua                  |
+------------------+--------------------+---------------------------------------------------+

When ``dynamic`` is set to ``true``, the shaders order cannot be manually moved, enabled, or disabled. The shaders state
can only be controlled via a Lua script.

In the code snippet below, a shader is defined that requires GLSL `330`, HDR capatiblities, and is only enabled underwater in exteriors.

.. code-block:: none

    fragment dummy {
        void main()
        {
            omw_FragColor = vec4(0.0);
        }
    }

    technique {
        passes = dummy;
        glsl_version = 330;
        hdr = true;
        flags = disable_interiors, disable_abovewater;
    }


``sampler_*``
*************

Any texture in the VFS can be loaded by a shader. All passes within the technique will have access to this texture as a sampler.
OpenMW currently supports ``1D``, ``2D``, and ``3D`` texture samplers, cubemaps can not yet be loaded.

+-------------+
| Block       |
+=============+
| sampler_1d  |
+-------------+
| sampler_2d  |
+-------------+
| sampler_3d  |
+-------------+

.. warning::
    OpenMW vertically flips all DDS textures when loading them, with the exception of ``3D`` textures.


The properties for a ``sampler_*`` block are as following.
The only required property for a texture is its ``source``.

+-----------------------+-----------------------+
| Property              | Type                  |
+=======================+=======================+
|``source``             |  string               |
+-----------------------+-----------------------+
|``min_filter``         | `FILTER_MODE`_        |
+-----------------------+-----------------------+
|``mag_filter``         | `FILTER_MODE`_        |
+-----------------------+-----------------------+
|``wrap_s``             | `WRAP_MODE`_          |
+-----------------------+-----------------------+
|``wrap_t``             | `WRAP_MODE`_          |
+-----------------------+-----------------------+
|``wrap_r``             | `WRAP_MODE`_          |
+-----------------------+-----------------------+
|``compression``        | `COMPRESSION_MODE`_   |
+-----------------------+-----------------------+
|``source_format``      | `SOURCE_FORMAT`_      |
+-----------------------+-----------------------+
|``source_type``        | `SOURCE_TYPE`_        |
+-----------------------+-----------------------+
|``internal_format``    | `INTERNAL_FORMAT`_    |
+-----------------------+-----------------------+

In the code snippet below, a simple noise texture is loaded with nearest filtering.

.. code-block:: none

    sampler_2d noise {
        source = "textures/noise.png";
        mag_filter = nearest;
        min_filter = nearest;
    }

To use the sampler, define the appropriately named `sampler2D` in any of your passes.

.. code-block:: none

    fragment pass {
        omw_In vec2 omw_TexCoord;

        uniform sampler2D noise;

        void main()
        {
            // ...
            vec4 noise = omw_Texture2D(noise, omw_TexCoord);
        }
    }

``uniform_*``
**************

It is possible to define settings for your shaders that can be adjusted by either users or a Lua script.


+-----------------+----------+----------+----------+---------+----------+--------------+-------------------+---------+--------------+
| Block           | default  | min      | max      | static  | step     | description  |  display_name     | header  |  widget_type |
+=================+==========+==========+==========+=========+==========+==============+===================+=========+==============+
|``uniform_bool`` | boolean  | x        | x        | boolean | x        | string       |  string           | string  |  choice(...) |
+-----------------+----------+----------+----------+---------+----------+--------------+-------------------+---------+--------------+
|``uniform_float``| float    | float    | float    | boolean | float    | string       |  string           | string  |  choice(...) |
+-----------------+----------+----------+----------+---------+----------+--------------+-------------------+---------+--------------+
|``uniform_int``  | integer  | integer  | integer  | boolean | integer  | string       |  string           | string  |  choice(...) |
+-----------------+----------+----------+----------+---------+----------+--------------+-------------------+---------+--------------+
|``uniform_vec2`` | vec2     | vec2     | vec2     | boolean | vec2     | string       |  string           | string  |  choice(...) |
+-----------------+----------+----------+----------+---------+----------+--------------+-------------------+---------+--------------+
|``uniform_vec3`` | vec3     | vec3     | vec3     | boolean | vec3     | string       |  string           | string  |  choice(...) |
+-----------------+----------+----------+----------+---------+----------+--------------+-------------------+---------+--------------+
|``uniform_vec4`` | vec4     | vec4     | vec4     | boolean | vec4     | string       |  string           | string  |  choice(...) |
+-----------------+----------+----------+----------+---------+----------+--------------+-------------------+---------+--------------+

The ``description`` field is used to display a toolip when viewed in the in-game HUD. The ``header`` field
field can be used to organize uniforms into groups in the HUD. The ``display_name`` field can be used to create a
more user friendly uniform name for display in the HUD.

If you would like a uniform to be adjustable with Lua API you `must` set ``static = false;``. Doing this
will also remove the uniform from the players HUD.

Below is an example of declaring a ``vec3`` uniform.

.. code-block:: none

    uniform_vec3 uColor {
        default = vec3(0,1,1);
        min = vec3(0,0,0);
        max = vec3(1,1,1);
        step = vec3(0.1, 0.1, 0.1);
        description = "Color uniform";
        static = true;
        header = "Colors";
    }

To use the uniform you can reference it in any pass, it should **not** be declared with the ``uniform`` keyword.

.. code-block:: none

    fragment pass {
        void main()
        {
            // ...
            vec3 color = uColor;
        }
    }

You can use uniform arrays as well, but they are restricted to the `Lua API <../lua-scripting/openmw_postprocessing.html>`_ scripts.
These uniform blocks must be defined with the new ``size`` parameter.

.. code-block:: none

    uniform_vec3 uArray {
        size = 10;
    }

You may also define a dropdown list for users to select specific values from instead of the default sliders using the ``widget_type`` field.
Each item in the dropdown has an associated display name, which can be a localized string.

.. code-block:: none

    uniform_int uStrength {
        default = 2;
        display_name = "Strength";
        widget_type = choice(
            "Low" = 1,
            "Medium" = 2,
            "High" = 3
        );
    }

``render_target``
*****************

Normally when defining passes, OpenMW will take care of setting up all of the render targets. Sometimes, this behavior
is not wanted and you want a custom render target.


+------------------+---------------------+-----------------------------------------------------------------------------+
| Property         | Type                | Description                                                                 |
+==================+=====================+=============================================================================+
| min_filter       | `FILTER_MODE`_      | x                                                                           |
+------------------+---------------------+-----------------------------------------------------------------------------+
| mag_filter       | `FILTER_MODE`_      | x                                                                           |
+------------------+---------------------+-----------------------------------------------------------------------------+
| wrap_s           | `WRAP_MODE`_        | x                                                                           |
+------------------+---------------------+-----------------------------------------------------------------------------+
| wrap_t           | `WRAP_MODE`_        | x                                                                           |
+------------------+---------------------+-----------------------------------------------------------------------------+
| internal_format  | `INTERNAL_FORMAT`_  | x                                                                           |
+------------------+---------------------+-----------------------------------------------------------------------------+
| source_type      | `SOURCE_TYPE`_      | x                                                                           |
+------------------+---------------------+-----------------------------------------------------------------------------+
| source_format    | `SOURCE_FORMAT`_    | x                                                                           |
+------------------+---------------------+-----------------------------------------------------------------------------+
| width_ratio      | float               | Automatic width as a percentage of screen width                             |
+------------------+---------------------+-----------------------------------------------------------------------------+
| height_ratio     | float               | Automatic height as a percentage of screen height                           |
+------------------+---------------------+-----------------------------------------------------------------------------+
| width            | float               | Width in pixels                                                             |
+------------------+---------------------+-----------------------------------------------------------------------------+
| height           | float               | Height in pixels                                                            |
+------------------+---------------------+-----------------------------------------------------------------------------+
| mipmaps          | boolean             | Whether mipmaps should be generated every frame                             |
+------------------+---------------------+-----------------------------------------------------------------------------+
| clear_color      | vec4                | The color the texture will be cleared to when it's first created            |
+------------------+---------------------+-----------------------------------------------------------------------------+

To use the render target a pass must be assigned to it, along with any optional blend modes.
As a restriction, only three render targets can be bound per pass with ``rt1``, ``rt2``, ``rt3``, respectively.

Blending modes can be useful at times. Below is a simple shader which, when activated, will slowly turn the screen pure red.
Notice how we only ever write the value `.01` to the `RT_Red` buffer. Since we're using appropriate blending modes the
color buffer will accumulate.

.. code-block:: none

    render_target RT_Red {
        width = 4;
        height = 4;
        source_format = rgb;
        internal_format = rgb16f;
        source_type = half_float;
        clear_color = vec4(0,0,0,1);
    }

    fragment red(target=RT_Red,blend=(add, src_color, one), rt1=RT_Red) {
        omw_In vec2 omw_TexCoord;

        void main()
        {
            omw_FragColor.rgb = vec3(0.01,0,0);
        }
    }

    fragment view(rt1=RT_Red) {
        omw_In vec2 omw_TexCoord;

        void main()
        {
            omw_FragColor = omw_Texture2D(RT_Red, omw_TexCoord);
        }
    }

    technique {
        author = "OpenMW";
        passes = red, view;
    }


These custom render targets are persistent and ownership is given to the shader which defines them.
This gives potential to implement temporal effects by storing previous frame data in these buffers.
Below is an example which calculates a naive average scene luminance and transitions between values smoothly.

.. code-block:: none

    render_target RT_Lum {
        width = 256;
        height = 256;
        mipmaps = true;
        source_format = rgb;
        internal_format = rgb16f;
        source_type = half_float;
        min_filter = linear_mipmap_linear;
        mag_filter = linear;
    }

    render_target RT_LumAvg {
        source_type = half_float;
        source_format = rgb;
        internal_format = rgb16f;
        min_filter = nearest;
        mag_filter = nearest;
    }

    render_target RT_LumAvgLastFrame {
        source_type = half_float;
        source_format = rgb;
        internal_format = rgb16f;
        min_filter = nearest;
        mag_filter = nearest;
    }

    fragment calculateLum(target=RT_Lum) {
        omw_In vec2 omw_TexCoord;

        void main()
        {
            vec3 orgi = pow(omw_GetLastShader(omw_TexCoord), vec4(2.2)).rgb;
            omw_FragColor.rgb = orgi;
        }
    }

    fragment fetchLumAvg(target=RT_LumAvg, rt1=RT_Lum, rt2=RT_LumAvgLastFrame) {
        omw_In vec2 omw_TexCoord;

        void main()
        {
            vec3 avgLumaCurrFrame = textureLod(RT_Lum, vec2(0.5, 0.5), 6).rgb;
            vec3 avgLumaLastFrame = omw_Texture2D(RT_LumAvgLastFrame, vec2(0.5, 0.5)).rgb;

            const float speed = 0.9;

            vec3 avgLuma = avgLumaLastFrame + (avgLumaCurrFrame - avgLumaLastFrame) * (1.0 - exp(-omw.deltaSimulationTime * speed));

            omw_FragColor.rgb = avgLuma;
        }
    }

    fragment adaptation(rt1=RT_LumAvg) {
        omw_In vec2 omw_TexCoord;

        void main()
        {
            vec3 avgLuma = omw_Texture2D(RT_LumAvg, vec2(0.5, 0.5)).rgb;

            if (omw_TexCoord.y < 0.2)
                omw_FragColor = vec4(avgLuma, 1.0);
            else
                omw_FragColor = omw_GetLastShader(omw_TexCoord);
        }
    }

    fragment store(target=RT_LumAvgLastFrame, rt1=RT_LumAvg) {
        void main()
        {
            vec3 avgLuma = omw_Texture2D(RT_LumAvg, vec2(0.5, 0.5)).rgb;
            omw_FragColor.rgb = avgLuma;
        }
    }

    technique {
        author = "OpenMW";
        passes = calculateLum, fetchLumAvg, store, adaptation;
        glsl_version = 330;
    }


Simple Example
##############

Let us go through a simple example in which we apply a simple desaturation
filter with a user-configurable factor.

Our first step is defining our user-configurable variable. In this case all we
want is a normalized value between 0 and 1 to influence the amount of
desaturation to apply to the scene. Here we setup a new variable of type
``float``, define a few basic properties, and give it a tooltip description.

.. code-block:: none

    uniform_float uDesaturationFactor {
        default = 0.5;
        min = 0.0;
        max = 1.0;
        step = 0.05;
        static = true;
        display_name = "Desaturation Factor";
        description = "Desaturation factor. A value of 1.0 is full grayscale.";
    }

Now, we can setup our first pass. Remember a pass is just a pixel shader invocation.

.. code-block:: none

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

Next we can define our ``technique`` block, which is in charge of glueing
together passes, setting up metadata, and setting up various flags.

.. code-block:: none

    technique {
        description = "Desaturates scene";
        passes = desaturate;
        version = "1.0";
        author = "Fargoth";
    }


Putting it all together we have this simple shader.

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
        passes = desaturate;
        version = "1.0";
        author = "Fargoth";
        passes = desaturate;
    }


Types
#####

`SHADER_FLAG`
*************

+--------------------+--------------------------------------------------------------------------+
| Flag               | Description                                                              |
+====================+==========================================================================+
| disable_interiors  | Disable in interiors.                                                    |
+--------------------+--------------------------------------------------------------------------+
| disable_exteriors  | Disable when in exteriors or interiors which behave like exteriors.      |
+--------------------+--------------------------------------------------------------------------+
| disable_underwater | Disable when underwater.                                                 |
+--------------------+--------------------------------------------------------------------------+
| disable_abovewater | Disable when above water.                                                |
+--------------------+--------------------------------------------------------------------------+
| disable_sunglare   | Disables builtin sunglare.                                               |
+--------------------+--------------------------------------------------------------------------+
| hidden             | Shader does not show in the HUD. Useful for shaders driven by Lua API.   |
+--------------------+--------------------------------------------------------------------------+

`BLEND_EQ`
**********

+-------------------+---------------------------+
| .omwfx            | OpenGL                    |
+===================+===========================+
| rgba_min          | GL_MIN                    |
+-------------------+---------------------------+
| rgba_max          | GL_MAX                    |
+-------------------+---------------------------+
| alpha_min         | GL_ALPHA_MIN_SGIX         |
+-------------------+---------------------------+
| alpha_max         | GL_ALPHA_MAX_SGIX         |
+-------------------+---------------------------+
| logic_op          | GL_LOGIC_OP               |
+-------------------+---------------------------+
| add               | GL_FUNC_ADD               |
+-------------------+---------------------------+
| subtract          | GL_FUNC_SUBTRACT          |
+-------------------+---------------------------+
| reverse_subtract  | GL_FUNC_REVERSE_SUBTRACT  |
+-------------------+---------------------------+

`BLEND_FUNC`
************

+---------------------------+------------------------------+
| .omwfx                    | OpenGL                       |
+===========================+==============================+
| dst_alpha                 | GL_DST_ALPHA                 |
+---------------------------+------------------------------+
| dst_color                 | GL_DST_COLOR                 |
+---------------------------+------------------------------+
| one                       | GL_ONE                       |
+---------------------------+------------------------------+
| one_minus_dst_alpha       | GL_ONE_MINUS_DST_ALPHA       |
+---------------------------+------------------------------+
| one_minus_dst_color       | GL_ONE_MINUS_DST_COLOR       |
+---------------------------+------------------------------+
| one_minus_src_alpha       | GL_ONE_MINUS_SRC_ALPHA       |
+---------------------------+------------------------------+
| one_minus_src_color       | GL_ONE_MINUS_SRC_COLOR       |
+---------------------------+------------------------------+
| src_alpha                 | GL_SRC_ALPHA                 |
+---------------------------+------------------------------+
| src_alpha_saturate        | GL_SRC_ALPHA_SATURATE        |
+---------------------------+------------------------------+
| src_color                 | GL_SRC_COLOR                 |
+---------------------------+------------------------------+
| constant_color            | GL_CONSTANT_COLOR            |
+---------------------------+------------------------------+
| one_minus_constant_color  | GL_ONE_MINUS_CONSTANT_COLOR  |
+---------------------------+------------------------------+
| constant_alpha            | GL_CONSTANT_ALPHA            |
+---------------------------+------------------------------+
| one_minus_constant_alpha  | GL_ONE_MINUS_CONSTANT_ALPHA  |
+---------------------------+------------------------------+
| zero                      | GL_ZERO                      |
+---------------------------+------------------------------+

`INTERNAL_FORMAT`
*****************

+--------------------+-----------------------+
| .omwfx             | OpenGL                |
+====================+=======================+
| red                | GL_RED                |
+--------------------+-----------------------+
| r16f               | GL_R16F               |
+--------------------+-----------------------+
| r32f               | GL_R32F               |
+--------------------+-----------------------+
| rg                 | GL_RG                 |
+--------------------+-----------------------+
| rg16f              | GL_RG16F              |
+--------------------+-----------------------+
| rg32f              | GL_RG32F              |
+--------------------+-----------------------+
| rgb                | GL_RGB                |
+--------------------+-----------------------+
| rgb16f             | GL_RGB16F             |
+--------------------+-----------------------+
| rgb32f             | GL_RGB32F             |
+--------------------+-----------------------+
| rgba               | GL_RGBA               |
+--------------------+-----------------------+
| rgba16f            | GL_RGBA16F            |
+--------------------+-----------------------+
| rgba32f            | GL_RGBA32F            |
+--------------------+-----------------------+
| depth_component16  | GL_DEPTH_COMPONENT16  |
+--------------------+-----------------------+
| depth_component24  | GL_DEPTH_COMPONENT24  |
+--------------------+-----------------------+
| depth_component32  | GL_DEPTH_COMPONENT32  |
+--------------------+-----------------------+
| depth_component32f | GL_DEPTH_COMPONENT32F |
+--------------------+-----------------------+

`SOURCE_TYPE`
*************

+--------------------+-----------------------+
| .omwfx             | OpenGL                |
+====================+=======================+
| byte               | GL_BYTE               |
+--------------------+-----------------------+
| unsigned_byte      | GL_UNSIGNED_BYTE      |
+--------------------+-----------------------+
| short              | GL_SHORT              |
+--------------------+-----------------------+
| unsigned_short     | GL_UNSIGNED_SHORT     |
+--------------------+-----------------------+
| int                | GL_INT                |
+--------------------+-----------------------+
| unsigned_int       | GL_UNSIGNED_INT       |
+--------------------+-----------------------+
| unsigned_int_24_8  | GL_UNSIGNED_INT_24_8  |
+--------------------+-----------------------+
| float              | GL_FLOAT              |
+--------------------+-----------------------+
| half_float         | GL_HALF_FLOAT         |
+--------------------+-----------------------+
| double             | GL_DOUBLE             |
+--------------------+-----------------------+


`SOURCE_FORMAT`
***************

+---------+---------+
| .omwfx  | OpenGL  |
+=========+=========+
| red     | GL_RED  |
+---------+---------+
| rg      | GL_RG   |
+---------+---------+
| rgb     | GL_RGB  |
+---------+---------+
| bgr     | GL_BGR  |
+---------+---------+
| rgba    | GL_RGBA |
+---------+---------+
| bgra    | GL_BGRA |
+---------+---------+

`FILTER_MODE`
*************

+-------------------------+----------------------------+
| .omwfx                  | OpenGL                     |
+=========================+============================+
| linear                  | GL_LINEAR                  |
+-------------------------+----------------------------+
| linear_mipmap_linear    | GL_LINEAR_MIPMAP_LINEAR    |
+-------------------------+----------------------------+
| linear_mipmap_nearest   | GL_LINEAR_MIPMAP_NEAREST   |
+-------------------------+----------------------------+
| nearest                 | GL_NEAREST                 |
+-------------------------+----------------------------+
| nearest_mipmap_linear   | GL_NEAREST_MIPMAP_LINEAR   |
+-------------------------+----------------------------+
| nearest_mipmap_nearest  | GL_NEAREST_MIPMAP_NEAREST  |
+-------------------------+----------------------------+

`WRAP_MODE`
***********

+------------------+---------------------+
| .omwfx           | OpenGL              |
+==================+=====================+
| clamp_to_edge    | GL_CLAMP_TO_EDGE    |
+------------------+---------------------+
| clamp_to_border  | GL_CLAMP_TO_BORDER  |
+------------------+---------------------+
| repeat           | GL_REPEAT           |
+------------------+---------------------+
| mirror           | GL_MIRRORED_REPEAT  |
+------------------+---------------------+

`COMPRESSION_MODE`
******************

+-------------+
| .omwfx      |
+=============+
| auto        |
+-------------+
| arb         |
+-------------+
| s3tc_dxt1   |
+-------------+
| s3tc_dxt3   |
+-------------+
| s3tc_dxt5   |
+-------------+
| pvrtc_2bpp  |
+-------------+
| pvrtc_4bpp  |
+-------------+
| etc         |
+-------------+
| etc2        |
+-------------+
| rgtc1       |
+-------------+
| rgtc2       |
+-------------+
| s3tc_dxt1c  |
+-------------+
| s3tc_dxt1a  |
+-------------+

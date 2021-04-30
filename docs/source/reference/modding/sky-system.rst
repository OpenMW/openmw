########################
Sky System and Structure
########################

Overview
********

The sky system is made from multiple components that contribute to the final result.

1. Background colour and fog
2. Atmosphere
3. Clouds
4. Stars
5. Sun
6. Moons
7. Weather effects

.. image:: https://gitlab.com/OpenMW/openmw-docs/-/raw/master/docs/source/reference/modding/_static/sky-system-overview.png
    :align: center

Background Colour and Fog
*************************

Even when nothing else is being rendered, OpenMW will always show the basic background colour. The distant fog effect is based off this colour as well. Other elements of the sky are rendered on top of the background to compose the sky. The colour of the background changes depending on the time of day and current weather.

Atmosphere
**********

A mesh that contributes the blue colour of the sky. It is a rough cylinder in shape without the bottom face and with the normals pointing inwards. During the day, it is light blue and transitions to a very dark blue, almost black, during the night. 

Towards the bottom edge the mesh gradually becomes transparent and blends with the background colour. Transparency is done per vertex and OpenMW decides which vertices are transparent based on their index. This adds a requirement for a very strict vertex and face order on this mesh to blend properly.

.. image:: https://gitlab.com/OpenMW/openmw-docs/-/raw/master/docs/source/reference/modding/_static/sky-system-mesh-atmosphere.jpg
    :align: center

Clouds
******

A mesh that renders a tiling, scrolling texture of the clouds. It is a flat dome in shape with the normals pointing inwards. Towards the boundary edge the mesh becomes transparent and blends with the background colour. As with the atmosphere, there is a very strict vertex order on this mesh to blend properly.

.. image:: https://gitlab.com/OpenMW/openmw-docs/-/raw/master/docs/source/reference/modding/_static/sky-system-mesh-clouds.jpg
    :align: center

By default, the UVs of this mesh are scaled so the clouds texture repeats five times across the sky. The weather system blends between different cloud textures depending on the current weather. Speed of the clouds moving across the sky can be set per weather type.

Stars
*****

A dome shaped mesh that shows the stars during the night. It starts to become visible during sunset and goes back to transparent during sunrise. At its bottom edge it blends to transparency which is defined with the vertex colour. White is full opacity while black is full transparency. The mesh ends above the horizon to prevent the stars being visible near the horizon when underwater.

.. image:: https://gitlab.com/OpenMW/openmw-docs/-/raw/master/docs/source/reference/modding/_static/sky-system-mesh-stars.jpg
    :align: center

It can use multiple textures to show different star constellations. In addition, extra meshes can be used to blend nebulae across the sky.

Sun
***

The sun is a billboard that moves across the sky. It is composed of two texures. One shows the regular sun sphere, the other is the sun glare that is added on top of the sun. Glare strength adjusts dynamically depending on how obstructed the view to the sun is.

Moons
*****

The moons are two separate billboards moving across the sky and are both rendered the same way. First, a circle texture is used to mask the background. A moon texture is then added on top of the mask. Depending on the current moon phase, a variant of the moon texture is used. The texture on top is additively blended so any transparent area is achieved with black pixels. The following image shows all the separate textures needed for one moon.

.. image:: https://gitlab.com/OpenMW/openmw-docs/-/raw/master/docs/source/reference/modding/_static/sky-system-moon-textures.jpg
    :align: center


Weather Effects
***************

These are particle emitters used to display weather phenomena such as rain, snow, or dust clouds. Originally, Morrowind used .nif files with a number of planes and baked animations. In OpenMW, these effects are done through code and are currently hardcoded. The particle emmitters emit particles around the player in a big enough area so it looks like the whole world is affected by the weather.

Settings
********

Colour and other settings for each weather type can be edited in ``openmw.cfg`` configuration file.


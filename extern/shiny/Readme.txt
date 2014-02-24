shiny - a shader and material management library for OGRE

FEATURES

- High-level layer on top of OGRE's material system. It allows you to generate multiple techniques for all your materials from a set of high-level per-material properties.

- Several available Macros in shader source files. Just a few examples of the possibilities: binding OGRE auto constants, binding uniforms to material properties, foreach loops (repeat shader source a given number of times), retrieving per-material properties in an #if condition, automatic packing for vertex to fragment passthroughs. These macros allow you to generate even very complex shaders (for example the Ogre::Terrain shader) without assembling them in C++ code. 

- Integrated preprocessor (no, I didn't reinvent the wheel, I used boost::wave which turned out to be an excellent choice) that allows me to blend out macros that shouldn't be in use because e.g. the shader permutation doesn't need this specific feature.

- User settings integration. They can be set by a C++ interface and retrieved through a macro in shader files.

- Automatic handling of shader permutations, i.e. shaders are shared between materials in a smart way. 

- An optional "meta-language" (well, actually it's just a small header with some conditional defines) that you may use to compile the same shader source for different target languages. If you don't like it, you can still code in GLSL / CG etc separately. You can also switch between the languages at runtime.

- On-demand material and shader creation. It uses Ogre's material listener to compile the shaders as soon as they are needed for rendering, and not earlier.

- Shader changes are fully dynamic and real-time. Changing a user setting will recompile all shaders affected by this setting when they are next needed.

- Serialization system that extends Ogre's material script system, it uses Ogre's script parser, but also adds some additional properties that are not available in Ogre's material system. 

- A concept called "Configuration" allowing you to create a different set of your shaders, doing the same thing except for some minor differences: the properties that are overridden by the active configuration. Possible uses for this are using simpler shaders (no shadows, no fog etc) when rendering for example realtime reflections or a minimap. You can easily switch between configurations by changing the active Ogre material scheme (for example on a viewport level).

- Fixed function support. You can globally enable or disable shaders at any time, and for texture units you can specify if they're only needed for the shader-based path (e.g. normal maps) or if they should also be created in the fixed function path. 

LICENSE

see License.txt

AUTHOR

scrawl <scrawl@baseoftrash.de>

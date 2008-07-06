/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: http://openmw.snaptoad.com/

  This file (bindings.d) is part of the OpenMW package.

  OpenMW is distributed as free software: you can redistribute it
  and/or modify it under the terms of the GNU General Public License
  version 3, as published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  version 3 along with this program. If not, see
  http://www.gnu.org/licenses/ .

 */

module ogre.bindings;

import nif.misc; // for Transformation
import ogre.ogre; // for Placement

import core.resource;

/*
 * This module is the interface to OGRE from D. Since OGRE is written
 * in C++, all the code that deals directly with the graphics engine
 * is packaged in a bunch of C++ functions. These functions are
 * exported from C++ through the C calling convention, and imported
 * here.
 *
 * Note that the C calling convension is not in any way type
 * safe. This is convenient, as it allows us to send pointers as one
 * type and recieve them as another, without casting, but also
 * dangerous since it opens for some nasty bugs.
 */

// Represents a pointer to a Node in the OGRE engine. We never use
// these directly in D code, only pass them back to the C++ code.
typedef void* NodePtr;

// Pointer to a manual loader class in C++.
typedef void* ManualLoader;

extern(C):

// Do engine configuration. Returns 0 if we should continue, 1 if
// not. The parameter (0 or 1) determine whether an Ogre config
// dialogue is shown.
int cpp_configure(int showConfig);

// Sets up the window
void cpp_initWindow();

// Set up an empty scene.
void cpp_makeScene();

// Set the ambient light and "sunlight"
void cpp_setAmbient(float r, float g, float b,
		    float rs, float gs, float bs);

// Set fog color and view distance
void cpp_setFog(float rf, float gf, float bf,
		float flow, float fhigh);

// Create a simple sky dome
int cpp_makeSky();

// Enter main rendering loop
void cpp_startRendering();

// Cleans up after ogre
void cpp_cleanup();

// Create a manual loader for textures
ManualLoader cpp_createTexture(
	         char* name,  // Resource name
		 TextureIndex ti); // Texture index

// Inserts texture from memory data. This isn't used anymore and will
// be removed.
void cpp_loadMemImage(
	     char* name,  // Name to give the resource
	     char* type,  // Image type (eg. "dds")
	     void* data,  // Pointer to file data
	     uint size, 	 // Size
	     void* texture); // Texture resource

// Gets a child SceneNode from the root node, then detatches it to
// hide it from view. Used for creating the "template" node associated
// with a NIF mesh.
NodePtr cpp_getDetachedNode();

// Create a copy of the given scene node, with the given coordinates
// and rotation.
NodePtr cpp_insertNode(NodePtr base, char* name,
		       Placement *pos, float scale);

// Create a (very crappy looking) plane to simulate the water level
void cpp_createWater(float level);

// Creates a scene node as a child of 'parent', then translates and
// rotates it according to the data in 'trafo'.
NodePtr cpp_createNode(
	     char *name, 		// Name to give the node
	     Transformation *trafo,	// Transformation
	     NodePtr parent,		// Parent node
	     int noRot);		// If 1, don't rotate node

// Create a light with the given diffuse color. Attach it to SceneNode
// 'parent'.
NodePtr cpp_attachLight(char* name, NodePtr parent,
			float r, float g, float b,
			float radius);

// Create the specified material
void cpp_createMaterial(char *name,	// Name to give resource
			float *ambient, // Ambient RBG value
			float *diffuse,
			float *specular,
			float *emissive,// Self illumination
			float glossiness,// Same as shininess?
			float alpha,    // Use this in all alpha values?
			char *texture); // Texture name

// Creates a mesh and gives it a bounding box. Also creates an entity
// and attached it to the given SceneNode 'owner'.
void cpp_createMesh(
		char* name,		// Name of the mesh
		int numVerts,		// Number of vertices
		float* vertices,	// Vertex list
		float* normals,		// Normal list
		float* colors,		// Vertex colors
		float* uvs,		// Texture coordinates
		int numFaces,		// Number of faces*3
		short* faces,		// Faces
		float radius,		// Bounding sphere
		char* material,		// Material name, if any

		// Bounding box
		float minX,float minY,float minZ,
		float maxX,float maxY,float maxZ,

		NodePtr owner		// Scene node to attach to.
		);

// Save a screen shot to the given file name
void cpp_screenshot(char *filename);

// Camera controll
void cpp_rotateCamera(float x, float y);
void cpp_moveCamera(float x, float y, float z, float r1, float r2, float r3);
void cpp_getCameraPos(float *x, float *y, float *z);
void cpp_moveCameraRel(float x, float y, float z);

// Do some debug action. Check the menu for today's specials!
void cpp_debug(int i);

// CALLBACKS:

// Called when a texture wants to be loaded. Not used anymore.
void d_loadTexture(TextureIndex ti, void *tex)
{
  assert(0);
  resources.loadTexture(ti, tex);
}

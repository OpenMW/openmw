/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: http://openmw.snaptoad.com/

  This file (meshloader.d) is part of the OpenMW package.

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

module ogre.meshloader;

import std.stdio;
import std.stream;

import nif.nif;
import nif.record;

import core.resource;
import ogre.bindings;

import bullet.bindings;

import util.uniquename;

/*
 There are some problems that will have to be looked into later:

 - Some meshes crash Ogre when shadows are turned on. (Not tested in
   newer versions of Ogre). Shadows are completely disabled for now.
 - There are obviously some boundry problems, some times the mesh
   disappears even though some part of it is inside the screen. This
   is especially a problem with animated meshes, since the animation
   might step outside the original bounding box.
 */

MeshLoader meshLoader;

struct MeshLoader
{
  // Not sure how to handle the bounding box, just ignore it for now.

  char[] baseName; // NIF file name. Used in scene node names etc. so
		   // that we can identify where they came from in
		   // case of error messages.

  // Load a NIF mesh. Assumes nifMesh is already opened. This creates
  // a "template" scene node containing this mesh, and removes it from
  // the main scene. This node can later be "cloned" so that multiple
  // instances of the object can be inserted into the world without
  // inserting the mesh more than once.
  void loadMesh(char[] name, out NodePtr base, out BulletShape shape)
  {
    baseName = name;

    // Check if the first record is a node
    Node n = cast(Node) nifMesh.records[0];

    if(n is null)
      {
	// TODO: Figure out what to do in this case, we should
	// probably throw.
        writefln("NIF '%s' IS NOT A MESH", name);
	return;
      }

    // Get a fresh SceneNode and detatch it from the root. We use this
    // as the base for our mesh.
    base = ogre_getDetachedNode();

    // Recursively insert nodes (don't rotate the first node)
    insertNode(n, base, true);

    // Get the final shape, if any
    shape = bullet_getFinalShape();

    return base;
  }

  private:

  void insertNode(Node data, NodePtr parent, bool noRot = false)
  {
    // Skip hidden nodes for now.
    if(data.flags & 0x01) return;

    // Create a scene node, move and rotate it into place. The name
    // must be unique, however we might have to recognize some special
    // names later, in order to attach arms and legs on NPCs
    // etc. Always ignore transformation of the first node? This is a
    // problem, I don't know when to do this and when not to. Neither
    // is always right. Update: I originally thought noRot should be
    // false for exteriors and true for interiors, but this isn't so.
    NodePtr node = ogre_createNode(UniqueName(data.name).ptr, &data.trafo,
				  parent, cast(int)noRot);

    // Handle any general properties here

    // Call functions that do node-specific things, like handleNiNode
    // or handleNiTriShape.
    {
      NiNode n = cast(NiNode)data;
      if(n !is null)
	// Handle the NiNode, and any children it might have
	handleNiNode(n, node);
    }

    {
      NiTriShape n = cast(NiTriShape)data;
      if(n !is null)
	// Trishape, with a mesh
	handleNiTriShape(n, node);
    }
  }

  void handleNiNode(NiNode data, NodePtr node)
  {
    // Ignore sound activators and similar objects.
    NiStringExtraData d = cast(NiStringExtraData) data.extra;
    if(d !is null && d.string == "MRK")
      return;

    // Handle any effects here

    // In the cases where meshes have skeletal animations, we must
    // insert the children as bones in a skeleton instead, like we
    // originally did for all nodes. Update: A much better way is to
    // first insert the nodes normally, and then create the
    // skeleton. The nodes can then be moved one by one over to the
    // appropriate bones.

    // Loop through children
    foreach(Node n; data.children)
      insertNode(n, node);
  }

  void handleNiTriShape(NiTriShape shape, NodePtr node)
  {
    char[] texture;
    char[] material;
    char[] newName = UniqueName(baseName);
    NiMaterialProperty mp;

    // Scan the property list for textures
    foreach(Property p; shape.properties)
      {
	// NiTexturingProperty block
	{
	  NiTexturingProperty t = cast(NiTexturingProperty) p;
	  if(t !is null && t.textures[0].inUse)
	    {
	      // Ignore all other options for now
	      NiSourceTexture st = t.textures[0].texture;
	      if(st.external)
		{
		  // Find the resource for this texture
		  TextureIndex ti = resources.lookupTexture(st.filename);
		  // Insert a manual loader into OGRE
		  // ti.load();

		  // Get the resource name. We use getNewName to get
		  // the real texture name, not the lookup
		  // name. NewName has been converted to .dds if
		  // necessary, to match the file name in the bsa
		  // archives.
		  texture = ti.getNewName();
		}
	      else
		{
		  // Internal textures
		  texture = "BLAH";
		  writefln("Internal texture, cannot read this yet.");
		  writefln("Final resource name: '%s'", texture);
		}
	      continue;
	    }
	}

	// NiMaterialProperty block
	{
	  NiMaterialProperty tmp = cast(NiMaterialProperty) p;
	  if(tmp !is null)
	    {
	      if(mp !is null) writefln("WARNING: More than one material!");
	      mp = tmp;
	      material = newName;
	      continue;
	    }
	  //writefln("Unknown property found: ", p);
	}
      }
    //if(!texture.length) writefln("No texture found");

    // Get a pointer to the texture name
    char* texturePtr;
    if(texture.length) texturePtr = toStringz(texture);
    else texturePtr = null;

    // Create the material
    if(material.length)
      ogre_createMaterial(material.ptr, mp.ambient.array.ptr, mp.diffuse.array.ptr,
			 mp.specular.array.ptr, mp.emissive.array.ptr,
			 mp.glossiness, mp.alpha, texturePtr);
    else if(texturePtr)
      {
	// Texture, but no material. Make a default one.
	writefln("WARNING: Making default material for %s", texture);
	float[3] zero;
	float[3] one;
	zero[] = 0.0;
	one[] = 1.0;

	ogre_createMaterial(newName.ptr, one.ptr, one.ptr, zero.ptr, zero.ptr, 0.0, 1.0,
			   texturePtr);
      }

    with(shape.data)
      {
	//writefln("Number of vertices: ", vertices.length);

	float *normalsPtr;
	float *colorsPtr;
	float *uvsPtr;
	short *facesPtr;

	// Point pointers into the correct arrays, if they are present. If
	// not, the pointers retain their values of null.
	if(normals.length) normalsPtr = normals.ptr;
	if(colors.length) colorsPtr = colors.ptr;
	if(uvlist.length) uvsPtr = uvlist.ptr;
	if(triangles.length) facesPtr = triangles.ptr;

	float
	  minX = float.infinity,
	  minY = float.infinity,
	  minZ = float.infinity,
	  maxX = -float.infinity,
	  maxY = -float.infinity,
	  maxZ = -float.infinity;

	// Calculate the bounding box. TODO: This is really a hack.
	for( int i; i < vertices.length; i+=3 )
	  {
	    if( vertices[i] < minX ) minX = vertices[i];
	    if( vertices[i+1] < minY ) minY = vertices[i+1];
	    if( vertices[i+2] < minZ) minZ = vertices[i+2];

	    if( vertices[i] > maxX) maxX = vertices[i];
	    if( vertices[i+1] > maxY) maxY = vertices[i+1];
	    if( vertices[i+2] > maxZ) maxZ = vertices[i+2];
	  }

        // TODO: Get the node world transformation, needed to set up
        // the collision shape properly.
        float[3] trans;
        float[9] matrix;
        ogre_getWorldTransform(node, trans.ptr, matrix.ptr);

        // Create a bullet collision shape from the trimesh, if there
        // are any triangles present. Pass along the world
        // transformation as well, since we must transform the trimesh
        // data manually.
        if(facesPtr != null)
          bullet_createTriShape(triangles.length, facesPtr,
                                vertices.length, vertices.ptr,
                                trans.ptr, matrix.ptr);

        // Create the ogre mesh, associate it with the node
	ogre_createMesh(newName.ptr, vertices.length, vertices.ptr,
		       normalsPtr, colorsPtr, uvsPtr, triangles.length, facesPtr,
		       radius, material.ptr, minX, minY, minZ, maxX, maxY, maxZ,
		       node);
      }
  }
}
/*
    // Create a skeleton and get the root bone (index 0)
    BonePtr bone = ogre_setupSkeleton(name);

    // Reset the bone index. The next bone to be created has index 1.
    boneIndex = 1;
    // Create a mesh and assign the skeleton to it
    MeshPtr mesh = ogre_setupMesh(name);

    // Loop through the nodes, creating submeshes, materials and
    // skeleton bones in the process.
    handleNode(node, bone, mesh);

  // Create the "template" entity
  EntityPtr entity = ogre_createEntity(name);

  // Loop through once again, this time to set the right
  // transformations on the entity's SkeletonInstance. The order of
  // children will be the same, allowing us to reference bones using
  // their boneIndex.
  int lastBone = boneIndex;
  boneIndex = 1;
  transformBones(node, entity);
  if(lastBone != boneIndex) writefln("WARNING: Bone number doesn't match");

  if(!hasBBox)
    ogre_setMeshBoundingBox(mesh, minX, minY, minZ, maxX, maxY, maxZ);

  return entity;
}
void handleNode(Node node, BonePtr root, MeshPtr mesh)
{
  // Insert a new bone for this node
  BonePtr bone = ogre_insertBone(node.name, root, boneIndex++);

}

void transformBones(Node node, EntityPtr entity)
{
  ogre_transformBone(entity, &node.trafo, boneIndex++);

  NiNode n = cast(NiNode)node;
  if(n !is null)
    foreach(Node nd; n.children)
      transformBones(nd, entity);
}
*/

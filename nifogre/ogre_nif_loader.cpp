/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008-2010  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: http://openmw.sourceforge.net/

  This file (ogre_nif_loader.cpp) is part of the OpenMW package.

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

#include "ogre_nif_loader.hpp"
#include <Ogre.h>
#include <stdio.h>

#include <mangle/vfs/servers/ogre_vfs.hpp>
#include "nif/nif_file.hpp"
#include "nif/node.hpp"
#include "nif/data.hpp"
#include "nif/property.hpp"
#include "platform/strings.h"

// For warning messages
#include <iostream>

// float infinity
#include <limits>

typedef unsigned char ubyte;

using namespace std;
using namespace Ogre;
using namespace Nif;
using namespace Mangle::VFS;

// This is the interface to the Ogre resource system. It allows us to
// load NIFs from BSAs, in the file system and in any other place we
// tell Ogre to look (eg. in zip or rar files.) It's also used to
// check for the existence of texture files, so we can exchange the
// extension from .tga to .dds if the texture is missing.
static OgreVFS *vfs;

// Singleton instance used by load()
static NIFLoader g_sing;

// Makeshift error reporting system
static string errName;
static void warn(const string &msg)
{
  cout << "WARNING (NIF:" << errName << "): " << msg << endl;
}

// Helper class that computes the bounding box and of a mesh
class BoundsFinder
{
  struct MaxMinFinder
  {
    float max, min;

    MaxMinFinder()
    {
      min = numeric_limits<float>::infinity();
      max = -min;
    }

    void add(float f)
    {
      if(f > max) max = f;
      if(f < min) min = f;
    }

    // Return Max(max**2, min**2)
    float getMaxSquared()
    {
      float m1 = max*max;
      float m2 = min*min;
      if(m1 >= m2) return m1;
      return m2;
    }
  };

  MaxMinFinder X, Y, Z;

public:
  // Add 'verts' vertices to the calculation. The 'data' pointer is
  // expected to point to 3*verts floats representing x,y,z for each
  // point.
  void add(float *data, int verts)
  {
    for(int i=0;i<verts;i++)
      {
        X.add(*(data++));
        Y.add(*(data++));
        Z.add(*(data++));
      }
  }

  // True if this structure has valid values
  bool isValid()
  {
    return
      minX() <= maxX() &&
      minY() <= maxY() &&
      minZ() <= maxZ();
  }

  // Compute radius
  float getRadius()
  {
    assert(isValid());

    // The radius is computed from the origin, not from the geometric
    // center of the mesh.
    return sqrt(X.getMaxSquared() + Y.getMaxSquared() + Z.getMaxSquared());
  }

  float minX() { return X.min; }
  float maxX() { return X.max; }
  float minY() { return Y.min; }
  float maxY() { return Y.max; }
  float minZ() { return Z.min; }
  float maxZ() { return Z.max; }
};

// Conversion of blend / test mode from NIF -> OGRE. Not in use yet.
static SceneBlendFactor getBlendFactor(int mode)
{
  switch(mode)
    {
    case 0: return SBF_ONE;
    case 1: return SBF_ZERO;
    case 2: return SBF_SOURCE_COLOUR;
    case 3: return SBF_ONE_MINUS_SOURCE_COLOUR;
    case 4: return SBF_DEST_COLOUR;
    case 5: return SBF_ONE_MINUS_DEST_COLOUR;
    case 6: return SBF_SOURCE_ALPHA;
    case 7: return SBF_ONE_MINUS_SOURCE_ALPHA;
    case 8: return SBF_DEST_ALPHA;
    case 9: return SBF_ONE_MINUS_DEST_ALPHA;
    /* [Comment from Chris Robinson:] Can't handle this mode? :/
    case 10: return SBF_SOURCE_ALPHA_SATURATE;
    */
    default:
      return SBF_SOURCE_ALPHA;
    }
}

static CompareFunction getTestMode(int mode)
{
  switch(mode)
    {
    case 0: return CMPF_ALWAYS_PASS;
    case 1: return CMPF_LESS;
    case 2: return CMPF_EQUAL;
    case 3: return CMPF_LESS_EQUAL;
    case 4: return CMPF_GREATER;
    case 5: return CMPF_NOT_EQUAL;
    case 6: return CMPF_GREATER_EQUAL;
    case 7: return CMPF_ALWAYS_FAIL;
    default:
      return CMPF_ALWAYS_PASS;
    }
}

static void createMaterial(const String &name,
                           const Vector &ambient,
                           const Vector &diffuse,
                           const Vector &specular,
                           const Vector &emissive,
                           float glossiness, float alpha,
                           float alphaFlags, float alphaTest,
                           const String &texName)
{
  MaterialPtr material = MaterialManager::getSingleton().create(name, "General");

  // This assigns the texture to this material. If the texture name is
  // a file name, and this file exists (in a resource directory), it
  // will automatically be loaded when needed. If not (such as for
  // internal NIF textures that we might support later), we should
  // already have inserted a manual loader for the texture.
  if(!texName.empty())
    {
      Pass *pass = material->getTechnique(0)->getPass(0);
      /*TextureUnitState *txt =*/ pass->createTextureUnitState(texName);

      /* As of yet UNTESTED code from Chris:
      pass->setTextureFiltering(Ogre::TFO_ANISOTROPIC);
      pass->setDepthFunction(Ogre::CMPF_LESS_EQUAL);
      pass->setDepthCheckEnabled(true);

      // Add transparency if NiAlphaProperty was present
      if(alphaFlags != -1)
        {
          if((alphaFlags&1))
            {
              pass->setDepthWriteEnabled(false);
              pass->setSceneBlending(getBlendFactor((alphaFlags>>1)&0xf),
                                     getBlendFactor((alphaFlags>>5)&0xf));
            }
          else
            pass->setDepthWriteEnabled(true);

          if((alphaFlags>>9)&1)
            pass->setAlphaRejectSettings(getTestMode((alphaFlags>>10)&0x7),
                                         alphaTest);

          pass->setTransparentSortingEnabled(!((alphaFlags>>13)&1));
        }
      else
        pass->setDepthWriteEnabled(true);
      */

      // Add transparency if NiAlphaProperty was present
      if(alphaFlags != -1)
        {
          // The 237 alpha flags are by far the most common. Check
          // NiAlphaProperty in nif/property.h if you need to decode
          // other values. 237 basically means normal transparencly.
          if(alphaFlags == 237)
            {
              // Enable transparency
              pass->setSceneBlending(SBT_TRANSPARENT_ALPHA);

              //pass->setDepthCheckEnabled(false);
              pass->setDepthWriteEnabled(false);
            }
          else
            warn("Unhandled alpha setting for texture " + texName);
        }
    }

  // Add material bells and whistles
  material->setAmbient(ambient.array[0], ambient.array[1], ambient.array[2]);
  material->setDiffuse(diffuse.array[0], diffuse.array[1], diffuse.array[2], alpha);
  material->setSpecular(specular.array[0], specular.array[1], specular.array[2], alpha);
  material->setSelfIllumination(emissive.array[0], emissive.array[1], emissive.array[2]);
  material->setShininess(glossiness);
}

// Takes a name and adds a unique part to it. This is just used to
// make sure that all materials are given unique names.
static String getUniqueName(const String &input)
{
  static int addon = 0;
  static char buf[8];
  snprintf(buf, 8, "_%d", addon++);

  // Don't overflow the buffer
  if(addon > 999999) addon = 0;

  return input + buf;
}

// Check if the given texture name exists in the real world. If it
// does not, change the string IN PLACE to say .dds instead and try
// that. The texture may still not exist, but no information of value
// is lost in that case.
static void findRealTexture(String &texName)
{
  assert(vfs);
  if(vfs->isFile(texName)) return;

  int len = texName.size();
  if(len < 4) return;

  // Change texture extension to .dds
  texName[len-3] = 'd';
  texName[len-2] = 'd';
  texName[len-1] = 's';
}

// Convert Nif::NiTriShape to Ogre::SubMesh, attached to the given
// mesh.
static void createOgreMesh(Mesh *mesh, NiTriShape *shape, const String &material)
{
  NiTriShapeData *data = shape->data.getPtr();
  SubMesh *sub = mesh->createSubMesh(shape->name.toString());

  int nextBuf = 0;

  // This function is just one long stream of Ogre-barf, but it works
  // great.

  // Add vertices
  int numVerts = data->vertices.length / 3;
  sub->vertexData = new VertexData();
  sub->vertexData->vertexCount = numVerts;
  sub->useSharedVertices = false;
  VertexDeclaration *decl = sub->vertexData->vertexDeclaration;
  decl->addElement(nextBuf, 0, VET_FLOAT3, VES_POSITION);
  HardwareVertexBufferSharedPtr vbuf =
    HardwareBufferManager::getSingleton().createVertexBuffer(
      VertexElement::getTypeSize(VET_FLOAT3),
      numVerts, HardwareBuffer::HBU_STATIC_WRITE_ONLY);
  vbuf->writeData(0, vbuf->getSizeInBytes(), data->vertices.ptr, true);
  VertexBufferBinding* bind = sub->vertexData->vertexBufferBinding;
  bind->setBinding(nextBuf++, vbuf);

  // Vertex normals
  if(data->normals.length)
    {
      decl->addElement(nextBuf, 0, VET_FLOAT3, VES_NORMAL);
      vbuf = HardwareBufferManager::getSingleton().createVertexBuffer(
          VertexElement::getTypeSize(VET_FLOAT3),
          numVerts, HardwareBuffer::HBU_STATIC_WRITE_ONLY);
      vbuf->writeData(0, vbuf->getSizeInBytes(), data->normals.ptr, true);
      bind->setBinding(nextBuf++, vbuf);
    }

  // Vertex colors
  if(data->colors.length)
    {
      const float *colors = data->colors.ptr;
      RenderSystem* rs = Root::getSingleton().getRenderSystem();
      std::vector<RGBA> colorsRGB(numVerts);
      RGBA *pColour = &colorsRGB.front();
      for(int i=0; i<numVerts; i++)
	{
	  rs->convertColourValue(ColourValue(colors[0],colors[1],colors[2],
                                             colors[3]),pColour++);
	  colors += 4;
	}
      decl->addElement(nextBuf, 0, VET_COLOUR, VES_DIFFUSE);
      vbuf = HardwareBufferManager::getSingleton().createVertexBuffer(
          VertexElement::getTypeSize(VET_COLOUR),
	  numVerts, HardwareBuffer::HBU_STATIC_WRITE_ONLY);
      vbuf->writeData(0, vbuf->getSizeInBytes(), &colorsRGB.front(), true);
      bind->setBinding(nextBuf++, vbuf);
    }

  // Texture UV coordinates
  if(data->uvlist.length)
    {
      decl->addElement(nextBuf, 0, VET_FLOAT2, VES_TEXTURE_COORDINATES);
      vbuf = HardwareBufferManager::getSingleton().createVertexBuffer(
          VertexElement::getTypeSize(VET_FLOAT2),
          numVerts, HardwareBuffer::HBU_STATIC_WRITE_ONLY);

      vbuf->writeData(0, vbuf->getSizeInBytes(), data->uvlist.ptr, true);
      bind->setBinding(nextBuf++, vbuf);
    }

  // Triangle faces
  int numFaces = data->triangles.length;
  if(numFaces)
    {
      HardwareIndexBufferSharedPtr ibuf = HardwareBufferManager::getSingleton().
	createIndexBuffer(HardwareIndexBuffer::IT_16BIT,
			  numFaces,
			  HardwareBuffer::HBU_STATIC_WRITE_ONLY);
      ibuf->writeData(0, ibuf->getSizeInBytes(), data->triangles.ptr, true);
      sub->indexData->indexBuffer = ibuf;
      sub->indexData->indexCount = numFaces;
      sub->indexData->indexStart = 0;
    }

  // Set material if one was given
  if(!material.empty()) sub->setMaterialName(material);

  /* Old commented D code. Might be useful when reimplementing
     animation.
  // Assign this submesh to the given bone
  VertexBoneAssignment v;
  v.boneIndex = ((Bone*)bone)->getHandle();
  v.weight = 1.0;

  std::cerr << "+ Assigning bone index " << v.boneIndex << "\n";

  for(int i=0; i < numVerts; i++)
    {
      v.vertexIndex = i;
      sub->addBoneAssignment(v);
    }
  */
}

// Helper math functions. Reinventing linear algebra for the win!

// Computes B = AxB (matrix*matrix)
static void matrixMul(const Matrix &A, Matrix &B)
{
  for(int i=0;i<3;i++)
    {
      float a = B.v[0].array[i];
      float b = B.v[1].array[i];
      float c = B.v[2].array[i];

      B.v[0].array[i] = a*A.v[0].array[0] + b*A.v[0].array[1] + c*A.v[0].array[2];
      B.v[1].array[i] = a*A.v[1].array[0] + b*A.v[1].array[1] + c*A.v[1].array[2];
      B.v[2].array[i] = a*A.v[2].array[0] + b*A.v[2].array[1] + c*A.v[2].array[2];
    }
}

// Computes C = B + AxC*scale
static void vectorMulAdd(const Matrix &A, const Vector &B, float *C, float scale)
{
  // Keep the original values
  float a = C[0];
  float b = C[1];
  float c = C[2];

  // Perform matrix multiplication, scaling and addition
  for(int i=0;i<3;i++)
    C[i] = B.array[i] + (a*A.v[i].array[0] + b*A.v[i].array[1] + c*A.v[i].array[2])*scale;
}

// Computes B = AxB (matrix*vector)
static void vectorMul(const Matrix &A, float *C)
{
  // Keep the original values
  float a = C[0];
  float b = C[1];
  float c = C[2];

  // Perform matrix multiplication, scaling and addition
  for(int i=0;i<3;i++)
    C[i] = a*A.v[i].array[0] + b*A.v[i].array[1] + c*A.v[i].array[2];
}

static void handleNiTriShape(Mesh *mesh, NiTriShape *shape, int flags, BoundsFinder &bounds)
{
  assert(shape != NULL);

  // Interpret flags
  bool hidden    = (flags & 0x01) != 0; // Not displayed
  bool collide   = (flags & 0x02) != 0; // Use mesh for collision
  bool bbcollide = (flags & 0x04) != 0; // Use bounding box for collision

  // Bounding box collision isn't implemented, always use mesh for now.
  if(bbcollide)
    {
      collide = true;
      bbcollide = false;
    }

  // If the object was marked "NCO" earlier, it shouldn't collide with
  // anything.
  if(flags & 0x800)
    { collide = false; bbcollide = false; }

  if(!collide && !bbcollide && hidden)
    // This mesh apparently isn't being used for anything, so don't
    // bother setting it up.
    return;

  // Material name for this submesh, if any
  String material;

  // Skip the entire material phase for hidden nodes
  if(!hidden)
    {
      // These are set below if present
      NiTexturingProperty *t = NULL;
      NiMaterialProperty *m = NULL;
      NiAlphaProperty *a = NULL;

      // Scan the property list for material information
      PropertyList &list = shape->props;
      int n = list.length();
      for(int i=0; i<n; i++)
        {
          // Entries may be empty
          if(!list.has(i)) continue;

          Property *pr = &list[i];

          if(pr->recType == RC_NiTexturingProperty)
            t = (NiTexturingProperty*)pr;
          else if(pr->recType == RC_NiMaterialProperty)
            m = (NiMaterialProperty*)pr;
          else if(pr->recType == RC_NiAlphaProperty)
            a = (NiAlphaProperty*)pr;
        }

      // Texture
      String texName;
      if(t && t->textures[0].inUse)
        {
          NiSourceTexture *st = t->textures[0].texture.getPtr();
          if(st->external)
            {
              SString tname = st->filename;

              /* findRealTexture checks if the file actually
                 exists. If it doesn't, and the name ends in .tga, it
                 will try replacing the extension with .dds instead
                 and search for that. Bethesda at some at some point
                 converted all their BSA textures from tga to dds for
                 increased load speed, but all texture file name
                 references were kept as .tga.

                 The function replaces the name in place (that's why
                 we cast away the const modifier), but this is no
                 problem since all the nif data is stored in a local
                 throwaway buffer.
               */
              texName = "textures\\" + tname.toString();
              findRealTexture(texName);
            }
          else warn("Found internal texture, ignoring.");
        }

      // Alpha modifiers
      int alphaFlags = -1;
      ubyte alphaTest = 0;
      if(a)
        {
          alphaFlags = a->flags;
          alphaTest  = a->data->threshold;
        }

      // Material
      if(m || !texName.empty())
        {
          // If we're here, then this mesh has a material. Thus we
          // need to calculate a snappy material name. It should
          // contain the mesh name (mesh->getName()) but also has to
          // be unique. One mesh may use many materials.
          material = getUniqueName(mesh->getName());

          if(m)
            {
              // Use NiMaterialProperty data to create the data
              const S_MaterialProperty *d = m->data;
              createMaterial(material, d->ambient, d->diffuse, d->specular, d->emissive,
                             d->glossiness, d->alpha, alphaFlags, alphaTest, texName);
            }
          else
            {
              // We only have a texture name. Create a default
              // material for it.
              Vector zero, one;
              for(int i=0; i<3;i++)
                {
                  zero.array[i] = 0.0;
                  one.array[i] = 1.0;
                }

              createMaterial(material, one, one, zero, zero, 0.0, 1.0,
                             alphaFlags, alphaTest, texName);
            }
        }
    } // End of material block, if(!hidden) ...

  /* Do in-place transformation of all the vertices and normals. This
     is pretty messy stuff, but we need it to make the sub-meshes
     appear in the correct place. Neither Ogre nor Bullet support
     nested levels of sub-meshes with transformations applied to each
     level.
  */
  NiTriShapeData *data = shape->data.getPtr();
  int numVerts = data->vertices.length / 3;

  float *ptr = (float*)data->vertices.ptr;
  float *optr = ptr;

  // Rotate, scale and translate all the vertices
  const Matrix &rot = shape->trafo->rotation;
  const Vector &pos = shape->trafo->pos;
  float scale = shape->trafo->scale;
  for(int i=0; i<numVerts; i++)
    {
      vectorMulAdd(rot, pos, ptr, scale);
      ptr += 3;
    }

  // Remember to rotate all the vertex normals as well
  if(data->normals.length)
    {
      ptr = (float*)data->normals.ptr;
      for(int i=0; i<numVerts; i++)
        {
          vectorMul(rot, ptr);
          ptr += 3;
        }
    }

  if(!hidden)
    {
      // Add this vertex set to the bounding box
      bounds.add(optr, numVerts);

      // Create the submesh
      createOgreMesh(mesh, shape, material);
    }
}

static void handleNode(Mesh* mesh, Nif::Node *node, int flags,
                       const Transformation *trafo, BoundsFinder &bounds)
{
  // Accumulate the flags from all the child nodes. This works for all
  // the flags we currently use, at least.
  flags |= node->flags;

  // Check for extra data
  Extra *e = node;
  while(!e->extra.empty())
    {
      // Get the next extra data in the list
      e = e->extra.getPtr();
      assert(e != NULL);

      if(e->recType == RC_NiStringExtraData)
        {
          // String markers may contain important information
          // affecting the entire subtree of this node
          NiStringExtraData *sd = (NiStringExtraData*)e;

          if(sd->string == "NCO")
            // No collision. Use an internal flag setting to mark this.
            flags |= 0x800;
          else if(sd->string == "MRK")
            // Marker objects. These are only visible in the
            // editor. Until and unless we add an editor component to
            // the engine, just skip this entire node.
            return;
        }
    }

  // Apply the parent transformation to this node. We overwrite the
  // existing data with the final transformation.
  if(trafo)
    {
      // Get a non-const reference to the node's data, since we're
      // overwriting it. TODO: Is this necessary?
      Transformation &final = *((Transformation*)node->trafo);

      // For both position and rotation we have that:
      // final_vector = old_vector + old_rotation*new_vector*old_scale
      vectorMulAdd(trafo->rotation, trafo->pos, final.pos.array, trafo->scale);
      vectorMulAdd(trafo->rotation, trafo->velocity, final.velocity.array, trafo->scale);

      // Merge the rotations together
      matrixMul(trafo->rotation, final.rotation);

      // Scalar values are so nice to deal with. Why can't everything
      // just be scalar?
      final.scale *= trafo->scale;
    }

  // For NiNodes, loop through children
  if(node->recType == RC_NiNode)
    {
      NodeList &list = ((NiNode*)node)->children;
      int n = list.length();
      for(int i=0; i<n; i++)
        {
          if(list.has(i))
            handleNode(mesh, &list[i], flags, node->trafo, bounds);
        }
    }
  else if(node->recType == RC_NiTriShape)
    // For shapes
    handleNiTriShape(mesh, dynamic_cast<NiTriShape*>(node), flags, bounds);
}

void NIFLoader::loadResource(Resource *resource)
{
  // Set up the VFS if it hasn't been done already
  if(!vfs) vfs = new OgreVFS("General");

  // Get the mesh
  Mesh *mesh = dynamic_cast<Mesh*>(resource);
  assert(mesh);

  // Look it up
  const String &name = mesh->getName();
  errName = name; // Set name for error messages
  if(!vfs->isFile(name))
    {
      warn("File not found.");
      return;
    }

  // Helper that computes bounding boxes for us.
  BoundsFinder bounds;

  // Load the NIF. TODO: Wrap this in a try-catch block once we're out
  // of the early stages of development. Right now we WANT to catch
  // every error as early and intrusively as possible, as it's most
  // likely a sign of incomplete code rather than faulty input.
  NIFFile nif(vfs->open(name), name);

  if(nif.numRecords() < 1)
    {
      warn("Found no records in NIF.");
      return;
    }

  // The first record is assumed to be the root node
  Record *r = nif.getRecord(0);
  assert(r != NULL);

  Nif::Node *node = dynamic_cast<Nif::Node*>(r);

  if(node == NULL)
    {
      warn("First record in file was not a node, but a " +
           r->recName.toString() + ". Skipping file.");
      return;
    }

  // Handle the node
  handleNode(mesh, node, 0, NULL, bounds);

  // Finally, set the bounding value.
  if(bounds.isValid())
    {
      mesh->_setBounds(AxisAlignedBox(bounds.minX(), bounds.minY(), bounds.minZ(),
                                      bounds.maxX(), bounds.maxY(), bounds.maxZ()));
      mesh->_setBoundingSphereRadius(bounds.getRadius());
    }
}

MeshPtr NIFLoader::load(const std::string &name,
                        const std::string &group)
{
  MeshManager *m = MeshManager::getSingletonPtr();

  // Check if the resource already exists
  ResourcePtr ptr = m->getByName(name, group);
  if(!ptr.isNull())
    return MeshPtr(ptr);

  // Nope, create a new one.
  return MeshManager::getSingleton().createManual(name, group, &g_sing);
}

/* More code currently not in use, from the old D source. This was
   used in the first attempt at loading NIF meshes, where each submesh
   in the file was given a separate bone in a skeleton. Unfortunately
   the OGRE skeletons can't hold more than 256 bones, and some NIFs go
   way beyond that. The code might be of use if we implement animated
   submeshes like this (the part of the NIF that is animated is
   usually much less than the entire file, but the method might still
   not be water tight.)

// Insert a raw RGBA image into the texture system.
extern "C" void ogre_insertTexture(char* name, uint32_t width, uint32_t height, void *data)
{
  TexturePtr texture = TextureManager::getSingleton().createManual(
      name, 		// name
      "General",	// group
      TEX_TYPE_2D,     	// type
      width, height,    // width & height
      0,                // number of mipmaps
      PF_BYTE_RGBA,     // pixel format
      TU_DEFAULT);      // usage; should be TU_DYNAMIC_WRITE_ONLY_DISCARDABLE for
                        // textures updated very often (e.g. each frame)

  // Get the pixel buffer
  HardwarePixelBufferSharedPtr pixelBuffer = texture->getBuffer();

  // Lock the pixel buffer and get a pixel box
  pixelBuffer->lock(HardwareBuffer::HBL_NORMAL); // for best performance use HBL_DISCARD!
  const PixelBox& pixelBox = pixelBuffer->getCurrentLock();

  void *dest = pixelBox.data;

  // Copy the data
  memcpy(dest, data, width*height*4);

  // Unlock the pixel buffer
  pixelBuffer->unlock();
}

// We need this later for animated meshes.
extern "C" void* ogre_setupSkeleton(char* name)
{
  SkeletonPtr skel = SkeletonManager::getSingleton().create(
    name, "Closet", true);

  skel->load();

  // Create all bones at the origin and unrotated. This is necessary
  // since our submeshes each have their own model space. We must
  // move the bones after creating an entity, then copy this entity.
  return (void*)skel->createBone();
}

extern "C" void *ogre_insertBone(char* name, void* rootBone, int32_t index)
{
  return (void*) ( ((Bone*)rootBone)->createChild(index) );
}
*/
/* This was the D part:

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

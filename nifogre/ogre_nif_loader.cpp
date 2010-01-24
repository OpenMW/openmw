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

#include "ogre_nif_loader.h"
#include <Ogre.h>
#include <stdio.h>

#include "../mangle/vfs/servers/ogre_vfs.h"
#include "../nif/nif_file.h"
#include "../nif/node.h"
#include "../nif/data.h"
#include "../nif/property.h"

// For warning messages
#include <iostream>

typedef unsigned char ubyte;

using namespace std;
using namespace Ogre;
using namespace Nif;
using namespace Mangle::VFS;

#define TRANSLATE 1

// This is the interface to the Ogre resource system. It allows us to
// load NIFs from BSAs, in the file system and in any other place we
// tell Ogre to look (eg. in zip or rar files.) It's also used to
// check for the existence of texture files, so we can exchange the
// extension from .tga to .dds if the texture is missing.
OgreVFS *vfs;

// Singleton instance used by load()
static NIFLoader g_sing;

static string errName;

static void warn(const string &msg)
{
  cout << "WARNING (NIF:" << errName << "): " << msg << endl;
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
      TextureUnitState *txt = pass->createTextureUnitState(texName);

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
  snprintf(buf,8,"_%d", addon++);

  // Don't overflow the buffer
  if(addon > 1999999) addon = 0;

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
      RGBA colorsRGB[numVerts];
      RGBA *pColour = colorsRGB;
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
      vbuf->writeData(0, vbuf->getSizeInBytes(), colorsRGB, true);
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

static void handleNiTriShape(Mesh *mesh, NiTriShape *shape, int flags)
{
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
      ubyte alphaTest;
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
    }

  if(TRANSLATE) // TODO FIXME TEMP
  {
    /* Do in-place transformation of all the vertices and normals. This
       is pretty messy stuff, but we need it to make the sub-meshes
       appear in the correct place. Neither Ogre nor Bullet support
       nested levels of sub-meshes with transformations applied to each
       level.
    */
    NiTriShapeData *data = shape->data.getPtr();
    int numVerts = data->vertices.length / 3;

    float *ptr = (float*)data->vertices.ptr;

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
  }

  if(!hidden)
    createOgreMesh(mesh, shape, material);
}

static void handleNode(Mesh* mesh, Nif::Node *node, int flags, const Transformation *trafo = NULL)
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
  if(TRANSLATE) // TODO FIXME TEMP
  if(trafo)
    {
      // Get a non-const reference to the node's data, since we're
      // overwriting it.
      Transformation &final = *((Transformation*)node->trafo);

      // For both position and rotation we have that:
      // final_vector = old_vector + old_rotation*new_vector*old_scale
      vectorMulAdd(trafo->rotation, trafo->pos, final.pos.array, trafo->scale);
      vectorMulAdd(trafo->rotation, trafo->velocity, final.velocity.array, trafo->scale);

      // Merge the rotations together
      matrixMul(trafo->rotation, final.rotation);

      // Scalar values are so nice to deal with. Why can't everything
      // just be scalars?
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
            handleNode(mesh, &list[i], flags, node->trafo);
        }
    }
  else if(node->recType == RC_NiTriShape)
    // For shapes
    handleNiTriShape(mesh, (NiTriShape*)node, flags);
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

  if(r->recType != RC_NiNode)
    {
      warn("First record in file was not a NiNode, but a " +
           r->recName.toString() + ". Skipping file.");
      return;
    }

  // Handle the node
  handleNode(mesh, (Nif::Node*)r, 0);

  // Finally, set the bounding value. Just use bogus info right now.
  mesh->_setBounds(AxisAlignedBox(-10,-10,-10,10,10,10));
  mesh->_setBoundingSphereRadius(10);
}

MeshPtr NIFLoader::load(const char* name, const char* group)
{
  MeshManager *m = MeshManager::getSingletonPtr();

  // Check if the resource already exists
  ResourcePtr ptr = m->getByName(name/*, group*/);
  if(!ptr.isNull())
    return MeshPtr(ptr);

  // Nope, create a new one.
  return MeshManager::getSingleton().createManual(name, group, &g_sing);
}

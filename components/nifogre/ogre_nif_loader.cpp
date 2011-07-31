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

//loadResource->handleNode->handleNiTriShape->createSubMesh

#include "ogre_nif_loader.hpp"
#include <Ogre.h>
#include <stdio.h>

#include <libs/mangle/vfs/servers/ogre_vfs.hpp>
#include "../nif/nif_file.hpp"
#include "../nif/node.hpp"
#include "../nif/data.hpp"
#include "../nif/property.hpp"
#include "../nif/controller.hpp"
#include "../nif/extra.hpp"
#include <libs/platform/strings.h>

#include <vector>
#include <list>
// For warning messages
#include <iostream>

// float infinity
#include <limits>

typedef unsigned char ubyte;

using namespace std;
using namespace Ogre;
using namespace Nif;
using namespace Mangle::VFS;
using namespace Misc;
using namespace NifOgre;

NIFLoader& NIFLoader::getSingleton()
{
    static NIFLoader instance;
    return instance;
}

NIFLoader* NIFLoader::getSingletonPtr()
{
    return &getSingleton();
}

void NIFLoader::warn(string msg)
{
    std::cerr << "NIFLoader: Warn:" << msg << "\n";
}

void NIFLoader::fail(string msg)
{
    std::cerr << "NIFLoader: Fail: "<< msg << std::endl;
    assert(1);
}

Vector3 NIFLoader::convertVector3(const Nif::Vector& vec)
{
    return Ogre::Vector3(vec.array);
}

Quaternion NIFLoader::convertRotation(const Nif::Matrix& rot)
{
    Real matrix[3][3];

    for (int i=0; i<3; i++)
        for (int j=0; j<3; j++)
            matrix[i][j] = rot.v[i].array[j];

        return Quaternion(Matrix3(matrix));
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
            if (f > max) max = f;
            if (f < min) min = f;
        }

        // Return Max(max**2, min**2)
        float getMaxSquared()
        {
            float m1 = max*max;
            float m2 = min*min;
            if (m1 >= m2) return m1;
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
        for (int i=0;i<verts;i++)
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

    float minX() {
        return X.min;
    }
    float maxX() {
        return X.max;
    }
    float minY() {
        return Y.min;
    }
    float maxY() {
        return Y.max;
    }
    float minZ() {
        return Z.min;
    }
    float maxZ() {
        return Z.max;
    }
};

// Conversion of blend / test mode from NIF -> OGRE.
// Not in use yet, so let's comment it out.
/*
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
      // [Comment from Chris Robinson:] Can't handle this mode? :/
      // case 10: return SBF_SOURCE_ALPHA_SATURATE;
    default:
      return SBF_SOURCE_ALPHA;
    }
}


// This is also unused
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
*/

void NIFLoader::createMaterial(const String &name,
                           const Vector &ambient,
                           const Vector &diffuse,
                           const Vector &specular,
                           const Vector &emissive,
                           float glossiness, float alpha,
                           int alphaFlags, float alphaTest,
                           const String &texName)
{
    MaterialPtr material = MaterialManager::getSingleton().create(name, resourceGroup);

    // This assigns the texture to this material. If the texture name is
    // a file name, and this file exists (in a resource directory), it
    // will automatically be loaded when needed. If not (such as for
    // internal NIF textures that we might support later), we should
    // already have inserted a manual loader for the texture.
    if (!texName.empty())
    {
        Pass *pass = material->getTechnique(0)->getPass(0);
        /*TextureUnitState *txt =*/
        pass->createTextureUnitState(texName);

        // As of yet UNTESTED code from Chris:
        /*pass->setTextureFiltering(Ogre::TFO_ANISOTROPIC);
        pass->setDepthFunction(Ogre::CMPF_LESS_EQUAL);
        pass->setDepthCheckEnabled(true);

        // Add transparency if NiAlphaProperty was present
        if (alphaFlags != -1)
        {
            std::cout << "Alpha flags set!" << endl;
            if ((alphaFlags&1))
            {
                pass->setDepthWriteEnabled(false);
                pass->setSceneBlending(getBlendFactor((alphaFlags>>1)&0xf),
                                       getBlendFactor((alphaFlags>>5)&0xf));
            }
            else
                pass->setDepthWriteEnabled(true);

            if ((alphaFlags>>9)&1)
                pass->setAlphaRejectSettings(getTestMode((alphaFlags>>10)&0x7),
                                             alphaTest);

            pass->setTransparentSortingEnabled(!((alphaFlags>>13)&1));
        }
        else
            pass->setDepthWriteEnabled(true); */


        // Add transparency if NiAlphaProperty was present
        if (alphaFlags != -1)
        {
            // The 237 alpha flags are by far the most common. Check
            // NiAlphaProperty in nif/property.h if you need to decode
            // other values. 237 basically means normal transparencly.
            if (alphaFlags == 237)
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
String NIFLoader::getUniqueName(const String &input)
{
    static int addon = 0;
    static char buf[8];
    snprintf(buf, 8, "_%d", addon++);

    // Don't overflow the buffer
    if (addon > 999999) addon = 0;

    return input + buf;
}

// Check if the given texture name exists in the real world. If it
// does not, change the string IN PLACE to say .dds instead and try
// that. The texture may still not exist, but no information of value
// is lost in that case.
void NIFLoader::findRealTexture(String &texName)
{
    assert(vfs);
    if (vfs->isFile(texName)) return;

    int len = texName.size();
    if (len < 4) return;

    // Change texture extension to .dds
    texName[len-3] = 'd';
    texName[len-2] = 'd';
    texName[len-1] = 's';
}

//Handle node at top

// Convert Nif::NiTriShape to Ogre::SubMesh, attached to the given
// mesh.
void NIFLoader::createOgreSubMesh(NiTriShape *shape, const String &material, std::list<VertexBoneAssignment> &vertexBoneAssignments)
{
    //  cout << "s:" << shape << "\n";
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
    if (data->normals.length)
    {
        decl->addElement(nextBuf, 0, VET_FLOAT3, VES_NORMAL);
        vbuf = HardwareBufferManager::getSingleton().createVertexBuffer(
                   VertexElement::getTypeSize(VET_FLOAT3),
                   numVerts, HardwareBuffer::HBU_STATIC_WRITE_ONLY);
        vbuf->writeData(0, vbuf->getSizeInBytes(), data->normals.ptr, true);
        bind->setBinding(nextBuf++, vbuf);
    }

    // Vertex colors
    if (data->colors.length)
    {
        const float *colors = data->colors.ptr;
        RenderSystem* rs = Root::getSingleton().getRenderSystem();
        std::vector<RGBA> colorsRGB(numVerts);
        RGBA *pColour = &colorsRGB.front();
        for (int i=0; i<numVerts; i++)
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
    if (data->uvlist.length)
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
    if (numFaces)
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
    if (!material.empty()) sub->setMaterialName(material);

    //add vertex bone assignments

    for (std::list<VertexBoneAssignment>::iterator it = vertexBoneAssignments.begin();
        it != vertexBoneAssignments.end(); it++)
    {
            sub->addBoneAssignment(*it);
    }
}

// Helper math functions. Reinventing linear algebra for the win!

// Computes B = AxB (matrix*matrix)
static void matrixMul(const Matrix &A, Matrix &B)
{
    for (int i=0;i<3;i++)
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
    for (int i=0;i<3;i++)
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
    for (int i=0;i<3;i++)
        C[i] = a*A.v[i].array[0] + b*A.v[i].array[1] + c*A.v[i].array[2];
}


void NIFLoader::handleNiTriShape(NiTriShape *shape, int flags, BoundsFinder &bounds)
{
    assert(shape != NULL);

    // Interpret flags
    bool hidden    = (flags & 0x01) != 0; // Not displayed
    bool collide   = (flags & 0x02) != 0; // Use mesh for collision
    bool bbcollide = (flags & 0x04) != 0; // Use bounding box for collision

    // Bounding box collision isn't implemented, always use mesh for now.
    if (bbcollide)
    {
        collide = true;
        bbcollide = false;
    }

    // If the object was marked "NCO" earlier, it shouldn't collide with
    // anything.
    if (flags & 0x800)
    {
        collide = false;
        bbcollide = false;
    }

    if (!collide && !bbcollide && hidden)
        // This mesh apparently isn't being used for anything, so don't
        // bother setting it up.
        return;

    // Material name for this submesh, if any
    String material;

    // Skip the entire material phase for hidden nodes
    if (!hidden)
    {
        // These are set below if present
        NiTexturingProperty *t = NULL;
        NiMaterialProperty *m = NULL;
        NiAlphaProperty *a = NULL;

        // Scan the property list for material information
        PropertyList &list = shape->props;
        int n = list.length();
        for (int i=0; i<n; i++)
        {
            // Entries may be empty
            if (!list.has(i)) continue;

            Property *pr = &list[i];

            if (pr->recType == RC_NiTexturingProperty)
                t = (NiTexturingProperty*)pr;
            else if (pr->recType == RC_NiMaterialProperty)
                m = (NiMaterialProperty*)pr;
            else if (pr->recType == RC_NiAlphaProperty)
                a = (NiAlphaProperty*)pr;
        }

        // Texture
        String texName;
        if (t && t->textures[0].inUse)
        {
            NiSourceTexture *st = t->textures[0].texture.getPtr();
            if (st->external)
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
        if (a)
        {
            alphaFlags = a->flags;
            alphaTest  = a->data->threshold;
        }

        // Material
        if (m || !texName.empty())
        {
            // If we're here, then this mesh has a material. Thus we
            // need to calculate a snappy material name. It should
            // contain the mesh name (mesh->getName()) but also has to
            // be unique. One mesh may use many materials.
            material = getUniqueName(mesh->getName());

            if (m)
            {
                // Use NiMaterialProperty data to create the data
                const S_MaterialProperty *d = m->data;

                std::multimap<std::string,std::string>::iterator itr = MaterialMap.find(texName);
                std::multimap<std::string,std::string>::iterator lastElement;
                lastElement = MaterialMap.upper_bound(texName);
                if (itr != MaterialMap.end())
                {
                    for ( ; itr != lastElement; ++itr)
                    {
                        //std::cout << "OK!";
                        //MaterialPtr mat = MaterialManager::getSingleton().getByName(itr->second,recourceGroup);
                        material = itr->second;
                        //if( mat->getA
                    }
                }
                else
                {
                    //std::cout << "new";
                    createMaterial(material, d->ambient, d->diffuse, d->specular, d->emissive,
                                    d->glossiness, d->alpha, alphaFlags, alphaTest, texName);
                    MaterialMap.insert(std::make_pair(texName,material));
                }
            }
            else
            {
                // We only have a texture name. Create a default
                // material for it.
                Vector zero, one;
                for (int i=0; i<3;i++)
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

    std::list<VertexBoneAssignment> vertexBoneAssignments;

    //use niskindata for the position of vertices.
    if (!shape->skin.empty())
    {
        // vector that stores if the position if a vertex is absolute
        std::vector<bool> vertexPosAbsolut(numVerts,false);

        float *ptrNormals = (float*)data->normals.ptr;
        //the bone from skin->bones[boneIndex] is linked to skin->data->bones[boneIndex]
        //the first one contains a link to the bone, the second vertex transformation
        //relative to the bone
        int boneIndex = 0;
        Bone *bonePtr;
        Vector3 vecPos;
        Quaternion vecRot;

        std::vector<NiSkinData::BoneInfo> boneList = shape->skin->data->bones;

        /*
        Iterate through the boneList which contains what vertices are linked to
        the bone (it->weights array) and at what position (it->trafo)
        That position is added to every vertex.
        */
        for (std::vector<NiSkinData::BoneInfo>::iterator it = boneList.begin();
                it != boneList.end(); it++)
        {
            if(mSkel.isNull())
            {
                std::cout << "No skeleton for :" << shape->skin->bones[boneIndex].name.toString() << std::endl;
                break;
            }
            //get the bone from bones array of skindata
            bonePtr = mSkel->getBone(shape->skin->bones[boneIndex].name.toString());

            // final_vector = old_vector + old_rotation*new_vector*old_scale
            vecPos = bonePtr->_getDerivedPosition() +
                bonePtr->_getDerivedOrientation() * convertVector3(it->trafo->trans);

            vecRot = bonePtr->_getDerivedOrientation() * convertRotation(it->trafo->rotation);

            for (unsigned int i=0; i<it->weights.length; i++)
            {
                unsigned int verIndex = (it->weights.ptr + i)->vertex;

                //Check if the vertex is relativ, FIXME: Is there a better solution?
                if (vertexPosAbsolut[verIndex] == false)
                {
                    //apply transformation to the vertices
                    Vector3 absVertPos = vecPos + vecRot * Vector3(ptr + verIndex *3);

                    //convert it back to float *
                    for (int j=0; j<3; j++)
                        (ptr + verIndex*3)[j] = absVertPos[j];

                    //apply rotation to the normals (not every vertex has a normal)
                    //FIXME: I guessed that vertex[i] = normal[i], is that true?
                    if (verIndex < data->normals.length)
                    {
                        Vector3 absNormalsPos = vecRot * Vector3(ptrNormals + verIndex *3);

                        for (int j=0; j<3; j++)
                            (ptrNormals + verIndex*3)[j] = absNormalsPos[j];
                    }

                    vertexPosAbsolut[verIndex] = true;
                }

                VertexBoneAssignment vba;
                vba.boneIndex = bonePtr->getHandle();
                vba.vertexIndex = verIndex;
                vba.weight = (it->weights.ptr + i)->weight;

                vertexBoneAssignments.push_back(vba);
            }

            boneIndex++;
        }
    }
    else
    {
        // Rotate, scale and translate all the vertices,
        const Matrix &rot = shape->trafo->rotation;
        const Vector &pos = shape->trafo->pos;
        float scale = shape->trafo->scale;
        for (int i=0; i<numVerts; i++)
        {
            vectorMulAdd(rot, pos, ptr, scale);
            ptr += 3;
        }

        // Remember to rotate all the vertex normals as well
        if (data->normals.length)
        {
            ptr = (float*)data->normals.ptr;
            for (int i=0; i<numVerts; i++)
            {
                vectorMul(rot, ptr);
                ptr += 3;
            }
        }
    }

    if (!hidden)
    {
        // Add this vertex set to the bounding box
        bounds.add(optr, numVerts);

        // Create the submesh
        createOgreSubMesh(shape, material, vertexBoneAssignments);
    }
}

void NIFLoader::handleNode(Nif::Node *node, int flags,
                           const Transformation *trafo, BoundsFinder &bounds, Bone *parentBone)
{
    stack++;
    //if( MWClass::isChest)
    //  cout << "u:" << node << "\n";
    // Accumulate the flags from all the child nodes. This works for all
    // the flags we currently use, at least.
    flags |= node->flags;

    // Check for extra data
    Extra *e = node;
    while (!e->extra.empty())
    {
        // Get the next extra data in the list
        e = e->extra.getPtr();
        assert(e != NULL);

        if (e->recType == RC_NiStringExtraData)
        {
            // String markers may contain important information
            // affecting the entire subtree of this node
            NiStringExtraData *sd = (NiStringExtraData*)e;

            if (sd->string == "NCO")
                // No collision. Use an internal flag setting to mark this.
                flags |= 0x800;
            else if (sd->string == "MRK")
                // Marker objects. These are only visible in the
                // editor. Until and unless we add an editor component to
                // the engine, just skip this entire node.
                return;
        }
    }

    Bone *bone = 0;

    // create skeleton or add bones
    if (node->recType == RC_NiNode)
    {
        //FIXME: "Bip01" isn't every time the root bone
        if (node->name == "Bip01" || node->name == "Root Bone")  //root node, create a skeleton
        {
            mSkel = SkeletonManager::getSingleton().create(getSkeletonName(), resourceGroup, true);

            /*if (node->extra->recType == RC_NiTextKeyExtraData )
            {
                //TODO: Get animation names
                std::cout << node->name.toString() << " is root bone and has textkeyextradata!\n";
            }*/
        }

        if (!mSkel.isNull())     //if there is a skeleton
        {
            std::string name = node->name.toString();
            //if (isBeast && isChest)
            //  std::cout << "NAME: " << name << "\n";
            // Quick-n-dirty workaround for the fact that several
            // bones may have the same name.
            if(!mSkel->hasBone(name))
            {
                bone = mSkel->createBone(name);

                if (parentBone)
                  parentBone->addChild(bone);

                bone->setInheritOrientation(true);
                bone->setPosition(convertVector3(node->trafo->pos));
                bone->setOrientation(convertRotation(node->trafo->rotation));
            }
        }
    }

    // Apply the parent transformation to this node. We overwrite the
    // existing data with the final transformation.
    if (trafo)
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
    if (node->recType == RC_NiNode)
    {
        NodeList &list = ((NiNode*)node)->children;
        int n = list.length();
        int i = 0;
        if(isHands){
            //cout << "NumberOfNs: " << n << "Stack:" << stack << "\n";
            //if(stack == 3)
                //n=0;
        }
        for (; i<n; i++)
        {

            if (list.has(i))
                handleNode(&list[i], flags, node->trafo, bounds, bone);
        }
    }
    else if (node->recType == RC_NiTriShape)
    {
    // For shapes
        /*For Beast Skins, Shape Bone Names
        Tri Left Foot
        Tri Right Foot
        Tri Tail
        Tri Chest
        */
        if((isChest && stack < 10 )  || (isHands && counter < 3) || !(isChest || isHands)){                       //(isBeast && isChest && stack < 10 && counter == skincounter )

            std::string name = node->name.toString();
            //if (isChest)
                //std::cout << "NAME: " << name << "\n";

            if(isChest && isBeast && skincounter == 0 && name.compare("Tri Chest") == 0){
                //std::cout <<"BEASTCHEST1\n";
                handleNiTriShape(dynamic_cast<NiTriShape*>(node), flags, bounds);
                skincounter++;
            }
            else if(isChest && isBeast && skincounter == 1 && name.compare("Tri Tail") == 0){
                //std::cout <<"BEASTCHEST2\n";
                handleNiTriShape(dynamic_cast<NiTriShape*>(node), flags, bounds);
                skincounter++;
            }
            else if(isChest && isBeast && skincounter == 2 && name.compare("Tri Left Foot") == 0){
                //std::cout <<"BEASTCHEST3\n";
                handleNiTriShape(dynamic_cast<NiTriShape*>(node), flags, bounds);
                skincounter=1000;
            }
            else if (!isChest || !isBeast)
            {
                handleNiTriShape(dynamic_cast<NiTriShape*>(node), flags, bounds);
            }
            //if(isBeast && isChest)
                //cout << "Handling Shape, Stack " << stack <<"\n";



                counter++;
        }
        /*if(isHands){
            //cout << "Handling Shape, Stack " << stack <<"\n";
            counter++;
        }*/

    }

    stack--;
}

void NIFLoader::loadResource(Resource *resource)
{
    if(skincounter == 1000)
        skincounter = 0;
    stack = 0;
    counter = 0;
    std::string name = resource->getName();
    if(resourceName.compare(name) != 0)
    {
        skincounter = 0;
        resourceName = name;
    }
    //std::cout <<"NAME:" << name;
    //if(name.length() >= 20)
    //  {std::string split = name.substr(name.length() - 20, 20);
    //if(name ==
    //std::cout <<"NAME:" << name << "LEN: " << name.length() << "\n";
    const std::string test ="meshes\\b\\B_N_Dark Elf_M_Skins.NIF";
    const std::string test2 ="meshes\\b\\B_N_Dark Elf_M_Skins.nif";
    const std::string test3 ="meshes\\b\\B_N_Redguard_F_Skins.NIF";
    const std::string test4 ="meshes\\b\\B_N_Redguard_F_Skins.nif";
    const std::string test5 ="meshes\\b\\B_N_Dark Elf_F_Skins.nif";
    const std::string test6 ="meshes\\b\\B_N_Redguard_M_Skins.nif";
    const std::string test7 ="meshes\\b\\B_N_Wood Elf_F_Skins.nif";
    const std::string test8 ="meshes\\b\\B_N_Wood Elf_M_Skins.nif";
    const std::string test9 ="meshes\\b\\B_N_Imperial_F_Skins.nif";
    const std::string test10 ="meshes\\b\\B_N_Imperial_M_Skins.nif";
    const std::string test11 ="meshes\\b\\B_N_Khajiit_F_Skins.nif";
    const std::string test12 ="meshes\\b\\B_N_Khajiit_M_Skins.nif";
    const std::string test13 ="meshes\\b\\B_N_Argonian_F_Skins.nif";
    const std::string test14 ="meshes\\b\\B_N_Argonian_M_Skins.nif";
    const std::string test15 ="meshes\\b\\B_N_Nord_F_Skins.nif";
    const std::string test16 ="meshes\\b\\B_N_Nord_M_Skins.nif";
    const std::string test17 ="meshes\\b\\B_N_Imperial_F_Skins.nif";
    const std::string test18 ="meshes\\b\\B_N_Imperial_M_Skins.nif";
    const std::string test19 ="meshes\\b\\B_N_Orc_F_Skins.nif";
    const std::string test20 ="meshes\\b\\B_N_Orc_M_Skins.nif";
    const std::string test21 ="meshes\\b\\B_N_Breton_F_Skins.nif";
    const std::string test22 ="meshes\\b\\B_N_Breton_M_Skins.nif";
    const std::string test23 ="meshes\\b\\B_N_High Elf_F_Skins.nif";
    const std::string test24 ="meshes\\b\\B_N_High Elf_M_Skins.nif";

    //std::cout <<"LEN1:" << test.length() << "TEST: " << test << "\n";


    if(name.compare(test) == 0 || name.compare(test2) == 0 || name.compare(test3) == 0 || name.compare(test4) == 0 ||
        name.compare(test5) == 0 || name.compare(test6) == 0 || name.compare(test7) == 0 || name.compare(test8) == 0 || name.compare(test9) == 0 ||
        name.compare(test10) == 0 || name.compare(test11) == 0 || name.compare(test12) == 0 || name.compare(test13) == 0 ||
        name.compare(test14) == 0 || name.compare(test15) == 0 || name.compare(test16) == 0 || name.compare(test17) == 0 ||
        name.compare(test18) == 0 || name.compare(test19) == 0 || name.compare(test20) == 0 || name.compare(test21) == 0 ||
        name.compare(test22) == 0 || name.compare(test23) == 0 || name.compare(test24) == 0

        ){
        //std::cout << "Welcome Chest\n";
        isChest = true;
        if(name.compare(test11) == 0 || name.compare(test12) == 0 || name.compare(test13) == 0 || name.compare(test14) == 0)
        {
            isBeast = true;
            //std::cout << "Welcome Beast\n";
        }
        else
            isBeast = false;
    }
    else
        isChest = false;
    const std::string hands ="meshes\\b\\B_N_Dark Elf_M_Hands.1st.NIF";
    const std::string hands2 ="meshes\\b\\B_N_Dark Elf_F_Hands.1st.NIF";
    const std::string hands3 ="meshes\\b\\B_N_Redguard_M_Hands.1st.nif";
    const std::string hands4 ="meshes\\b\\B_N_Redguard_F_Hands.1st.nif";
    const std::string hands5 ="meshes\\b\\b_n_argonian_m_hands.1st.nif";
    const std::string hands6 ="meshes\\b\\b_n_argonian_f_hands.1st.nif";
    const std::string hands7 ="meshes\\b\\B_N_Breton_M_Hand.1st.NIF";
    const std::string hands8 ="meshes\\b\\B_N_Breton_F_Hands.1st.nif";
    const std::string hands9 ="meshes\\b\\B_N_High Elf_M_Hands.1st.nif";
    const std::string hands10 ="meshes\\b\\B_N_High Elf_F_Hands.1st.nif";
    const std::string hands11 ="meshes\\b\\B_N_Nord_M_Hands.1st.nif";
    const std::string hands12 ="meshes\\b\\B_N_Nord_F_Hands.1st.nif";
    const std::string hands13 ="meshes\\b\\b_n_khajiit_m_hands.1st.nif";
    const std::string hands14 ="meshes\\b\\b_n_khajiit_f_hands.1st.nif";
    const std::string hands15 ="meshes\\b\\B_N_Orc_M_Hands.1st.nif";
    const std::string hands16 ="meshes\\b\\B_N_Orc_F_Hands.1st.nif";
    const std::string hands17 ="meshes\\b\\B_N_Wood Elf_M_Hands.1st.nif";
    const std::string hands18 ="meshes\\b\\B_N_Wood Elf_F_Hands.1st.nif";
    const std::string hands19 ="meshes\\b\\B_N_Imperial_M_Hands.1st.nif";
    const std::string hands20 ="meshes\\b\\B_N_Imperial_F_Hands.1st.nif";
    if(name.compare(hands) == 0 || name.compare(hands2) == 0 || name.compare(hands3) == 0 || name.compare(hands4) == 0 ||
        name.compare(hands5) == 0 || name.compare(hands6) == 0 || name.compare(hands7) == 0 || name.compare(hands8) == 0 ||
        name.compare(hands9) == 0 || name.compare(hands10) == 0 || name.compare(hands11) == 0 || name.compare(hands12) == 0 ||
        name.compare(hands13) == 0 || name.compare(hands14) == 0 || name.compare(hands15) == 0 || name.compare(hands16) == 0 ||
        name.compare(hands17) == 0 || name.compare(hands18) == 0 || name.compare(hands19) == 0 || name.compare(hands20) == 0)
    {
        //std::cout << "Welcome Hands1st\n";
        isHands = true;
        isChest = false;
    }
    else
        isHands = false;


    /*
    else if(name.compare(test3) == 0 || name.compare(test4) == 0)
    {
        std::cout << "\n\n\nWelcome FRedguard Chest\n\n\n";
        isChest = true;
    }
    else if(name.compare(test5) == 0 || name.compare(test6) == 0)
    {
        std::cout << "\n\n\nWelcome FRedguard Chest\n\n\n";
        isChest = true;
    }
    else if(name.compare(test7) == 0 || name.compare(test8) == 0)
    {
        std::cout << "\n\n\nWelcome FRedguard Chest\n\n\n";
        isChest = true;
    }
    else if(name.compare(test9) == 0 || name.compare(test10) == 0)
    {
        std::cout << "\n\n\nWelcome FRedguard Chest\n\n\n";
        isChest = true;
    }*/

    //if(split== "Skins.NIF")
    //  std::cout << "\nSPECIAL PROPS\n";
    resourceName = "";
    // Check if the resource already exists
    //MeshPtr ptr = m->load(name, "custom");
    //cout << "THISNAME: " << ptr->getName() << "\n";
    //cout << "RESOURCE:"<< resource->getName();
    mesh = 0;
    mSkel.setNull();

    // Set up the VFS if it hasn't been done already
    if (!vfs) vfs = new OgreVFS(resourceGroup);

    // Get the mesh
    mesh = dynamic_cast<Mesh*>(resource);
    assert(mesh);

    // Look it up
    resourceName = mesh->getName();
    //std::cout << resourceName << "\n";

    if (!vfs->isFile(resourceName))
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
    NIFFile nif(vfs->open(resourceName), resourceName);

    if (nif.numRecords() < 1)
    {
        warn("Found no records in NIF.");
        return;
    }

    // The first record is assumed to be the root node
    Record *r = nif.getRecord(0);
    assert(r != NULL);

    Nif::Node *node = dynamic_cast<Nif::Node*>(r);

    if (node == NULL)
    {
        warn("First record in file was not a node, but a " +
             r->recName.toString() + ". Skipping file.");
        return;
    }

    // Handle the node
    handleNode(node, 0, NULL, bounds, 0);

    // set the bounding value.
    if (bounds.isValid())
    {
        mesh->_setBounds(AxisAlignedBox(bounds.minX(), bounds.minY(), bounds.minZ(),
                                        bounds.maxX(), bounds.maxY(), bounds.maxZ()));
        mesh->_setBoundingSphereRadius(bounds.getRadius());
    }

    // set skeleton
//     if (!skel.isNull())
//         mesh->setSkeletonName(getSkeletonName());
}

MeshPtr NIFLoader::load(const std::string &name,
                         const std::string &group)
{
    MeshManager *m = MeshManager::getSingletonPtr();
    // Check if the resource already exists
    ResourcePtr ptr = m->getByName(name, group);
    MeshPtr resize;

    const std::string beast1 ="meshes\\b\\B_N_Khajiit_F_Skins.nif";
    const std::string beast2 ="meshes\\b\\B_N_Khajiit_M_Skins.nif";
    const std::string beast3 ="meshes\\b\\B_N_Argonian_F_Skins.nif";
    const std::string beast4 ="meshes\\b\\B_N_Argonian_M_Skins.nif";

    const std::string beasttail1 ="tail\\b\\B_N_Khajiit_F_Skins.nif";
    const std::string beasttail2 ="tail\\b\\B_N_Khajiit_M_Skins.nif";
    const std::string beasttail3 ="tail\\b\\B_N_Argonian_F_Skins.nif";
    const std::string beasttail4 ="tail\\b\\B_N_Argonian_M_Skins.nif";

    if (!ptr.isNull()){

        //if(pieces > 1)
            //cout << "It exists\n";
            resize = MeshPtr(ptr);
        //resize->load();
        //resize->reload();
    }
    else // Nope, create a new one.
    {
        resize = MeshManager::getSingleton().createManual(name, group, NIFLoader::getSingletonPtr());
        //cout <<"EXISTING" << name << "\n";

        //if(pieces > 1)
            //cout << "Creating it\n";


        //resize->load();
        //resize->reload();
        //return 0;
         ResourcePtr ptr = m->getByName(name, group);
         resize = MeshPtr(ptr);

        //NIFLoader::getSingletonPtr()->
        /*ResourcePtr ptr = m->getByName(name, group);
    if (!ptr.isNull()){
        if(pieces > 1)
            cout << "It exists\n";
        resize = MeshPtr(ptr);*/
        //return resize;
    }
    return resize;
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
      name,         // name
      "General",    // group
      TEX_TYPE_2D,      // type
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


*/

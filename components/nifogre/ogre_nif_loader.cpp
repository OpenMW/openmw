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
    
    if(flip)
	{
		float *datamod = new float[data->vertices.length];
		//std::cout << "Shape" << shape->name.toString() << "\n";
		//std::cout << "MTransform" << mTransform << "\n";
		for(int i = 0; i < numVerts; i++)
		{
			int index = i * 3;
			const float *pos = data->vertices.ptr + index;
		    Ogre::Vector3 original = Ogre::Vector3(*pos  ,*(pos+1), *(pos+2));
			//std::cout << "Original: " << original;
			//rstd::cout << "vectorfirst" << original << "\n";
			original = mTransform * original;
			mBoundingBox.merge(original);
			//std::cout <<" New: " << original << "\n";
			datamod[index] = original.x;
			datamod[index+1] = original.y;
			datamod[index+2] = original.z;
			//std::cout << "vector " << original << "\n";
			//std::cout << "datamod: " << datamod[index+1] << "datamod2: " << *(pos+1) << "\n";
		}
		 vbuf->writeData(0, vbuf->getSizeInBytes(), datamod, false);
	}
	else
	{
		//std::cout << "X " << data->vertices.ptr[0] << "Y " << data->vertices.ptr[1] << "Z " << data->vertices.ptr[2] << "NIFLOADER" <<  "\n";
		vbuf->writeData(0, vbuf->getSizeInBytes(), data->vertices.ptr, false);
	}
    
    vbuf->writeData(0, vbuf->getSizeInBytes(), data->vertices.ptr, true);

    VertexBufferBinding* bind = sub->vertexData->vertexBufferBinding;
    bind->setBinding(nextBuf++, vbuf);

     // Vertex normals
    if (data->normals.length)
    {
        decl->addElement(nextBuf, 0, VET_FLOAT3, VES_NORMAL);
        vbuf = HardwareBufferManager::getSingleton().createVertexBuffer(
                   VertexElement::getTypeSize(VET_FLOAT3),
                   numVerts, HardwareBuffer::HBU_DYNAMIC,true);
		
		if(flip)
		{
			Quaternion rotation = mTransform.extractQuaternion();
			rotation.normalise();

			float *datamod = new float[data->normals.length];
			for(int i = 0; i < numVerts; i++)
		    {
			     int index = i * 3;
			     const float *pos = data->normals.ptr + index;
		        Ogre::Vector3 original = Ogre::Vector3(*pos  ,*(pos+1), *(pos+2));
				original = rotation * original;
				 if (mNormaliseNormals)
				{
                original.normalise();
				 }


			    datamod[index] = original.x;
			    datamod[index+1] = original.y;
			    datamod[index+2] = original.z;
		    }
			vbuf->writeData(0, vbuf->getSizeInBytes(), datamod, false);
		}
		else
		{
            vbuf->writeData(0, vbuf->getSizeInBytes(), data->normals.ptr, false);
		}
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

     if (data->uvlist.length)
    {



        decl->addElement(nextBuf, 0, VET_FLOAT2, VES_TEXTURE_COORDINATES);
        vbuf = HardwareBufferManager::getSingleton().createVertexBuffer(
                   VertexElement::getTypeSize(VET_FLOAT2),
                   numVerts, HardwareBuffer::HBU_STATIC_WRITE_ONLY,false);

		if(flip)
		{
		    float *datamod = new float[data->uvlist.length];
		
		    for(int i = 0; i < data->uvlist.length; i+=2){
			    float x = *(data->uvlist.ptr + i);
			    
			    float y = *(data->uvlist.ptr + i + 1);

			    datamod[i] =x;
				datamod[i + 1] =y;
		    }
			vbuf->writeData(0, vbuf->getSizeInBytes(), datamod, false);
		}
		else
			vbuf->writeData(0, vbuf->getSizeInBytes(), data->uvlist.ptr, false);
        bind->setBinding(nextBuf++, vbuf);
    }

   // Triangle faces - The total number of triangle points
    int numFaces = data->triangles.length;
	
    if (numFaces)
    {
		
		sub->indexData->indexCount = numFaces;
        sub->indexData->indexStart = 0;
        HardwareIndexBufferSharedPtr ibuf = HardwareBufferManager::getSingleton().
                                            createIndexBuffer(HardwareIndexBuffer::IT_16BIT,
                                                              numFaces,
                                                              HardwareBuffer::HBU_STATIC_WRITE_ONLY, true);

		if(flip && mFlipVertexWinding && sub->indexData->indexCount % 3 == 0){
			sub->indexData->indexBuffer = ibuf;

			uint16 *datamod = new uint16[numFaces];
			int index = 0;
			for (size_t i = 0; i < sub->indexData->indexCount; i+=3)
			{

			     const short *pos = data->triangles.ptr + index;
				uint16 i0 = (uint16) *(pos+0);
				uint16 i1 = (uint16) *(pos+1);
				uint16 i2 = (uint16) *(pos+2);

				//std::cout << "i0: " << i0 << "i1: " << i1 << "i2: " << i2 << "\n";


				datamod[index] = i2;
				datamod[index+1] = i1;
				datamod[index+2] = i0;

				index += 3;
			}
			
			 ibuf->writeData(0, ibuf->getSizeInBytes(), datamod, false);

		}
		else
		     ibuf->writeData(0, ibuf->getSizeInBytes(), data->triangles.ptr, false);
        sub->indexData->indexBuffer = ibuf;
        


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


void NIFLoader::handleNiTriShape(NiTriShape *shape, int flags, BoundsFinder &bounds, Transformation original, std::vector<std::string> boneSequence)
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

    Nif::NiTriShapeCopy copy = shape->clone();
	if(!shape->controller.empty())
	{
		//Nif::NiGeomMorpherController* cont = dynamic_cast<Nif::NiGeomMorpherController*> (shape->controller.getPtr());
		Nif::Controller* cont = shape->controller.getPtr();
		if(cont->recType == RC_NiGeomMorpherController)
		{
			Nif::NiGeomMorpherController* morph = dynamic_cast<Nif::NiGeomMorpherController*> (cont);
			copy.morph = morph->data.get();
			copy.morph.setStartTime(morph->timeStart);
			copy.morph.setStopTime(morph->timeStop);
			//std::cout << "Size" << morph->data->getInitialVertices().size() << "\n";

		}

				//std::cout << "We have a controller";
	}
    //use niskindata for the position of vertices.
    if (!shape->skin.empty())
    {
		
		//std::cout << "Skin is not empty\n";
		//Bone assignments are stored in submeshes, so we don't need to copy them
		//std::string triname
		//std::vector<Ogre::Vector3> vertices;
        //std::vector<Ogre::Vector3> normals;
        //std::vector<Nif::NiSkinData::BoneInfoCopy> boneinfo;
		
		
        // vector that stores if the position if a vertex is absolute
        std::vector<bool> vertexPosAbsolut(numVerts,false);
		std::vector<Ogre::Vector3> vertexPosOriginal(numVerts, Ogre::Vector3::ZERO);
		std::vector<Ogre::Vector3> vertexNormalOriginal(numVerts, Ogre::Vector3::ZERO);

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
			if(!mSkel->hasBone(shape->skin->bones[boneIndex].name.toString()))
				std::cout << "We don't have this bone";
            bonePtr = mSkel->getBone(shape->skin->bones[boneIndex].name.toString());

            // final_vector = old_vector + old_rotation*new_vector*old_scale
           

			Nif::NiSkinData::BoneInfoCopy boneinfo;
			boneinfo.trafo.rotation = convertRotation(it->trafo->rotation);
			boneinfo.trafo.trans = convertVector3(it->trafo->trans);
			boneinfo.bonename = shape->skin->bones[boneIndex].name.toString();
            for (unsigned int i=0; i<it->weights.length; i++)
            {
				 vecPos = bonePtr->_getDerivedPosition() +
                bonePtr->_getDerivedOrientation() * convertVector3(it->trafo->trans);

            vecRot = bonePtr->_getDerivedOrientation() * convertRotation(it->trafo->rotation);
                unsigned int verIndex = (it->weights.ptr + i)->vertex;
				boneinfo.weights.push_back(*(it->weights.ptr + i));
                //Check if the vertex is relativ, FIXME: Is there a better solution?
                if (vertexPosAbsolut[verIndex] == false)
                {
                    //apply transformation to the vertices
                    Vector3 absVertPos = vecPos + vecRot * Vector3(ptr + verIndex *3);
					absVertPos = absVertPos * (it->weights.ptr + i)->weight;
					vertexPosOriginal[verIndex] = Vector3(ptr + verIndex *3);

					mBoundingBox.merge(absVertPos);
                    //convert it back to float *
                    for (int j=0; j<3; j++)
                        (ptr + verIndex*3)[j] = absVertPos[j];

                    //apply rotation to the normals (not every vertex has a normal)
                    //FIXME: I guessed that vertex[i] = normal[i], is that true?
                    if (verIndex < data->normals.length)
                    {
                        Vector3 absNormalsPos = vecRot * Vector3(ptrNormals + verIndex *3);
						absNormalsPos = absNormalsPos * (it->weights.ptr + i)->weight;
						vertexNormalOriginal[verIndex] = Vector3(ptrNormals + verIndex *3);
                        
                        for (int j=0; j<3; j++)
                            (ptrNormals + verIndex*3)[j] = absNormalsPos[j];
                    }

                    vertexPosAbsolut[verIndex] = true;
                }
				else
				{
					Vector3 absVertPos = vecPos + vecRot * vertexPosOriginal[verIndex];
					absVertPos = absVertPos * (it->weights.ptr + i)->weight;
					Vector3 old = Vector3(ptr + verIndex *3);
					absVertPos = absVertPos + old;

					mBoundingBox.merge(absVertPos);
                    //convert it back to float *
                    for (int j=0; j<3; j++)
                        (ptr + verIndex*3)[j] = absVertPos[j];

                    //apply rotation to the normals (not every vertex has a normal)
                    //FIXME: I guessed that vertex[i] = normal[i], is that true?
                    if (verIndex < data->normals.length)
                    {
                        Vector3 absNormalsPos = vecRot * vertexNormalOriginal[verIndex];
						absNormalsPos = absNormalsPos * (it->weights.ptr + i)->weight;
						Vector3 oldNormal = Vector3(ptrNormals + verIndex *3);
						absNormalsPos = absNormalsPos + oldNormal;
                        
                        for (int j=0; j<3; j++)
                            (ptrNormals + verIndex*3)[j] = absNormalsPos[j];
                    }
				}


                VertexBoneAssignment vba;
                vba.boneIndex = bonePtr->getHandle();
                vba.vertexIndex = verIndex;
                vba.weight = (it->weights.ptr + i)->weight;
	

                vertexBoneAssignments.push_back(vba);
            }
			copy.boneinfo.push_back(boneinfo);

            boneIndex++;
        }
		
    }
    else
    {
		
			copy.boneSequence = boneSequence;
        // Rotate, scale and translate all the vertices,
        const Matrix &rot = shape->trafo->rotation;
        const Vector &pos = shape->trafo->pos;
        float scale = shape->trafo->scale;

		copy.trafo.trans = convertVector3(original.pos);
		copy.trafo.rotation = convertRotation(original.rotation);
		copy.trafo.scale = original.scale;
		//We don't use velocity for anything yet, so it does not need to be saved
	
		// Computes C = B + AxC*scale
        for (int i=0; i<numVerts; i++)
        {
            vectorMulAdd(rot, pos, ptr, scale);
			Ogre::Vector3 absVertPos = Ogre::Vector3(*(ptr + 3 * i), *(ptr + 3 * i + 1), *(ptr + 3 * i + 2));
			mBoundingBox.merge(absVertPos);
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
		if(!mSkel.isNull() ){
			int boneIndex;
			Ogre::Bone *parentBone = mSkel->getBone(boneSequence[boneSequence.size() - 1]);
			if(parentBone)
				boneIndex = parentBone->getHandle();
			else
				boneIndex = mSkel->getNumBones() - 1;
			for(int i = 0; i < numVerts; i++){
		 VertexBoneAssignment vba;
                vba.boneIndex = boneIndex;
                vba.vertexIndex = i;
                vba.weight = 1;
				 vertexBoneAssignments.push_back(vba);
			}
		}
    }

    if (!hidden)
    {
        // Add this vertex set to the bounding box
        bounds.add(optr, numVerts);
        shapes.push_back(copy);

        // Create the submesh
        createOgreSubMesh(shape, material, vertexBoneAssignments);
    }
}

void NIFLoader::calculateTransform()
{
        // Calculate transform
        Matrix4 transform = Matrix4::IDENTITY;
        transform = Matrix4::getScale(vector) * transform;
        // Check whether we have to flip vertex winding.
        // We do have to, if we changed our right hand base.
        // We can test it by using the cross product from X and Y and see, if it is a non-negative
        // projection on Z. Actually it should be exactly Z, as we don't do non-uniform scaling yet,
        // but the test is cheap either way.
        Matrix3 m3;
        transform.extract3x3Matrix(m3);
		
        if (m3.GetColumn(0).crossProduct(m3.GetColumn(1)).dotProduct(m3.GetColumn(2)) < 0)
        {
        	mFlipVertexWinding = true;
        }

        mTransform = transform;

}
void NIFLoader::handleNode(Nif::Node *node, int flags,
                           const Transformation *trafo, BoundsFinder &bounds, Ogre::Bone *parentBone, std::vector<std::string> boneSequence)
{
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

        if (e->recType == RC_NiTextKeyExtraData){
			Nif::NiTextKeyExtraData* extra =  dynamic_cast<Nif::NiTextKeyExtraData*> (e);
			
			std::vector<Nif::NiTextKeyExtraData::TextKey>::iterator textiter = extra->list.begin();
			//std::ofstream File("Indices" + name + ".txt");
			
			//std::string sample = "uy";
			
			std::string cut = "";
			for(int i = 0; i < name.length();  i++)
			{
				if(!(name.at(i) == '\\' || name.at(i) == '/' || name.at(i) == '>' || name.at(i) == '<' || name.at(i) == '?' || name.at(i) == '*' || name.at(i) == '|' || name.at(i) == ':' || name.at(i) == '"'))
				{
					cut += name.at(i);
				}
			}
			//std::cout << "End" << end;
			
			std::cout << "Outputting " << cut << "\n";

			std::ofstream File("Indices" + cut + ".txt");
	
			/*if(File.is_open())
				std::cout << "We could open\n";
			else
				std::cout << "We could not\n";*/
			for(; textiter != extra->list.end(); textiter++)
			{
				//if(textiter->text.toString().find("Torch") < textiter->text.toString().length())
					//std::cout << "Time: " << textiter->time << " " << textiter->text.toString() << "\n";
				std::string text = textiter->text.toString();
				
				replace(text.begin(), text.end(), '\n', '/');

				text.erase(std::remove(text.begin(), text.end(), '\r'), text.end());
				File << "Time: " << textiter->time << "|" << text << "\n";
				
				textmappings[text] = textiter->time;
			}
			File.close();
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
            boneSequence.push_back(name);
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
    Transformation original = *(node->trafo);
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
        for (int i = 0; i<n; i++)
        {

            if (list.has(i))
                handleNode(&list[i], flags, node->trafo, bounds, bone, boneSequence);
        }
    }
    else if (node->recType == RC_NiTriShape)
    {
         std::string nodename = node->name.toString();
		
			if (triname == "")
            {
                handleNiTriShape(dynamic_cast<NiTriShape*>(node), flags, bounds, original, boneSequence);
            }
			else if(name.length() >= triname.length())
			{
				std::transform(nodename.begin(), nodename.end(), nodename.begin(), std::tolower);
				if(triname == name.substr(0, triname.length()))
					handleNiTriShape(dynamic_cast<NiTriShape*>(node), flags, bounds, original, boneSequence);
			}
    }
}

void NIFLoader::loadResource(Resource *resource)
{
   mBoundingBox.setNull();
    mesh = 0;
    mSkel.setNull();
    flip = false;
    name = resource->getName();
    char suffix = name.at(name.length() - 2);

        if(suffix == '*')
		{
			vector = Ogre::Vector3(-1,1,1);
			flip = true;
		}
		else if(suffix == '?'){
			vector = Ogre::Vector3(1,-1,1);
			flip = true;
		}
		else if(suffix == '<'){
			vector = Ogre::Vector3(1,1,-1);
			flip = true;
		}
		else if(suffix == '>')
		{

			//bNiTri = false;
			//baddin = true;
			std::string sub = name.substr(name.length() - 6, 4);
			//if(sub.compare("0000") != 0)
			//	addAnim = false;
				
		}
       
       switch(name.at(name.length() - 1))
	{
	    case '"':
			triname = "tri chest";
			break;
		case '*':
			triname = "tri tail";
			break;
		case ':':
			triname = "tri left foot";
			break;
		case '<':
			triname = "tri right foot";
			break;
		case '>':
			triname = "tri left hand";
			break;
		case '?':
			triname = "tri right hand";
			break;
		default:
			triname = "";
			break;
	}
    if(flip)
	{
		//std::cout << "Flipping";
		calculateTransform();
	}
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
	std::vector<std::string> boneSequence;
	
	

    handleNode(node, 0, NULL, bounds, 0, boneSequence);

    // set the bounding value.
    if (bounds.isValid())
    {
        mesh->_setBounds(AxisAlignedBox(bounds.minX(), bounds.minY(), bounds.minZ(),
                                        bounds.maxX(), bounds.maxY(), bounds.maxZ()));
        mesh->_setBoundingSphereRadius(bounds.getRadius());
    }

     if (!mSkel.isNull())
    {
       mesh->_notifySkeleton(mSkel);
    }
}

MeshPtr NIFLoader::load(const std::string &name,
                         const std::string &group)
{
    
    MeshManager *m = MeshManager::getSingletonPtr();
    // Check if the resource already exists
    ResourcePtr ptr = m->getByName(name, group);
    MeshPtr themesh;
    if (!ptr.isNull()){
            themesh = MeshPtr(ptr);
    }
    else // Nope, create a new one.
    {
        themesh = MeshManager::getSingleton().createManual(name, group, NIFLoader::getSingletonPtr());
    }
    return themesh;
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

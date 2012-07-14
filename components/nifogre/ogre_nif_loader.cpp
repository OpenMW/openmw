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

#include <OgreMaterialManager.h>
#include <OgreMeshManager.h>
#include <OgreHardwareBufferManager.h>
#include <OgreSkeletonManager.h>
#include <OgreTechnique.h>
#include <OgreSubMesh.h>
#include <OgreRoot.h>

#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>

#include <extern/shiny/Main/Factory.hpp>

#include <components/settings/settings.hpp>
#include <components/nifoverrides/nifoverrides.hpp>

typedef unsigned char ubyte;

using namespace std;
using namespace Nif;
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

void NIFLoader::setOutputAnimFiles(bool output){
    mOutputAnimFiles = output;
}
void NIFLoader::setVerbosePath(std::string path){
    verbosePath = path;
}
void NIFLoader::createMaterial(const Ogre::String &name,
                           const Ogre::Vector3 &ambient,
                           const Ogre::Vector3 &diffuse,
                           const Ogre::Vector3 &specular,
                           const Ogre::Vector3 &emissive,
                           float glossiness, float alpha,
                           int alphaFlags, float alphaTest,
                           const String &texName, bool vertexColor)
{
    if (texName.empty())
        return;

    sh::MaterialInstance* instance = sh::Factory::getInstance ().createMaterialInstance (name, "openmw_objects_base");
    instance->setProperty ("ambient", sh::makeProperty<sh::Vector3> (
        new sh::Vector3(ambient.array[0], ambient.array[1], ambient.array[2])));

    instance->setProperty ("diffuse", sh::makeProperty<sh::Vector4> (
        new sh::Vector4(diffuse.array[0], diffuse.array[1], diffuse.array[2], alpha)));

    instance->setProperty ("specular", sh::makeProperty<sh::Vector4> (
        new sh::Vector4(specular.array[0], specular.array[1], specular.array[2], glossiness)));

    instance->setProperty ("emissive", sh::makeProperty<sh::Vector3> (
        new sh::Vector3(emissive.array[0], emissive.array[1], emissive.array[2])));

    instance->setProperty ("diffuseMap", sh::makeProperty(texName));

    if (vertexColor)
        instance->setProperty ("has_vertex_colour", sh::makeProperty<sh::BooleanValue>(new sh::BooleanValue(true)));

    // Add transparency if NiAlphaProperty was present
    if (alphaFlags != -1)
    {
        // The 237 alpha flags are by far the most common. Check
        // NiAlphaProperty in nif/property.h if you need to decode
        // other values. 237 basically means normal transparencly.
        if (alphaFlags == 237)
        {
            NifOverrides::TransparencyResult result = NifOverrides::Overrides::getTransparencyOverride(texName);
            if (result.first)
            {
                instance->setProperty("alpha_rejection",
                    sh::makeProperty<sh::StringValue>(new sh::StringValue("greater_equal " + boost::lexical_cast<std::string>(result.second))));
            }
            else
            {
                // Enable transparency
                instance->setProperty("scene_blend", sh::makeProperty<sh::StringValue>(new sh::StringValue("alpha_blend")));
                instance->setProperty("depth_write", sh::makeProperty<sh::StringValue>(new sh::StringValue("off")));
            }
        }
        else
            warn("Unhandled alpha setting for texture " + texName);
    }
    else
        instance->getMaterial ()->setShadowCasterMaterial ("openmw_shadowcaster_noalpha");

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
}

// Takes a name and adds a unique part to it. This is just used to
// make sure that all materials are given unique names.
Ogre::String NIFLoader::getUniqueName(const Ogre::String &input)
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
void NIFLoader::findRealTexture(Ogre::String &texName)
{
    if(Ogre::ResourceGroupManager::getSingleton().resourceExistsInAnyGroup(texName))
        return;

    // Change texture extension to .dds
    Ogre::String::size_type pos = texName.rfind('.');
    texName.replace(pos, texName.length(), ".dds");
}

//Handle node at top

// Convert Nif::NiTriShape to Ogre::SubMesh, attached to the given
// mesh.
void NIFLoader::createOgreSubMesh(NiTriShape *shape, const Ogre::String &material, std::list<Ogre::VertexBoneAssignment> &vertexBoneAssignments)
{
    //  cout << "s:" << shape << "\n";
    NiTriShapeData *data = shape->data.getPtr();
    Ogre::SubMesh *sub = mesh->createSubMesh(shape->name);

    int nextBuf = 0;

    // This function is just one long stream of Ogre-barf, but it works
    // great.

    // Add vertices
    int numVerts = data->vertices.size() / 3;
    sub->vertexData = new Ogre::VertexData();
    sub->vertexData->vertexCount = numVerts;
    sub->useSharedVertices = false;

    Ogre::VertexDeclaration *decl = sub->vertexData->vertexDeclaration;
    decl->addElement(nextBuf, 0, Ogre::VET_FLOAT3, Ogre::VES_POSITION);

    Ogre::HardwareVertexBufferSharedPtr vbuf =
        Ogre::HardwareBufferManager::getSingleton().createVertexBuffer(
            Ogre::VertexElement::getTypeSize(Ogre::VET_FLOAT3),
            numVerts, Ogre::HardwareBuffer::HBU_DYNAMIC_WRITE_ONLY, false);

    if(flip)
	{
		float *datamod = new float[data->vertices.size()];
		//std::cout << "Shape" << shape->name.toString() << "\n";
		for(int i = 0; i < numVerts; i++)
		{
			int index = i * 3;
			const float *pos = &data->vertices[index];
		    Ogre::Vector3 original = Ogre::Vector3(*pos  ,*(pos+1), *(pos+2));
			original = mTransform * original;
			mBoundingBox.merge(original);
			datamod[index] = original.x;
			datamod[index+1] = original.y;
			datamod[index+2] = original.z;
		}
        vbuf->writeData(0, vbuf->getSizeInBytes(), datamod, false);
        delete [] datamod;
	}
	else
	{
		vbuf->writeData(0, vbuf->getSizeInBytes(), &data->vertices[0], false);
	}


    Ogre::VertexBufferBinding* bind = sub->vertexData->vertexBufferBinding;
    bind->setBinding(nextBuf++, vbuf);

    if (data->normals.size())
    {
        decl->addElement(nextBuf, 0, Ogre::VET_FLOAT3, Ogre::VES_NORMAL);
        vbuf = Ogre::HardwareBufferManager::getSingleton().createVertexBuffer(
                   Ogre::VertexElement::getTypeSize(Ogre::VET_FLOAT3),
                   numVerts, Ogre::HardwareBuffer::HBU_STATIC_WRITE_ONLY, false);

		if(flip)
		{
			Ogre::Quaternion rotation = mTransform.extractQuaternion();
			rotation.normalise();

			float *datamod = new float[data->normals.size()];
			for(int i = 0; i < numVerts; i++)
		    {
			    int index = i * 3;
			    const float *pos = &data->normals[index];
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
            delete [] datamod;
		}
		else
		{
            vbuf->writeData(0, vbuf->getSizeInBytes(), &data->normals[0], false);
		}
        bind->setBinding(nextBuf++, vbuf);
    }


    // Vertex colors
    if (data->colors.size())
    {
        const float *colors = &data->colors[0];
        Ogre::RenderSystem* rs = Ogre::Root::getSingleton().getRenderSystem();
        std::vector<Ogre::RGBA> colorsRGB(numVerts);
        Ogre::RGBA *pColour = &colorsRGB.front();
        for (int i=0; i<numVerts; i++)
        {
            rs->convertColourValue(Ogre::ColourValue(colors[0],colors[1],colors[2],
                                                     colors[3]),pColour++);
            colors += 4;
        }
        decl->addElement(nextBuf, 0, Ogre::VET_COLOUR, Ogre::VES_DIFFUSE);
        vbuf = Ogre::HardwareBufferManager::getSingleton().createVertexBuffer(
                   Ogre::VertexElement::getTypeSize(Ogre::VET_COLOUR),
                   numVerts, Ogre::HardwareBuffer::HBU_STATIC_WRITE_ONLY);
        vbuf->writeData(0, vbuf->getSizeInBytes(), &colorsRGB.front(), true);
        bind->setBinding(nextBuf++, vbuf);
    }

    if (data->uvlist.size())
    {

        decl->addElement(nextBuf, 0, Ogre::VET_FLOAT2, Ogre::VES_TEXTURE_COORDINATES);
        vbuf = Ogre::HardwareBufferManager::getSingleton().createVertexBuffer(
                   Ogre::VertexElement::getTypeSize(Ogre::VET_FLOAT2),
                   numVerts, Ogre::HardwareBuffer::HBU_STATIC_WRITE_ONLY,false);

		if(flip)
		{
		    float *datamod = new float[data->uvlist.size()];

		    for(unsigned int i = 0; i < data->uvlist.size(); i+=2){
			    float x = data->uvlist[i];

			    float y = data->uvlist[i + 1];

			    datamod[i] =x;
				datamod[i + 1] =y;
		    }
			vbuf->writeData(0, vbuf->getSizeInBytes(), datamod, false);
            delete [] datamod;
		}
		else
			vbuf->writeData(0, vbuf->getSizeInBytes(), &data->uvlist[0], false);
        bind->setBinding(nextBuf++, vbuf);
    }

   // Triangle faces - The total number of triangle points
    int numFaces = data->triangles.size();
    if (numFaces)
    {

		sub->indexData->indexCount = numFaces;
        sub->indexData->indexStart = 0;
        Ogre::HardwareIndexBufferSharedPtr ibuf = Ogre::HardwareBufferManager::getSingleton().
                                                  createIndexBuffer(Ogre::HardwareIndexBuffer::IT_16BIT, numFaces,
                                                                    Ogre::HardwareBuffer::HBU_STATIC_WRITE_ONLY, true);

		if(flip && mFlipVertexWinding && sub->indexData->indexCount % 3 == 0){

			sub->indexData->indexBuffer = ibuf;

			uint16_t *datamod = new uint16_t[numFaces];
			int index = 0;
			for (size_t i = 0; i < sub->indexData->indexCount; i+=3)
			{

			     const short *pos = &data->triangles[index];
				uint16_t i0 = (uint16_t) *(pos+0);
				uint16_t i1 = (uint16_t) *(pos+1);
				uint16_t i2 = (uint16_t) *(pos+2);

				//std::cout << "i0: " << i0 << "i1: " << i1 << "i2: " << i2 << "\n";


				datamod[index] = i2;
				datamod[index+1] = i1;
				datamod[index+2] = i0;

				index += 3;
			}

            ibuf->writeData(0, ibuf->getSizeInBytes(), datamod, false);
            delete [] datamod;

		}
		else
            ibuf->writeData(0, ibuf->getSizeInBytes(), &data->triangles[0], false);
        sub->indexData->indexBuffer = ibuf;
    }

    // Set material if one was given
    if (!material.empty()) sub->setMaterialName(material);

    //add vertex bone assignments

    for (std::list<Ogre::VertexBoneAssignment>::iterator it = vertexBoneAssignments.begin();
        it != vertexBoneAssignments.end(); it++)
    {
            sub->addBoneAssignment(*it);
    }
    if(mSkel.isNull())
       needBoneAssignments.push_back(sub);
}

// Helper math functions. Reinventing linear algebra for the win!

// Computes C = B + AxC*scale
static void vectorMulAdd(const Ogre::Matrix3 &A, const Ogre::Vector3 &B, float *C, float scale)
{
    // Keep the original values
    float a = C[0];
    float b = C[1];
    float c = C[2];

    // Perform matrix multiplication, scaling and addition
    for (int i=0;i<3;i++)
        C[i] = B[i] + (a*A[i][0] + b*A[i][1] + c*A[i][2])*scale;
}

// Computes B = AxB (matrix*vector)
static void vectorMul(const Ogre::Matrix3 &A, float *C)
{
    // Keep the original values
    float a = C[0];
    float b = C[1];
    float c = C[2];

    // Perform matrix multiplication, scaling and addition
    for (int i=0;i<3;i++)
        C[i] = a*A[i][0] + b*A[i][1] + c*A[i][2];
}


void NIFLoader::handleNiTriShape(NiTriShape *shape, int flags, BoundsFinder &bounds, Transformation original, std::vector<std::string> boneSequence)
{
    assert(shape != NULL);

    bool saveTheShape = inTheSkeletonTree;
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
    Ogre::String material;

    // Skip the entire material phase for hidden nodes
    if (!hidden)
    {
        // These are set below if present
        NiTexturingProperty *t = NULL;
        NiMaterialProperty *m = NULL;
        NiAlphaProperty *a = NULL;
        // can't make any sense of these values, so ignoring them for now
        //NiVertexColorProperty *v = NULL;

        // Scan the property list for material information
        PropertyList &list = shape->props;
        int n = list.length();
        for (int i=0; i<n; i++)
        {
            // Entries may be empty
            if (!list.has(i)) continue;

            Property *pr = &list[i];

            if (pr->recType == RC_NiTexturingProperty)
                t = static_cast<NiTexturingProperty*>(pr);
            else if (pr->recType == RC_NiMaterialProperty)
                m = static_cast<NiMaterialProperty*>(pr);
            else if (pr->recType == RC_NiAlphaProperty)
                a = static_cast<NiAlphaProperty*>(pr);
            //else if (pr->recType == RC_NiVertexColorProperty)
                //v = static_cast<NiVertexColorProperty*>(pr);
        }

        // Texture
        Ogre::String texName;
        if (t && t->textures[0].inUse)
        {
            NiSourceTexture *st = t->textures[0].texture.getPtr();
            if (st->external)
            {
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
                texName = "textures\\" + st->filename;
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
            alphaTest  = a->data.threshold;
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
                const S_MaterialProperty *d = &m->data;

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
                                   d->glossiness, d->alpha, alphaFlags, alphaTest, texName, shape->data->colors.length != 0);
                    MaterialMap.insert(std::make_pair(texName,material));
                }
            }
            else
            {
                // We only have a texture name. Create a default
                // material for it.
                const Ogre::Vector3 zero(0.0f), one(1.0f);
                createMaterial(material, one, one, zero, zero, 0.0f, 1.0f,
                               alphaFlags, alphaTest, texName, shape->data->colors.length != 0);
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
    int numVerts = data->vertices.size() / 3;

    float *ptr = (float*)&data->vertices[0];
    float *optr = ptr;

    std::list<Ogre::VertexBoneAssignment> vertexBoneAssignments;

    Nif::NiTriShapeCopy copy = shape->clone();

	if(!shape->controller.empty())
	{
		Nif::Controller* cont = shape->controller.getPtr();
		if(cont->recType == RC_NiGeomMorpherController)
		{
			Nif::NiGeomMorpherController* morph = dynamic_cast<Nif::NiGeomMorpherController*> (cont);
			copy.morph = morph->data.get();
			copy.morph.setStartTime(morph->timeStart);
			copy.morph.setStopTime(morph->timeStop);
            saveTheShape = true;
		}

	}
    //use niskindata for the position of vertices.
    if (!shape->skin.empty())
    {



        // vector that stores if the position of a vertex is absolute
        std::vector<bool> vertexPosAbsolut(numVerts,false);
		std::vector<Ogre::Vector3> vertexPosOriginal(numVerts, Ogre::Vector3::ZERO);
		std::vector<Ogre::Vector3> vertexNormalOriginal(numVerts, Ogre::Vector3::ZERO);

        float *ptrNormals = (float*)&data->normals[0];
        //the bone from skin->bones[boneIndex] is linked to skin->data->bones[boneIndex]
        //the first one contains a link to the bone, the second vertex transformation
        //relative to the bone
        int boneIndex = 0;
        Ogre::Bone *bonePtr;
        Ogre::Vector3 vecPos;
        Ogre::Quaternion vecRot;

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
                std::cout << "No skeleton for :" << shape->skin->bones[boneIndex].name << std::endl;
                break;
            }
            //get the bone from bones array of skindata
			if(!mSkel->hasBone(shape->skin->bones[boneIndex].name))
				std::cout << "We don't have this bone";
            bonePtr = mSkel->getBone(shape->skin->bones[boneIndex].name);

            // final_vector = old_vector + old_rotation*new_vector*old_scale


			Nif::NiSkinData::BoneInfoCopy boneinfocopy;
			boneinfocopy.trafo.rotation = it->trafo.rotation;
			boneinfocopy.trafo.trans = it->trafo.trans;
			boneinfocopy.bonename = shape->skin->bones[boneIndex].name;
            boneinfocopy.bonehandle = bonePtr->getHandle();
            copy.boneinfo.push_back(boneinfocopy);
            for (unsigned int i=0; i<it->weights.size(); i++)
            {
				 vecPos = bonePtr->_getDerivedPosition() +
                bonePtr->_getDerivedOrientation() * it->trafo.trans;

            vecRot = bonePtr->_getDerivedOrientation() * it->trafo.rotation;
                unsigned int verIndex = it->weights[i].vertex;
				//boneinfo.weights.push_back(*(it->weights.ptr + i));
                Nif::NiSkinData::IndividualWeight ind;
                ind.weight = it->weights[i].weight;
                ind.boneinfocopyindex = copy.boneinfo.size() - 1;
                if(copy.vertsToWeights.find(verIndex) == copy.vertsToWeights.end())
                {
                    std::vector<Nif::NiSkinData::IndividualWeight> blank;
                    blank.push_back(ind);
                    copy.vertsToWeights[verIndex] = blank;
                }
                else
                {
                    copy.vertsToWeights[verIndex].push_back(ind);
                }

                //Check if the vertex is relativ, FIXME: Is there a better solution?
                if (vertexPosAbsolut[verIndex] == false)
                {
                    //apply transformation to the vertices
                    Ogre::Vector3 absVertPos = vecPos + vecRot * Ogre::Vector3(ptr + verIndex *3);
					absVertPos = absVertPos * it->weights[i].weight;
					vertexPosOriginal[verIndex] = Ogre::Vector3(ptr + verIndex *3);

					mBoundingBox.merge(absVertPos);
                    //convert it back to float *
                    for (int j=0; j<3; j++)
                        (ptr + verIndex*3)[j] = absVertPos[j];

                    //apply rotation to the normals (not every vertex has a normal)
                    //FIXME: I guessed that vertex[i] = normal[i], is that true?
                    if (verIndex < data->normals.size())
                    {
                        Ogre::Vector3 absNormalsPos = vecRot * Ogre::Vector3(ptrNormals + verIndex *3);
						absNormalsPos = absNormalsPos * it->weights[i].weight;
						vertexNormalOriginal[verIndex] = Ogre::Vector3(ptrNormals + verIndex *3);

                        for (int j=0; j<3; j++)
                            (ptrNormals + verIndex*3)[j] = absNormalsPos[j];
                    }

                    vertexPosAbsolut[verIndex] = true;
                }
				else
				{
					Ogre::Vector3 absVertPos = vecPos + vecRot * vertexPosOriginal[verIndex];
					absVertPos = absVertPos * it->weights[i].weight;
					Ogre::Vector3 old = Ogre::Vector3(ptr + verIndex *3);
					absVertPos = absVertPos + old;

					mBoundingBox.merge(absVertPos);
                    //convert it back to float *
                    for (int j=0; j<3; j++)
                        (ptr + verIndex*3)[j] = absVertPos[j];

                    //apply rotation to the normals (not every vertex has a normal)
                    //FIXME: I guessed that vertex[i] = normal[i], is that true?
                    if (verIndex < data->normals.size())
                    {
                        Ogre::Vector3 absNormalsPos = vecRot * vertexNormalOriginal[verIndex];
						absNormalsPos = absNormalsPos * it->weights[i].weight;
						Ogre::Vector3 oldNormal = Ogre::Vector3(ptrNormals + verIndex *3);
						absNormalsPos = absNormalsPos + oldNormal;

                        for (int j=0; j<3; j++)
                            (ptrNormals + verIndex*3)[j] = absNormalsPos[j];
                    }
				}


                Ogre::VertexBoneAssignment vba;
                vba.boneIndex = bonePtr->getHandle();
                vba.vertexIndex = verIndex;
                vba.weight = it->weights[i].weight;


                vertexBoneAssignments.push_back(vba);
            }


            boneIndex++;
        }


    }
    else
    {

			copy.boneSequence = boneSequence;
        // Rotate, scale and translate all the vertices,
        const Ogre::Matrix3 &rot = shape->trafo.rotation;
        const Ogre::Vector3 &pos = shape->trafo.pos;
        float scale = shape->trafo.scale;

		copy.trafo.trans = original.pos;
		copy.trafo.rotation = original.rotation;
		copy.trafo.scale = original.scale;
		//We don't use velocity for anything yet, so it does not need to be saved

		// Computes C = B + AxC*scale
        for (int i=0; i<numVerts; i++)
        {
            vectorMulAdd(rot, pos, ptr, scale);
			Ogre::Vector3 absVertPos = Ogre::Vector3(ptr);
			mBoundingBox.merge(absVertPos);
            ptr += 3;
        }

        // Remember to rotate all the vertex normals as well
        if (data->normals.size())
        {
            ptr = (float*)&data->normals[0];
            for (int i=0; i<numVerts; i++)
            {
                vectorMul(rot, ptr);
                ptr += 3;
            }
        }
		if(!mSkel.isNull() ){
			int boneIndex;

				boneIndex = mSkel->getNumBones() - 1;
			for(int i = 0; i < numVerts; i++){
		 Ogre::VertexBoneAssignment vba;
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
        if(saveTheShape)
            shapes.push_back(copy);

        // Create the submesh
        createOgreSubMesh(shape, material, vertexBoneAssignments);
    }
}

void NIFLoader::calculateTransform()
{
        // Calculate transform
        Ogre::Matrix4 transform = Ogre::Matrix4::IDENTITY;
        transform = Ogre::Matrix4::getScale(vector) * transform;

        // Check whether we have to flip vertex winding.
        // We do have to, if we changed our right hand base.
        // We can test it by using the cross product from X and Y and see, if it is a non-negative
        // projection on Z. Actually it should be exactly Z, as we don't do non-uniform scaling yet,
        // but the test is cheap either way.
        Ogre::Matrix3 m3;
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

            std::ofstream file;

            if(mOutputAnimFiles){
                std::string cut = "";
                for(unsigned int i = 0; i < name.length();  i++)
                {
                    if(!(name.at(i) == '\\' || name.at(i) == '/' || name.at(i) == '>' || name.at(i) == '<' || name.at(i) == '?' || name.at(i) == '*' || name.at(i) == '|' || name.at(i) == ':' || name.at(i) == '"'))
                    {
                        cut += name.at(i);
                    }
                }

                std::cout << "Outputting " << cut << "\n";

                file.open((verbosePath + "/Indices" + cut + ".txt").c_str());
            }

            for(std::vector<Nif::NiTextKeyExtraData::TextKey>::iterator textiter = extra->list.begin(); textiter != extra->list.end(); textiter++)
            {
                std::string text = textiter->text;

                replace(text.begin(), text.end(), '\n', '/');

                text.erase(std::remove(text.begin(), text.end(), '\r'), text.end());
                std::size_t i = 0;
                while(i < text.length()){
                    while(i < text.length() && text.at(i) == '/' ){
                        i++;
                    }
                    std::size_t first = i;
                    int length = 0;
                    while(i < text.length() && text.at(i) != '/' ){
                        i++;
                        length++;
                    }
                    if(first < text.length()){
                            //length = text.length() - first;
                        std::string sub = text.substr(first, length);

                       if(mOutputAnimFiles)
                            file << "Time: " << textiter->time << "|" << sub << "\n";

                        textmappings[sub] = textiter->time;
                    }
                }
            }
            file.close();
        }
    }

    Ogre::Bone *bone = 0;

    // create skeleton or add bones
    if (node->recType == RC_NiNode)
    {
        //FIXME: "Bip01" isn't every time the root bone
        if (node->name == "Bip01" || node->name == "Root Bone")  //root node, create a skeleton
        {
            inTheSkeletonTree = true;

            mSkel = Ogre::SkeletonManager::getSingleton().create(getSkeletonName(), resourceGroup, true);
        }
        else if (!mSkel.isNull() && !parentBone)
            inTheSkeletonTree = false;

        if (!mSkel.isNull())     //if there is a skeleton
        {
            std::string name = node->name;

            // Quick-n-dirty workaround for the fact that several
            // bones may have the same name.
            if(!mSkel->hasBone(name))
            {
                boneSequence.push_back(name);
                bone = mSkel->createBone(name);

                if (parentBone)
                  parentBone->addChild(bone);

                bone->setInheritOrientation(true);
                bone->setPosition(node->trafo.pos);
                bone->setOrientation(node->trafo.rotation);
            }
        }
    }
    Transformation original = node->trafo;
    // Apply the parent transformation to this node. We overwrite the
    // existing data with the final transformation.
    if (trafo)
    {
        // Get a non-const reference to the node's data, since we're
        // overwriting it. TODO: Is this necessary?
        Transformation &final = node->trafo;

        // For both position and rotation we have that:
        // final_vector = old_vector + old_rotation*new_vector*old_scale
        final.pos = trafo->pos + trafo->rotation*final.pos*trafo->scale;
        final.velocity = trafo->velocity + trafo->rotation*final.velocity*trafo->scale;

        // Merge the rotations together
        final.rotation = trafo->rotation * final.rotation;

        // Scale
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
                handleNode(&list[i], flags, &node->trafo, bounds, bone, boneSequence);
        }
    }
    else if (node->recType == RC_NiTriShape && bNiTri)
    {
         std::string nodename = node->name;

			if (triname == "")
            {
                handleNiTriShape(dynamic_cast<NiTriShape*>(node), flags, bounds, original, boneSequence);
            }
			else if(nodename.length() >= triname.length())
			{
				std::transform(nodename.begin(), nodename.end(), nodename.begin(), ::tolower);
				if(triname == nodename.substr(0, triname.length()))
					handleNiTriShape(dynamic_cast<NiTriShape*>(node), flags, bounds, original, boneSequence);
			}
    }
}

void NIFLoader::loadResource(Ogre::Resource *resource)
{
    inTheSkeletonTree = false;
    	allanim.clear();
	shapes.clear();
    needBoneAssignments.clear();
   // needBoneAssignments.clear();
   mBoundingBox.setNull();
    mesh = 0;
    mSkel.setNull();
    flip = false;
    name = resource->getName();
    char suffix = name.at(name.length() - 2);
    bool addAnim = true;
    bool hasAnim = false;
	bool linkSkeleton = true;
    //bool baddin = false;
    bNiTri = true;
    if(name == "meshes\\base_anim.nif" || name == "meshes\\base_animkna.nif")
    {
        bNiTri = false;
    }

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
            //baddin = true;
			bNiTri = true;
			std::string sub = name.substr(name.length() - 6, 4);

			if(sub.compare("0000") != 0)
			addAnim = false;

		}
		else if(suffix == ':')
		{
            //baddin = true;
			linkSkeleton = false;
			bNiTri = true;
			std::string sub = name.substr(name.length() - 6, 4);

			if(sub.compare("0000") != 0)
			addAnim = false;

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
		calculateTransform();
	}
    // Get the mesh
    mesh = dynamic_cast<Ogre::Mesh*>(resource);
    assert(mesh);

    // Look it up
    resourceName = mesh->getName();
    

    // Helper that computes bounding boxes for us.
    BoundsFinder bounds;

    // Load the NIF. TODO: Wrap this in a try-catch block once we're out
    // of the early stages of development. Right now we WANT to catch
    // every error as early and intrusively as possible, as it's most
    // likely a sign of incomplete code rather than faulty input.
    NIFFile nif(resourceName);
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
             r->recName + ". Skipping file.");
        return;
    }

    // Handle the node
	std::vector<std::string> boneSequence;



    handleNode(node, 0, NULL, bounds, 0, boneSequence);
    if(addAnim)
    {
        for(int i = 0; i < nif.numRecords(); i++)
        {
            Nif::NiKeyframeController *f = dynamic_cast<Nif::NiKeyframeController*>(nif.getRecord(i));

            if(f != NULL)
            {
                hasAnim = true;
                Nif::Node *o = dynamic_cast<Nif::Node*>(f->target.getPtr());
                Nif::NiKeyframeDataPtr data = f->data;

                if (f->timeStart >= 10000000000000000.0f)
                    continue;
                data->setBonename(o->name);
                data->setStartTime(f->timeStart);
                data->setStopTime(f->timeStop);

                allanim.push_back(data.get());
            }
        }
    }
    // set the bounding value.
    if (bounds.isValid())
    {
        mesh->_setBounds(Ogre::AxisAlignedBox(bounds.minX(), bounds.minY(), bounds.minZ(),
                                              bounds.maxX(), bounds.maxY(), bounds.maxZ()));
        mesh->_setBoundingSphereRadius(bounds.getRadius());
    }
    if(hasAnim && addAnim){
        allanimmap[name] = allanim;
        alltextmappings[name] = textmappings;
    }
    if(!mSkel.isNull() && shapes.size() > 0 && addAnim)
    {
        allshapesmap[name] = shapes;

    }

    if(flip){
        mesh->_setBounds(mBoundingBox, false);
    }

     if (!mSkel.isNull() )
    {
        for(std::vector<Ogre::SubMesh*>::iterator iter = needBoneAssignments.begin(); iter != needBoneAssignments.end(); iter++)
        {
            int boneIndex = mSkel->getNumBones() - 1;
		        Ogre::VertexBoneAssignment vba;
                vba.boneIndex = boneIndex;
                vba.vertexIndex = 0;
                vba.weight = 1;


            (*iter)->addBoneAssignment(vba);
        }
		//Don't link on npc parts to eliminate redundant skeletons
		//Will have to be changed later slightly for robes/skirts
		if(linkSkeleton)
			mesh->_notifySkeleton(mSkel);
    }
}





Ogre::MeshPtr NIFLoader::load(const std::string &name, const std::string &group)
{

    Ogre::MeshManager *m = Ogre::MeshManager::getSingletonPtr();
    // Check if the resource already exists
    Ogre::ResourcePtr ptr = m->getByName(name, group);
    Ogre::MeshPtr themesh;
    if (!ptr.isNull()){
            themesh = Ogre::MeshPtr(ptr);
    }
    else // Nope, create a new one.
    {
        themesh = Ogre::MeshManager::getSingleton().createManual(name, group, NIFLoader::getSingletonPtr());
    }
    return themesh;
}

/*
This function shares much of the same code handleShapes() in MWRender::Animation
This function also creates new position and normal buffers for submeshes.
This function points to existing texture and IndexData buffers
*/

std::vector<Nif::NiKeyframeData>* NIFLoader::getAnim(std::string lowername){

        std::map<std::string,std::vector<Nif::NiKeyframeData>,ciLessBoost>::iterator iter = allanimmap.find(lowername);
       std::vector<Nif::NiKeyframeData>* pass = 0;
        if(iter != allanimmap.end())
            pass = &(iter->second);
        return pass;

}
std::vector<Nif::NiTriShapeCopy>* NIFLoader::getShapes(std::string lowername){

        std::map<std::string,std::vector<Nif::NiTriShapeCopy>,ciLessBoost>::iterator iter = allshapesmap.find(lowername);
        std::vector<Nif::NiTriShapeCopy>* pass = 0;
        if(iter != allshapesmap.end())
            pass = &(iter->second);
        return pass;
}

std::map<std::string, float>* NIFLoader::getTextIndices(std::string lowername){
	std::map<std::string,std::map<std::string, float>, ciLessBoost>::iterator iter = alltextmappings.find(lowername);
    std::map<std::string, float>* pass = 0;
		if(iter != alltextmappings.end())
			pass = &(iter->second);
		return pass;
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

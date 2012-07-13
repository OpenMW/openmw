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

#include <components/settings/settings.hpp>
#include <components/nifoverrides/nifoverrides.hpp>

typedef unsigned char ubyte;

using namespace std;
using namespace Nif;
using namespace NifOgre;


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


void NIFLoader::warn(const std::string &msg)
{
    std::cerr << "NIFLoader: Warn:" << msg << "\n";
}

void NIFLoader::fail(const std::string &msg)
{
    std::cerr << "NIFLoader: Fail: "<< msg << std::endl;
    assert(1);
}


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

#if 0
void NIFLoader::createMaterial(const Ogre::String &name,
                           const Ogre::Vector3 &ambient,
                           const Ogre::Vector3 &diffuse,
                           const Ogre::Vector3 &specular,
                           const Ogre::Vector3 &emissive,
                           float glossiness, float alpha,
                           int alphaFlags, float alphaTest,
                           const Ogre::String &texName)
{
    Ogre::MaterialPtr material = Ogre::MaterialManager::getSingleton().create(name, resourceGroup);


    // This assigns the texture to this material. If the texture name is
    // a file name, and this file exists (in a resource directory), it
    // will automatically be loaded when needed. If not (such as for
    // internal NIF textures that we might support later), we should
    // already have inserted a manual loader for the texture.


    if (!texName.empty())
    {
        Ogre::Pass *pass = material->getTechnique(0)->getPass(0);
        /*TextureUnitState *txt =*/
        pass->createTextureUnitState(texName);

        pass->setVertexColourTracking(Ogre::TVC_DIFFUSE);

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
                NifOverrides::TransparencyResult result = NifOverrides::Overrides::getTransparencyOverride(texName);
                if (result.first)
                {
                    pass->setAlphaRejectFunction(Ogre::CMPF_GREATER_EQUAL);
                    pass->setAlphaRejectValue(result.second);
                }
                else
                {
                    // Enable transparency
                    pass->setSceneBlending(Ogre::SBT_TRANSPARENT_ALPHA);

                    //pass->setDepthCheckEnabled(false);
                    pass->setDepthWriteEnabled(false);
                    //std::cout << "alpha 237; material: " << name << " texName: " << texName << std::endl;
                }
            }
            else
                warn("Unhandled alpha setting for texture " + texName);
        }
        else
        {
            material->getTechnique(0)->setShadowCasterMaterial("depth_shadow_caster_noalpha");
        }
    }

    if (Settings::Manager::getBool("enabled", "Shadows"))
    {
        bool split = Settings::Manager::getBool("split", "Shadows");
        const int numsplits = 3;
        for (int i = 0; i < (split ? numsplits : 1); ++i)
        {
            Ogre::TextureUnitState* tu = material->getTechnique(0)->getPass(0)->createTextureUnitState();
            tu->setName("shadowMap" + Ogre::StringConverter::toString(i));
            tu->setContentType(Ogre::TextureUnitState::CONTENT_SHADOW);
            tu->setTextureAddressingMode(Ogre::TextureUnitState::TAM_BORDER);
            tu->setTextureBorderColour(Ogre::ColourValue::White);
        }
    }

    if (Settings::Manager::getBool("shaders", "Objects"))
    {
        material->getTechnique(0)->getPass(0)->setVertexProgram("main_vp");
        material->getTechnique(0)->getPass(0)->setFragmentProgram("main_fp");

        material->getTechnique(0)->getPass(0)->setFog(true); // force-disable fixed function fog, it is calculated in shader
    }

    // Create a fallback technique without shadows and without mrt
    Ogre::Technique* tech2 = material->createTechnique();
    tech2->setSchemeName("Fallback");
    Ogre::Pass* pass2 = tech2->createPass();
    pass2->createTextureUnitState(texName);
    pass2->setVertexColourTracking(Ogre::TVC_DIFFUSE);
    if (Settings::Manager::getBool("shaders", "Objects"))
    {
        pass2->setVertexProgram("main_fallback_vp");
        pass2->setFragmentProgram("main_fallback_fp");
        pass2->setFog(true); // force-disable fixed function fog, it is calculated in shader
    }

    // Add material bells and whistles
    material->setAmbient(ambient[0], ambient[1], ambient[2]);
    material->setDiffuse(diffuse[0], diffuse[1], diffuse[2], alpha);
    material->setSpecular(specular[0], specular[1], specular[2], alpha);
    material->setSelfIllumination(emissive[0], emissive[1], emissive[2]);
    material->setShininess(glossiness);
}
#endif


void NIFLoader::loadResource(Ogre::Resource *resource)
{
    warn("Found no records in NIF.");
}

Ogre::MeshPtr NIFLoader::load(const std::string &name, const std::string &group)
{
    Ogre::MeshManager &meshMgr = Ogre::MeshManager::getSingleton();

    // Check if the resource already exists
    Ogre::MeshPtr themesh = meshMgr.getByName(name, group);
    if(themesh.isNull())
    {
        static NIFLoader loader;
        themesh = meshMgr.createManual(name, group, &loader);
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

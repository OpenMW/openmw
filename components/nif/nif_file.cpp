/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008-2010  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: http://openmw.sourceforge.net/

  This file (nif_file.cpp) is part of the OpenMW package.

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

#include "nif_file.hpp"
#include "record.hpp"
#include "components/misc/stringops.hpp"

#include "extra.hpp"
#include "controlled.hpp"
#include "node.hpp"
#include "property.hpp"
#include "data.hpp"
#include "effect.hpp"
#include "controller.hpp"

#include <iostream>
using namespace std;
using namespace Nif;
using namespace Misc;

/* This file implements functions from the NIFFile class. It is also
   where we stash all the functions we couldn't add as inline
   definitions in the record types.
 */

void NIFFile::parse()
{
  // Check the header string
  std::string head = getString(40);
  if(head.compare(0, 22, "NetImmerse File Format") != 0)
    fail("Invalid NIF header");

  // Get BCD version
  ver = getInt();
  if(ver != VER_MW)
    fail("Unsupported NIF version");

  // Number of records
  int recNum = getInt();
  records.resize(recNum);

  /* The format for 10.0.1.0 seems to be a bit different. After the
     header, it contains the number of records, r (int), just like
     4.0.0.2, but following that it contains a short x, followed by x
     strings. Then again by r shorts, one for each record, giving
     which of the above strings to use to identify the record. After
     this follows two ints (zero?) and then the record data. However
     we do not support or plan to support other versions yet.
  */

  for(int i=0;i<recNum;i++)
    {
      std::string rec = getString();
      //cout << i << ": " << rec.toString() << endl;

      Record *r = NULL;

      /* These are all the record types we know how to read.

         This can be heavily optimized later if needed. For example, a
         hash table or a FSM-based parser could be used to look up
         node names.
      */

      // NiNodes
      if(rec == "NiNode" || rec == "AvoidNode" ||
         rec == "NiBSParticleNode" ||
         rec == "NiBSAnimationNode" ||
         rec == "NiBillboardNode") { r = new NiNode; r->recType = RC_NiNode; }

      // Other nodes
      else if(rec == "NiTriShape") { r = new NiTriShape; r->recType = RC_NiTriShape; }
      else if(rec == "NiRotatingParticles") { r = new NiRotatingParticles; r->recType = RC_NiRotatingParticles; }
      else if(rec == "NiAutoNormalParticles") { r = new NiAutoNormalParticles; r->recType = RC_NiAutoNormalParticles; }
      else if(rec == "NiCamera") { r = new NiCamera; r->recType = RC_NiCamera; }
        else if(rec == "RootCollisionNode"){ r = new NiNode; r->recType = RC_RootCollisionNode; }// a root collision node is exactly like a node
                                                                                                 //that's why there is no need to create a new type

      // Properties
      else if(rec == "NiTexturingProperty") { r = new NiTexturingProperty; r->recType = RC_NiTexturingProperty; }
      else if(rec == "NiMaterialProperty") { r = new NiMaterialProperty; r->recType = RC_NiMaterialProperty; }
      else if(rec == "NiZBufferProperty") { r = new NiZBufferProperty; r->recType = RC_NiZBufferProperty; }
      else if(rec == "NiAlphaProperty") { r = new NiAlphaProperty; r->recType = RC_NiAlphaProperty; }
      else if(rec == "NiVertexColorProperty") { r = new NiVertexColorProperty; r->recType = RC_NiVertexColorProperty; }
      else if(rec == "NiShadeProperty") { r = new NiShadeProperty; r->recType = RC_NiShadeProperty; }
      else if(rec == "NiDitherProperty") { r = new NiDitherProperty; r->recType = RC_NiDitherProperty; }
      else if(rec == "NiWireframeProperty") { r = new NiWireframeProperty; r->recType = RC_NiWireframeProperty; }
      else if(rec == "NiSpecularProperty") { r = new NiSpecularProperty; r->recType = RC_NiSpecularProperty; }

      // Controllers
      else if(rec == "NiVisController") { r = new NiVisController; r->recType = RC_NiVisController; }
      else if(rec == "NiGeomMorpherController") { r = new NiGeomMorpherController; r->recType = RC_NiGeomMorpherController; }
      else if(rec == "NiKeyframeController") { r = new NiKeyframeController; r->recType = RC_NiKeyframeController; }
      else if(rec == "NiAlphaController") { r = new NiAlphaController; r->recType = RC_NiAlphaController; }
      else if(rec == "NiUVController") { r = new NiUVController; r->recType = RC_NiUVController; }
      else if(rec == "NiPathController") { r = new NiPathController; r->recType = RC_NiPathController; }
      else if(rec == "NiMaterialColorController") { r = new NiMaterialColorController; r->recType = RC_NiMaterialColorController; }
      else if(rec == "NiBSPArrayController") { r = new NiBSPArrayController; r->recType = RC_NiBSPArrayController; }
      else if(rec == "NiParticleSystemController") { r = new NiParticleSystemController; r->recType = RC_NiParticleSystemController; }

      // Effects
      else if(rec == "NiAmbientLight" ||
              rec == "NiDirectionalLight") { r = new NiLight; r->recType = RC_NiLight; }
      else if(rec == "NiTextureEffect") { r = new NiTextureEffect; r->recType = RC_NiTextureEffect; }

      // Extra Data
      else if(rec == "NiVertWeightsExtraData") { r = new NiVertWeightsExtraData; r->recType = RC_NiVertWeightsExtraData; }
      else if(rec == "NiTextKeyExtraData") { r = new NiTextKeyExtraData; r->recType = RC_NiTextKeyExtraData; }
      else if(rec == "NiStringExtraData") { r = new NiStringExtraData; r->recType = RC_NiStringExtraData; }

      else if(rec == "NiGravity") { r = new NiGravity; r->recType = RC_NiGravity; }
      else if(rec == "NiPlanarCollider") { r = new NiPlanarCollider; r->recType = RC_NiPlanarCollider; }
      else if(rec == "NiParticleGrowFade") { r = new NiParticleGrowFade; r->recType = RC_NiParticleGrowFade; }
      else if(rec == "NiParticleColorModifier") { r = new NiParticleColorModifier; r->recType = RC_NiParticleColorModifier; }
      else if(rec == "NiParticleRotation") { r = new NiParticleRotation; r->recType = RC_NiParticleRotation; }

      // Data
      else if(rec == "NiFloatData") { r = new NiFloatData; r->recType = RC_NiFloatData; }
      else if(rec == "NiTriShapeData") { r = new NiTriShapeData; r->recType = RC_NiTriShapeData; }
      else if(rec == "NiVisData") { r = new NiVisData; r->recType = RC_NiVisData; }
      else if(rec == "NiColorData") { r = new NiColorData; r->recType = RC_NiColorData; }
      else if(rec == "NiPixelData") { r = new NiPixelData; r->recType = RC_NiPixelData; }
      else if(rec == "NiMorphData") { r = new NiMorphData; r->recType = RC_NiMorphData; }
      else if(rec == "NiKeyframeData") { r = new NiKeyframeData; r->recType = RC_NiKeyframeData; }
      else if(rec == "NiSkinData") { r = new NiSkinData; r->recType = RC_NiSkinData; }
      else if(rec == "NiUVData") { r = new NiUVData; r->recType = RC_NiUVData; }
      else if(rec == "NiPosData") { r = new NiPosData; r->recType = RC_NiPosData; }
      else if(rec == "NiRotatingParticlesData") { r = new NiRotatingParticlesData; r->recType = RC_NiRotatingParticlesData; }
      else if(rec == "NiAutoNormalParticlesData") { r = new NiAutoNormalParticlesData; r->recType = RC_NiAutoNormalParticlesData; }

      // Other
      else if(rec == "NiSequenceStreamHelper") { r = new NiSequenceStreamHelper; r->recType = RC_NiSequenceStreamHelper; }
      else if(rec == "NiSourceTexture") { r = new NiSourceTexture; r->recType = RC_NiSourceTexture; }
      else if(rec == "NiSkinInstance") { r = new NiSkinInstance; r->recType = RC_NiSkinInstance; }

      // Failure
      else
        fail("Unknown record type " + rec);

      assert(r != NULL);
      assert(r->recType != RC_MISSING);
      r->recName = rec;
      records[i] = r;
      r->read(this);

      // Discard tranformations for the root node, otherwise some meshes
      // occasionally get wrong orientation. Only for NiNode-s for now, but
      // can be expanded if needed.
      // This should be rewritten when the method is cleaned up.
      if (0 == i && rec == "NiNode")
      {
          static_cast<Nif::Node*>(r)->trafo = Nif::Transformation::getIdentity();
      }
    }

  /* After the data, the nif contains an int N and then a list of N
     ints following it. This might be a list of the root nodes in the
     tree, but for the moment we ignore it.
   */

  // TODO: Set up kf file here first, if applicable. It needs its own
  // code to link it up with the main NIF structure.

  // Once parsing is done, do post-processing.
  for(int i=0; i<recNum; i++)
    records[i]->post(this);
}

/// \todo move to the write cpp file

void NiSkinInstance::post(NIFFile *nif)
{
    data.post(nif);
    root.post(nif);
    bones.post(nif);

    if(data.empty() || root.empty())
        nif->fail("NiSkinInstance missing root or data");

    size_t bnum = bones.length();
    if(bnum != data->bones.size())
        nif->fail("Mismatch in NiSkinData bone count");

    root->makeRootBone(&data->trafo);

    for(size_t i=0; i<bnum; i++)
    {
        if(bones[i].empty())
            nif->fail("Oops: Missing bone! Don't know how to handle this.");
        bones[i]->makeBone(i, data->bones[i]);
    }
}

Ogre::Matrix4 Node::getLocalTransform()
{
    Ogre::Matrix4 mat4(Ogre::Matrix4::IDENTITY);
    mat4.makeTransform(trafo.pos, Ogre::Vector3(trafo.scale), Ogre::Quaternion(trafo.rotation));
    return mat4;
}

Ogre::Matrix4 Node::getWorldTransform()
{
    if(parent != NULL)
        return parent->getWorldTransform() * getLocalTransform();
    return getLocalTransform();
}

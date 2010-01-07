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

#include "nif_file.h"
#include "record.h"
#include "../tools/stringops.h"

#include "extra.h"
#include "controlled.h"
#include "node.h"
#include "property.h"
#include "data.h"
#include "effect.h"
#include "controller.h"

#include <iostream>
using namespace std;
using namespace Nif;

void NIFFile::parse()
{
  // Check the header string
  const char* head = getString(40);
  if(!begins(head, "NetImmerse File Format"))
    fail("Invalid NIF header");

  // Get BCD version
  ver = getInt();
  if(ver != VER_MW)
    fail("Unsupported NIF version");

  // Number of records
  int recNum = getInt();
  records.resize(recNum);

  for(int i=0;i<recNum;i++)
    {
      SString rec = getString();
      //cout << i << ": " << rec.toString() << endl;

      Record *r;

      /* These are all the record types we know how to read.

         This can be heavily optimized later if needed. For example, a
         hash table or a FSM-based parser could be used to look up
         node names.
      */

      // NiNodes
      if(rec == "NiNode" || rec == "AvoidNode" ||
         rec == "RootCollisionNode" ||
         rec == "NiBSParticleNode" ||
         rec == "NiBSAnimationNode" ||
         rec == "NiBillboardNode") r = new NiNode;

      // Other nodes
      else if(rec == "NiTriShape") r = new NiTriShape;
      else if(rec == "NiRotatingParticles") r = new NiRotatingParticles;
      else if(rec == "NiAutoNormalParticles") r = new NiAutoNormalParticles;
      else if(rec == "NiCamera") r = new NiCamera;

      // Properties
      else if(rec == "NiTexturingProperty") r = new NiTexturingProperty;
      else if(rec == "NiMaterialProperty") r = new NiMaterialProperty;
      else if(rec == "NiZBufferProperty") r = new NiZBufferProperty;
      else if(rec == "NiAlphaProperty") r = new NiAlphaProperty;
      else if(rec == "NiVertexColorProperty") r = new NiVertexColorProperty;
      else if(rec == "NiShadeProperty") r = new NiShadeProperty;
      else if(rec == "NiDitherProperty") r = new NiDitherProperty;
      else if(rec == "NiWireframeProperty") r = new NiWireframeProperty;
      else if(rec == "NiSpecularProperty") r = new NiSpecularProperty;

      // Controllers
      else if(rec == "NiVisController") r = new NiVisController;
      else if(rec == "NiGeomMorpherController") r = new NiGeomMorpherController;
      else if(rec == "NiKeyframeController") r = new NiKeyframeController;
      else if(rec == "NiAlphaController") r = new NiAlphaController;
      else if(rec == "NiUVController") r = new NiUVController;
      else if(rec == "NiPathController") r = new NiPathController;
      else if(rec == "NiMaterialColorController") r = new NiMaterialColorController;
      else if(rec == "NiBSPArrayController") r = new NiBSPArrayController;
      else if(rec == "NiParticleSystemController") r = new NiParticleSystemController;

      // Effects
      else if(rec == "NiAmbientLight" ||
              rec == "NiDirectionalLight") r = new NiLight;
      else if(rec == "NiTextureEffect") r = new NiTextureEffect;

      // Extra Data
      else if(rec == "NiVertWeightsExtraData") r = new NiVertWeightsExtraData;
      else if(rec == "NiTextKeyExtraData") r = new NiTextKeyExtraData;
      else if(rec == "NiStringExtraData") r = new NiStringExtraData;

      else if(rec == "NiGravity") r = new NiGravity;
      else if(rec == "NiPlanarCollider") r = new NiPlanarCollider;
      else if(rec == "NiParticleGrowFade") r = new NiParticleGrowFade;
      else if(rec == "NiParticleColorModifier") r = new NiParticleColorModifier;
      else if(rec == "NiParticleRotation") r = new NiParticleRotation;

      // Data
      else if(rec == "NiFloatData") r = new NiFloatData;
      else if(rec == "NiTriShapeData") r = new NiTriShapeData;
      else if(rec == "NiVisData") r = new NiVisData;
      else if(rec == "NiColorData") r = new NiColorData;
      else if(rec == "NiPixelData") r = new NiPixelData;
      else if(rec == "NiMorphData") r = new NiMorphData;
      else if(rec == "NiKeyframeData") r = new NiKeyframeData;
      else if(rec == "NiSkinData") r = new NiSkinData;
      else if(rec == "NiUVData") r = new NiUVData;
      else if(rec == "NiPosData") r = new NiPosData;
      else if(rec == "NiRotatingParticlesData") r = new NiRotatingParticlesData;
      else if(rec == "NiAutoNormalParticlesData") r = new NiAutoNormalParticlesData;

      // Other
      else if(rec == "NiSequenceStreamHelper") r = new NiSequenceStreamHelper;
      else if(rec == "NiSourceTexture") r = new NiSourceTexture;
      else if(rec == "NiSkinInstance") r = new NiSkinInstance;

      // Failure
      else
        fail("Unknown record type " + rec.toString());

      r->read(this);
    }
}

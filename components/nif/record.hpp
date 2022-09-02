/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008-2010  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: https://openmw.org/

  This file (record.h) is part of the OpenMW package.

  OpenMW is distributed as free software: you can redistribute it
  and/or modify it under the terms of the GNU General Public License
  version 3, as published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  version 3 along with this program. If not, see
  https://www.gnu.org/licenses/ .

 */

#ifndef OPENMW_COMPONENTS_NIF_RECORD_HPP
#define OPENMW_COMPONENTS_NIF_RECORD_HPP

#include <string>

namespace Nif
{

class NIFFile;
class NIFStream;

enum RecordType
{
  RC_MISSING = 0,
  RC_NiNode,
  RC_NiSwitchNode,
  RC_NiLODNode,
  RC_NiFltAnimationNode,
  RC_NiBillboardNode,
  RC_AvoidNode,
  RC_NiCollisionSwitch,
  RC_NiTriShape,
  RC_NiTriStrips,
  RC_NiLines,
  RC_NiParticles,
  RC_NiBSParticleNode,
  RC_NiCamera,
  RC_NiTexturingProperty,
  RC_NiFogProperty,
  RC_NiMaterialProperty,
  RC_NiZBufferProperty,
  RC_NiAlphaProperty,
  RC_NiVertexColorProperty,
  RC_NiShadeProperty,
  RC_NiDitherProperty,
  RC_NiWireframeProperty,
  RC_NiSpecularProperty,
  RC_NiStencilProperty,
  RC_NiVisController,
  RC_NiGeomMorpherController,
  RC_NiKeyframeController,
  RC_NiAlphaController,
  RC_NiRollController,
  RC_NiUVController,
  RC_NiPathController,
  RC_NiMaterialColorController,
  RC_NiBSPArrayController,
  RC_NiParticleSystemController,
  RC_NiFlipController,
  RC_NiBSAnimationNode,
  RC_NiLight,
  RC_NiTextureEffect,
  RC_NiExtraData,
  RC_NiVertWeightsExtraData,
  RC_NiTextKeyExtraData,
  RC_NiStringExtraData,
  RC_NiGravity,
  RC_NiPlanarCollider,
  RC_NiParticleGrowFade,
  RC_NiParticleColorModifier,
  RC_NiParticleRotation,
  RC_NiFloatData,
  RC_NiTriShapeData,
  RC_NiTriStripsData,
  RC_NiLinesData,
  RC_NiVisData,
  RC_NiColorData,
  RC_NiPixelData,
  RC_NiMorphData,
  RC_NiKeyframeData,
  RC_NiSkinData,
  RC_NiUVData,
  RC_NiPosData,
  RC_NiRotatingParticlesData,
  RC_NiParticlesData,
  RC_NiSequenceStreamHelper,
  RC_NiSourceTexture,
  RC_NiSkinInstance,
  RC_RootCollisionNode,
  RC_NiSphericalCollider,
  RC_NiLookAtController,
  RC_NiPalette,
  RC_NiIntegerExtraData,
  RC_NiIntegersExtraData,
  RC_NiBinaryExtraData,
  RC_NiBooleanExtraData,
  RC_NiVectorExtraData,
  RC_NiColorExtraData,
  RC_NiFloatExtraData,
  RC_NiFloatsExtraData,
  RC_NiStringPalette,
  RC_NiBoolData,
  RC_NiSkinPartition,
  RC_BSXFlags,
  RC_BSBound,
  RC_bhkBlendController,
  RC_NiFloatInterpolator,
  RC_NiPoint3Interpolator,
  RC_NiBoolInterpolator,
  RC_NiTransformInterpolator,
  RC_NiColorInterpolator,
  RC_BSShaderTextureSet,
  RC_BSLODTriShape,
  RC_BSShaderProperty,
  RC_BSShaderPPLightingProperty,
  RC_BSShaderNoLightingProperty,
  RC_BSFurnitureMarker,
  RC_NiCollisionObject,
  RC_bhkCollisionObject,
  RC_BSDismemberSkinInstance,
  RC_NiControllerManager,
  RC_bhkMoppBvTreeShape,
  RC_bhkNiTriStripsShape,
  RC_bhkPackedNiTriStripsShape,
  RC_hkPackedNiTriStripsData,
  RC_bhkConvexVerticesShape,
  RC_bhkBoxShape,
  RC_bhkListShape,
  RC_bhkRigidBody,
  RC_bhkRigidBodyT,
  RC_BSLightingShaderProperty,
  RC_NiClusterAccumulator,
  RC_NiAlphaAccumulator,
  RC_NiSortAdjustNode,
  RC_NiMultiTargetTransformController
};

/// Base class for all records
struct Record
{
    // Record type and type name
    RecordType recType{RC_MISSING};
    std::string recName;
    unsigned int recIndex{~0u};

    Record() = default;

    /// Parses the record from file
    virtual void read(NIFStream *nif) = 0;

    /// Does post-processing, after the entire tree is loaded
    virtual void post(NIFFile *nif) {}

    virtual ~Record() {}
};

} // Namespace
#endif

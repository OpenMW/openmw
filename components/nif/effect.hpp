/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008-2010  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: https://openmw.org/

  This file (effect.h) is part of the OpenMW package.

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

#ifndef OPENMW_COMPONENTS_NIF_EFFECT_HPP
#define OPENMW_COMPONENTS_NIF_EFFECT_HPP

#include "node.hpp"

namespace Nif
{

    // Abstract
    struct NiDynamicEffect : public NiAVObject
    {
        bool mSwitchState{ true };
        void read(NIFStream* nif) override;
    };

    // Abstract light source
    struct NiLight : NiDynamicEffect
    {
        float mDimmer;
        osg::Vec3f mAmbient;
        osg::Vec3f mDiffuse;
        osg::Vec3f mSpecular;

        void read(NIFStream* nif) override;
    };

    struct NiPointLight : public NiLight
    {
        float mConstantAttenuation;
        float mLinearAttenuation;
        float mQuadraticAttenuation;

        void read(NIFStream* nif) override;
    };

    struct NiSpotLight : public NiPointLight
    {
        float mOuterSpotAngle;
        float mInnerSpotAngle{ 0.f };
        float mExponent;
        void read(NIFStream* nif) override;
    };

    struct NiTextureEffect : NiDynamicEffect
    {
        enum class TextureType : uint32_t
        {
            ProjectedLight = 0,
            ProjectedShadow = 1,
            EnvironmentMap = 2,
            FogMap = 3,
        };

        enum class CoordGenType : uint32_t
        {
            WorldParallel = 0,
            WorldPerspective = 1,
            SphereMap = 2,
            SpecularCubeMap = 3,
            DiffuseCubeMap = 4,
        };

        Matrix3 mProjectionRotation;
        osg::Vec3f mProjectionPosition;
        uint32_t mFilterMode;
        NiSourceTexturePtr mTexture;
        uint16_t mMaxAnisotropy{ 0 };
        uint32_t mClampMode;
        TextureType mTextureType;
        CoordGenType mCoordGenType;
        uint8_t mEnableClipPlane;
        osg::Plane mClipPlane;

        void read(NIFStream* nif) override;
        void post(Reader& nif) override;

        bool wrapT() const { return mClampMode & 1; }
        bool wrapS() const { return mClampMode & 2; }
    };

} // Namespace
#endif

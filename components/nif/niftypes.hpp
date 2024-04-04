/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008-2010  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: https://openmw.org/

  This file (nif_types.h) is part of the OpenMW package.

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

#ifndef OPENMW_COMPONENTS_NIF_NIFTYPES_HPP
#define OPENMW_COMPONENTS_NIF_NIFTYPES_HPP

#include <osg/Matrixf>
#include <osg/Quat>
#include <osg/Vec3f>

// Common types used in NIF files

namespace Nif
{

    struct Matrix3
    {
        float mValues[3][3];

        Matrix3()
        {
            for (int i = 0; i < 3; ++i)
                for (int j = 0; j < 3; ++j)
                    mValues[i][j] = (i == j) ? 1.f : 0.f;
        }

        bool isIdentity() const
        {
            for (int i = 0; i < 3; ++i)
                for (int j = 0; j < 3; ++j)
                    if ((i == j) != (mValues[i][j] == 1))
                        return false;
            return true;
        }

        osg::Matrixf toOsgMatrix() const
        {
            osg::Matrixf osgMat;

            for (int i = 0; i < 3; ++i)
                for (int j = 0; j < 3; ++j)
                    osgMat(i, j) = mValues[j][i]; // NB: column/row major difference

            return osgMat;
        }
    };

    struct NiTransform
    {
        Matrix3 mRotation; // this can contain scale components too, including negative and nonuniform scales
        osg::Vec3f mTranslation;
        float mScale;

        osg::Matrixf toMatrix() const
        {
            osg::Matrixf transform;
            transform.setTrans(mTranslation);

            for (int i = 0; i < 3; ++i)
                for (int j = 0; j < 3; ++j)
                    transform(j, i) = mRotation.mValues[i][j] * mScale; // NB column/row major difference

            return transform;
        }

        bool isIdentity() const { return mRotation.isIdentity() && mTranslation == osg::Vec3f() && mScale == 1.f; }

        static const NiTransform& getIdentity()
        {
            static const NiTransform identity = { Matrix3(), osg::Vec3f(), 1.0f };
            return identity;
        }
    };

    struct NiQuatTransform
    {
        osg::Vec3f mTranslation;
        osg::Quat mRotation;
        float mScale;

        osg::Matrixf toMatrix() const
        {
            osg::Matrixf transform(mRotation);
            transform.setTrans(mTranslation);
            for (int i = 0; i < 3; i++)
                transform(i, i) *= mScale;

            return transform;
        }

        bool isIdentity() const { return mTranslation == osg::Vec3f() && mRotation == osg::Quat() && mScale == 1.f; }

        static const NiQuatTransform& getIdentity()
        {
            static const NiQuatTransform identity = { osg::Vec3f(), osg::Quat(), 1.f };
            return identity;
        }
    };

} // Namespace
#endif

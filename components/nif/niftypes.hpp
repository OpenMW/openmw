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

#include <osg/Vec3f>
#include <osg/Matrixf>

// Common types used in NIF files

namespace Nif
{

struct Matrix3
{
    float mValues[3][3];

    Matrix3()
    {
        for (int i=0;i<3;++i)
            for (int j=0;j<3;++j)
                mValues[i][j] = (i==j) ? 1.f : 0.f;
    }

    bool isIdentity() const
    {
        for (int i=0;i<3;++i)
            for (int j=0;j<3;++j)
                if ((i==j) != (mValues[i][j] == 1))
                    return false;
        return true;
    }
};

struct Transformation
{
    osg::Vec3f pos;
    Matrix3 rotation; // this can contain scale components too, including negative and nonuniform scales
    float scale;

    osg::Matrixf toMatrix() const
    {
        osg::Matrixf transform;
        transform.setTrans(pos);

        for (int i=0;i<3;++i)
            for (int j=0;j<3;++j)
                transform(j,i) = rotation.mValues[i][j] * scale; // NB column/row major difference

        return transform;
    }

    bool isIdentity() const
    {
        return pos == osg::Vec3f(0,0,0)
                && rotation.isIdentity() && scale == 1.f;
    }

    static const Transformation& getIdentity()
    {
        static const Transformation identity = {
            osg::Vec3f(), Matrix3(), 1.0f
        };
        return identity;
    }
};

} // Namespace
#endif

/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008-2010  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: http://openmw.sourceforge.net/

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
  http://www.gnu.org/licenses/ .

 */

#ifndef OPENMW_COMPONENTS_NIF_NIFTYPES_HPP
#define OPENMW_COMPONENTS_NIF_NIFTYPES_HPP

#include <OgreVector3.h>
#include <OgreMatrix3.h>

// Common types used in NIF files

namespace Nif
{

struct Transformation
{
    Ogre::Vector3 pos;
    Ogre::Matrix3 rotation;
    float scale;

    static const Transformation& getIdentity()
    {
        static const Transformation identity = {
            Ogre::Vector3::ZERO, Ogre::Matrix3::IDENTITY, 1.0f
        };
        return identity;
    }
};

} // Namespace
#endif

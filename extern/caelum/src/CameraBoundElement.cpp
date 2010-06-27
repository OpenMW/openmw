/*
This file is part of Caelum.
See http://www.ogre3d.org/wiki/index.php/Caelum 

Copyright (c) 2006-2008 Caelum team. See Contributors.txt for details.

Caelum is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published
by the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Caelum is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with Caelum. If not, see <http://www.gnu.org/licenses/>.
*/

#include "CaelumPrecompiled.h"
#include "CameraBoundElement.h"

namespace Caelum
{
    const Ogre::Real CameraBoundElement::CAMERA_NEAR_DISTANCE_MULTIPLIER = 10;

    CameraBoundElement::CameraBoundElement():
            mAutoRadius(true)
    {
    }

    CameraBoundElement::~CameraBoundElement()
    {
    }

    void CameraBoundElement::notifyCameraChanged (Ogre::Camera *cam) {
	    if (mAutoRadius) {
            if (cam->getFarClipDistance () > 0) {
                setFarRadius((cam->getFarClipDistance () + cam->getNearClipDistance ()) / 2);
            } else {
                setFarRadius(cam->getNearClipDistance () * CAMERA_NEAR_DISTANCE_MULTIPLIER);
            }
	    }	
    }

    void CameraBoundElement::forceFarRadius (Ogre::Real radius) {
        if (radius > 0) {
            mAutoRadius = false;
            setFarRadius(radius);
        } else {
            mAutoRadius = true;
        }
    }

    bool CameraBoundElement::getAutoRadius () const {
        return mAutoRadius;
    }

    void CameraBoundElement::setAutoRadius () {
        forceFarRadius (-1);
    }

    void CameraBoundElement::setFarRadius(Ogre::Real radius) {
    }
}

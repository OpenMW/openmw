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

#ifndef CAELUM__CAMERA_BOUND_ELEMENT_H
#define CAELUM__CAMERA_BOUND_ELEMENT_H

#include "CaelumPrerequisites.h"

namespace Caelum
{
    /** A camera-bound element.
     *
     *	This should be used as a base class for domes which follow the camera.
     *  It is only meant to be used inside Caelum.
     *
     *  By default this class work in autoradius mode; where it automatically
     *  resizes itself for camera near/far clipping radius. It will correctly
     *  handle infinite far clip planes.
     *
     *  This is meant to be used with depth_check and depth_write off.
     *  Trying to place an object "as far as possible" causes precision
     *  troubles; and was removed in version 0.4.
     *
     *  If far clip distance is finite the radius will be (near + far) / 2.
     *  If far clip distance is infinite (0) the radius will be 10 * near/
     */
    class CAELUM_EXPORT CameraBoundElement
    {
	private:
		/// Defines if the element has an automatic "far" radius or not.
		bool mAutoRadius;
		
	public:
        /** Constructor. Sets auto radius to true.
         */
        CameraBoundElement();

		/// Virtual Destructor.
		virtual ~CameraBoundElement ();

		/** Notify new camera conditions.
		 *  This method notifies that a new camera is about to be used, so
         *  this element can follow it or perform other operations.
         *  The default implementation calls setRadius if in autoRadius mode.
		 *  @param cam The new camera.
		 */
		virtual void notifyCameraChanged (Ogre::Camera *cam) = 0;

		/** Forces the "far" size of the element to a specific radius.
         *
         *  If greater than zero this disables AutoRadius mode and forces a
         *  fixed radius. If this is negative or zero the radius is set
         *  automatically in notifyCameraChanged.
         *
         *  AutoRadius is turned on by default.
         *
		 *  @param radius The positive radius of the element, or a 
         *          negative/zero value for AutoRadius mode.
		 */
        void forceFarRadius (Ogre::Real radius);

        /** Checks if this element is in auto-radius mode.
         *  While in autoradius mode the element is automatically resized fit
         *  between the near and far radius.
         */
        bool getAutoRadius () const;

        /** Re-enable auto-radius; if disabled.
         *  Auto-radius is on by default; but can be disabled. This function
         *  can turn it back on.
         */
        void setAutoRadius ();

        /** Camera distances multiplier for the far clipping distance. 
         *  This threshold will be multiplied with the far clipping distance,
         *  if the camera doesn't use an infinite far clipping plane.
         */
        static const Ogre::Real CAMERA_FAR_DISTANCE_MULTIPLIER;

        /** Camera distances multiplier for the near clipping distance. 
         *  This threshold will be multiplied with the near clipping distance,
         *  if the camera does use an infinite far clipping plane.
         */
        static const Ogre::Real CAMERA_NEAR_DISTANCE_MULTIPLIER;

    protected:
        /** Abstract method to set the radius for this elements
         *  Derived classes should override this and resize their domes.
         *  The actual radius for the dome is controlled in the base class.
         */
        virtual void setFarRadius (Ogre::Real radius);
    };
}

#endif // CAELUM__CAMERA_BOUND_ELEMENT_H

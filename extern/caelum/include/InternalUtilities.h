/*
This file is part of Caelum.
See http://www.ogre3d.org/wiki/index.php/Caelum 

Copyright (c) 2006-2007 Caelum team. See Contributors.txt for details.

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

#ifndef CAELUM_HEADER__PRIVATE_UTILITIES_H
#define CAELUM_HEADER__PRIVATE_UTILITIES_H

#include "CaelumPrerequisites.h"
#include "PrivatePtr.h"

namespace Caelum
{
    /** Private caelum utilities
     *
     *  This class constains various tiny utilities for caelum to use.
     */
    class CAELUM_EXPORT InternalUtilities
    {
    public:
        /** Gets the interpolated colour between two pixels from an image.
            Interpolate a texture pixel by hand. (fx, fy) are in texture coordinates,
            ranging [0-1] across the entire texture.
            Smooth blending is only done on the x coordinate.
            Wrapping is only supported on X as well.

            @param fx Horizontal coordinate.
            @param fy Vertical coordiate.
            @param img The lookup image.
            @param wrapX To wrap the x coordinate.
            @return The interpolated colour.
         */
        static Ogre::ColourValue getInterpolatedColour (
                float fx,
                float fy,
                Ogre::Image *img,
                bool wrapX = true);

        /** Quickly format a pointer as a string; in hex
         */
        static const Ogre::String pointerToString(void* pointer);

        /** Creates a private clone of a material from a script.
         *
         *  When a class wants to modify a material at runtime it must not
         *  modify the original material loaded from scripts. Instead it
         *  should create a clone and use that.
         *
         *  This method throws a Caelum::UnsupportedException on failure.
         *  
         *  @param originalName Name of the original material.
         *  @param cloneName Name of the result clone.
         *
         *  @return A pointer to an unique material.
         */
        static Ogre::MaterialPtr checkLoadMaterialClone (
                const Ogre::String& originalName,
                const Ogre::String& cloneName);

        /** Fetch a compositor by name and check it can be loaded properly
         *
         *  This method throws a Caelum::UnsupportedException on failure.
         *
         *  @param name Name of the compositor to check.
         *
         *  @return A pointer to the compositor (can be ignored)
         */
        static Ogre::CompositorPtr checkCompositorSupported (const Ogre::String& name);

	public:
		/** Enumeration of types of sky domes.
		 */
		enum DomeType {
            DT_SKY_DOME,
            DT_IMAGE_STARFIELD,
        };

		/** Creates a longitude-latitude sky dome.
		 *  @note Does nothing if the sphere already exists.
		 *  @param name The name of the mesh to be created.
		 *  @param segments The number of sphere segments.
		 *  @param domeType The type of dome to create.
		 */
		static void generateSphericDome (const Ogre::String &name, int segments, DomeType domeType);

	private:
		/** Fills the vertex and index buffers for a sky gradients type dome.
		 *  @param pVertex Pointer to the vertex buffer.
		 *  @param pIndices Pointer to the index buffer.
		 *  @param segments Subdivision detail.
		 */
		static void fillGradientsDomeBuffers (float *pVertex, unsigned short *pIndices, int segments);

		/** Fills the vertex and index buffers for a stardield type dome.
		 *  @param pVertex Pointer to the vertex buffer.
		 *  @param pIndices Pointer to the index buffer.
		 *  @param segments Subdivision detail.
		 */
		static void fillStarfieldDomeBuffers (float *pVertex, unsigned short *pIndices, int segments);
    };
}

#endif // CAELUM_HEADER__PRIVATE_UTILITIES_H

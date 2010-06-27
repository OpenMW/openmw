/*
This file is part of Caelum.
See http://www.ogre3d.org/wiki/index.php/Caelum 

Copyright (c) 2008 Caelum team. See Contributors.txt for details.

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

#ifndef CAELUM__MOON_H
#define CAELUM__MOON_H

#include "CaelumPrerequisites.h"
#include "SkyLight.h"
#include "FastGpuParamRef.h"
#include "PrivatePtr.h"

namespace Caelum
{
    /** Class representing the moon.
     *  Drawn as two billboards; one after the stars and one after the skydome.
     *  Drawing it before the skydome will make it invisible in daylight; and that's bad.
     */
    class CAELUM_EXPORT Moon:
            public BaseSkyLight
    {
	public:
		/// Name of the moon material.
		static const Ogre::String MOON_MATERIAL_NAME;

        /// Name of the moon background material.
		static const Ogre::String MOON_BACKGROUND_MATERIAL_NAME;

	private:
        /// Material for MoonBB
		PrivateMaterialPtr mMoonMaterial;

		/// The moon sprite.
		PrivateBillboardSetPtr mMoonBB;

        /// Material for mBackBB
		PrivateMaterialPtr mBackMaterial;
		
        /// The moon's background; used to block the stars.
		PrivateBillboardSetPtr mBackBB;

		/// The moon sprite visible angle
		Ogre::Degree mAngularSize;

        struct Params {
            void setup(Ogre::GpuProgramParametersSharedPtr fpParams);

            Ogre::GpuProgramParametersSharedPtr fpParams;
            FastGpuParamRef phase;
        } mParams;

	public:
		/** Constructor.
		 */
		Moon (
				Ogre::SceneManager *sceneMgr,
				Ogre::SceneNode *caelumRootNode,
				const Ogre::String& moonTextureName = "moon_disc.dds", 
				Ogre::Degree angularSize = Ogre::Degree(3.77f));

		virtual ~Moon ();

		/** Updates the moon material.
			@param textureName The new moon texture name.
		 */
		void setMoonTexture (const Ogre::String &textureName);
		
		/** Updates the moon size.
			@param moon TextureAngularSize The new moon texture angular size.
		 */
		void setMoonTextureAngularSize(const Ogre::Degree& moonTextureAngularSize);

		/** Sets the moon sphere colour.
			@param colour The colour used to draw the moon
		 */
		void setBodyColour (const Ogre::ColourValue &colour);

		/// Set the moon's phase
		void setPhase (Ogre::Real phase);

    public:
		/// Handle camera change.
		virtual void notifyCameraChanged (Ogre::Camera *cam);

        virtual void setQueryFlags (uint flags);
        virtual uint getQueryFlags () const;
        virtual void setVisibilityFlags (uint flags);
        virtual uint getVisibilityFlags () const;
    };
}

#endif // CAELUM__MOON_H

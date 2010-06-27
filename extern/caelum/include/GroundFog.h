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

#ifndef GROUNDFOG_H
#define GROUNDFOG_H

#include "CaelumPrerequisites.h"
#include "CameraBoundElement.h"
#include "PrivatePtr.h"
#include "FastGpuParamRef.h"

namespace Caelum
{
    /** Exponential ground fog system implementation.
     *
     *  This class controls CaelumGroundFog passes in a potentially large number
     *  of materials, changing shader program parameters. This class keeps a list
     *  of passes to control; which can be build based on pass name.
     *
     *  This simulates a field of fog where "absorption" at a certain point is
     *	exp(-verticalDecay * (h - fogLevel)). This absorption is multiplicative,
     *	the total fog alpha is e^(-density * absorption_on_view_path).
     *
     *	You can set verticalDecay to 0 and get standard GL_EXP fog. Don't actually
     *	do that though because you'll get a division by 0.
     *
     *  @note: This is deprecated starting from Caelum 0.4. The DepthComposer class
     *  provides similar functionality with less intrusion on your materials.
     */
    class CAELUM_EXPORT GroundFog: public CameraBoundElement
    {
	public:
		static const Ogre::String DEFAULT_PASS_NAME;

		/** Constructor.
		 */
		GroundFog (Ogre::SceneManager *scene,
			Ogre::SceneNode *caelumRootNode,
			const Ogre::String &domeMaterialName = "CaelumGroundFogDome",
			const Ogre::String &domeEntityName = "CaelumGroundFogDome");

		/** Virtual destructor.
		 */
		virtual ~GroundFog ();

		/** Typedef for easier manipulation of a set of Passes.
		 */
		typedef std::set<Ogre::Pass *> PassSet;

		/** Get the set of currently controlled passes.
		 *	This is provided if you really want to change the set by hand.
		 *  You should call forceUpdate after modifying this set.
		 */
		PassSet& getPasses();

		/** Get the set of currently controlled passes.
		 *  This is a const overload which doesn't let you modify the
		 *	underlying collection.
		 */
		const PassSet& getPasses () const;

		/** Find caelum fog passes to control by name.
		 *	By default this looks for passes called "CaleumGroundFog".
		 *	@note This calls forceUpdate()
		 */
		void findFogPassesByName (const Ogre::String& passName = DEFAULT_PASS_NAME);

		/// Sets the fog density multiplier
		void setDensity (Ogre::Real density);

		/// Get the fog density multiplier
		Ogre::Real getDensity () const;

		/// Sets fog colour
		void setColour (const Ogre::ColourValue &colour);

		/// Gets fog colour
		const Ogre::ColourValue getColour () const;

		/// Sets the vertical fog decay constant.
		void setVerticalDecay (Ogre::Real verticalDecay);

		/// Get the vertical fog decay constant.
		Ogre::Real getVerticalDecay () const;

		/** Sets the ground level.
		 *	At ground level 'fogginess' is equal to 1.
		 */
		void setGroundLevel (Ogre::Real GroundLevela);

		/** Get the ground level.
		 */
		Ogre::Real getGroundLevel () const;

		/** Forces an update of all the passes. You have to use this if you modify
		 *	the set of passes by hand, otherwise avoid it.
		 */
		void forceUpdate ();

	private:
		/// Cached Density
		Ogre::Real mDensity;

		/// Cached VerticalDecay
		Ogre::Real mVerticalDecay;

		/// Cached GroundLevel
		Ogre::Real mGroundLevel;

        /// Fog colour
		Ogre::ColourValue mFogColour;

    private:
		/// The scene to control fog in.
		Ogre::SceneManager* mScene;

		/// Sky dome material
		PrivateMaterialPtr mDomeMaterial;
        
		/// Sky dome node
		PrivateSceneNodePtr mDomeNode;

		/// Sky dome entity
		PrivateEntityPtr mDomeEntity;

		// Called whenever something changes to update the sky dome.
		void updateSkyFogging();

    protected:
        /// Handle far radius.
	    virtual void setFarRadius (Ogre::Real radius);

    public:
		/// Handle camera change.
		virtual void notifyCameraChanged (Ogre::Camera *cam);

        void setQueryFlags (uint flags) { mDomeEntity->setQueryFlags (flags); }
        uint getQueryFlags () const { return mDomeEntity->getQueryFlags (); }
        void setVisibilityFlags (uint flags) { mDomeEntity->setVisibilityFlags (flags); }
        uint getVisibilityFlags () const { return mDomeEntity->getVisibilityFlags (); }

    private:
		/// The passes to control.
		PassSet mPasses;

        /// Params references.
        struct FogParamsBase
        {
            void setup(Ogre::GpuProgramParametersSharedPtr fpParams);

            Ogre::GpuProgramParametersSharedPtr fpParams;

    		FastGpuParamRef fogDensity;
		    FastGpuParamRef fogColour;
		    FastGpuParamRef fogVerticalDecay;
		    FastGpuParamRef fogGroundLevel;

        };

        struct DomeFogParams: public FogParamsBase {
            void setup(Ogre::GpuProgramParametersSharedPtr fpParams);
		    FastGpuParamRef cameraHeight;
        } mDomeParams;

        struct PassFogParams: public FogParamsBase {
            PassFogParams(Ogre::GpuProgramParametersSharedPtr fpParams) { setup(fpParams); }

            static inline bool lessThanByParams(const PassFogParams& a, const PassFogParams& b) {
                return a.fpParams.get() <= b.fpParams.get();
            }

            static inline bool equalByParams(const PassFogParams& a, const PassFogParams& b) {
                return a.fpParams.get() == b.fpParams.get();
            }
        };

        typedef std::vector<PassFogParams> PassFogParamsVector;
        PassFogParamsVector mPassFogParams;

        /// Update mPassFogParams based on mPasses
        void updatePassFogParams();
    };
}

#endif //GROUNDFOG_H

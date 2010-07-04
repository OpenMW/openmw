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

#ifndef CAELUM__CLOUD_SYSTEM_H
#define CAELUM__CLOUD_SYSTEM_H

#include "CaelumPrerequisites.h"

namespace Caelum
{
	/**	A cloud system is implemented by a number of cloud layers.
	 *	Different cloud layers could implement different kinds of clouds (cirrus, stratus).
	 */
	class CAELUM_EXPORT CloudSystem
	{
	public:
		CloudSystem (
					Ogre::SceneManager *sceneMgr,
					Ogre::SceneNode *cloudRoot);

		~CloudSystem();

        typedef std::vector<FlatCloudLayer*> LayerVector;

	private:
		Ogre::SceneManager *mSceneMgr;
		Ogre::SceneNode *mCloudRoot;
		LayerVector mLayers;

    public:
        /** Direct access to the layer vector.
         */
        LayerVector& getLayerVector() { return mLayers; }

        /// Clears all cloud layers.
		void clearLayers();

        /// Create a new cloud layer with default settings at height 0.
        /// @return pointer to the new layer.
        FlatCloudLayer* createLayer();

        /// Create a new cloud layer with default settings at a certain height.
        /// @return pointer to the new layer.
        FlatCloudLayer* createLayerAtHeight(Ogre::Real height);

        /// Add new layer. Takes ownership of the layer.
        void addLayer(FlatCloudLayer* layer);

        /// Get a pointer to a certain layer.
        inline FlatCloudLayer* getLayer(int index) { return mLayers[index]; }

        /// Get the total number of layers.
        inline int getLayerCount() { return static_cast<int> (mLayers.size ()); }

        /** Update function called every frame from high above.
         */
		void update (
                Ogre::Real timePassed,
		        const Ogre::Vector3 &sunDirection,
		        const Ogre::ColourValue &sunLightColour,
		        const Ogre::ColourValue &fogColour,
				const Ogre::ColourValue &sunSphereColour);

        /// Similar to @see CaelumSystem::forceSubcomponentQueryFlags.
        virtual void forceLayerQueryFlags (uint flags);

        /// Similar to @see CaelumSystem::forceSubcomponentVisibilityFlags.
        virtual void forceLayerVisibilityFlags (uint flags);
	};
}

#endif // CAELUM__CLOUD_SYSTEM_H

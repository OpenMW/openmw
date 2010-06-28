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

#include "CaelumPrecompiled.h"
#include "CloudSystem.h"
#include "FlatCloudLayer.h"

using namespace Ogre;

namespace Caelum
{
    CloudSystem::CloudSystem(
		    Ogre::SceneManager *sceneMgr,
		    Ogre::SceneNode *cloudRoot)
    {
        mSceneMgr = sceneMgr;
        mCloudRoot = cloudRoot;
    }

    FlatCloudLayer* CloudSystem::createLayerAtHeight(Ogre::Real height)
    {
        FlatCloudLayer* layer = this->createLayer ();
        layer->setHeight(height);
        return layer;
    }

    FlatCloudLayer* CloudSystem::createLayer()
    {
        std::auto_ptr<FlatCloudLayer> layer(new FlatCloudLayer(mSceneMgr, mCloudRoot));
        mLayers.push_back(layer.get());
        return layer.release();
    }

    void CloudSystem::addLayer(FlatCloudLayer* layer)
    {
        assert(layer != NULL);
        mLayers.push_back(layer);
    }

    void CloudSystem::clearLayers()
    {
	    for (unsigned i = 0; i < mLayers.size(); i++)
        {
		    delete mLayers[i];
		    mLayers[i] = 0;
	    }
    }

    CloudSystem::~CloudSystem()
    {
	    clearLayers ();
    }

    void CloudSystem::update(
		    Ogre::Real timePassed,
		    const Ogre::Vector3 &sunDirection,
		    const Ogre::ColourValue &sunLightColour,
		    const Ogre::ColourValue &fogColour,
		    const Ogre::ColourValue &sunSphereColour)
    {
	    for (uint i = 0; i < mLayers.size(); i++) {
            assert(mLayers[i] != NULL);
		    mLayers[i]->update(timePassed, sunDirection, sunLightColour, fogColour, sunSphereColour);
	    }
    }

    void CloudSystem::forceLayerQueryFlags (uint flags) {
	    for (uint i = 0; i < mLayers.size(); i++) {
		    mLayers[i]->setQueryFlags (flags);
	    }
    }

    void CloudSystem::forceLayerVisibilityFlags (uint flags) {
	    for (uint i = 0; i < mLayers.size(); i++) {
		    mLayers[i]->setVisibilityFlags (flags);
	    }
    }
}

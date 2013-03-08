/*
-----------------------------------------------------------------------------
This source file is part of OGRE
(Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org/

Copyright (c) 2000-2011 Torus Knot Software Ltd

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
-----------------------------------------------------------------------------
*/

#ifndef MWRENDER_TERRAINMATERIAL_H
#define MWRENDER_TERRAINMATERIAL_H

#include "OgreTerrainPrerequisites.h"
#include "OgreTerrainMaterialGenerator.h"
#include "OgreGpuProgramParams.h"

namespace sh
{
    class MaterialInstance;
}

namespace MWRender
{

    class TerrainMaterial : public Ogre::TerrainMaterialGenerator
    {
    public:

        class Profile : public Ogre::TerrainMaterialGenerator::Profile
        {
        public:
            Profile(Ogre::TerrainMaterialGenerator* parent, const Ogre::String& name, const Ogre::String& desc);
            virtual ~Profile();

            virtual bool isVertexCompressionSupported() const { return false; }

            virtual Ogre::MaterialPtr generate(const Ogre::Terrain* terrain);

            virtual Ogre::MaterialPtr generateForCompositeMap(const Ogre::Terrain* terrain);

            virtual Ogre::uint8 getMaxLayers(const Ogre::Terrain* terrain) const;

            virtual void updateParams(const Ogre::MaterialPtr& mat, const Ogre::Terrain* terrain);

            virtual void updateParamsForCompositeMap(const Ogre::MaterialPtr& mat, const Ogre::Terrain* terrain);

            virtual void requestOptions(Ogre::Terrain* terrain);

            void setGlobalColourMapEnabled(bool enabled);
            void setGlobalColourMap (Ogre::Terrain* terrain, const std::string& name);
            virtual void setLightmapEnabled(bool) {}

        private:
            sh::MaterialInstance* mMaterial;

            bool mGlobalColourMap;

        };

        TerrainMaterial();
    };

}


#endif

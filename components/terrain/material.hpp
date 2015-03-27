/*
 * Copyright (c) 2015 scrawl <scrawl@baseoftrash.de>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#ifndef COMPONENTS_TERRAIN_MATERIAL_H
#define COMPONENTS_TERRAIN_MATERIAL_H

#include <OgreMaterial.h>

#include "storage.hpp"

namespace Terrain
{

    class MaterialGenerator
    {
    public:
        MaterialGenerator ();

        void setLayerList (const std::vector<LayerInfo>& layerList) { mLayerList = layerList; }
        bool hasLayers() { return mLayerList.size() > 0; }
        void setBlendmapList (const std::vector<Ogre::TexturePtr>& blendmapList) { mBlendmapList = blendmapList; }
        const std::vector<Ogre::TexturePtr>& getBlendmapList() { return mBlendmapList; }
        void setCompositeMap (const std::string& name) { mCompositeMap = name; }

        void enableShaders(bool shaders) { mShaders = shaders; }
        void enableShadows(bool shadows) { mShadows = shadows; }
        void enableNormalMapping(bool normalMapping) { mNormalMapping = normalMapping; }
        void enableParallaxMapping(bool parallaxMapping) { mParallaxMapping = parallaxMapping; }
        void enableSplitShadows(bool splitShadows) { mSplitShadows = splitShadows; }

        /// Creates a material suitable for displaying a chunk of terrain using alpha-blending.
        Ogre::MaterialPtr generate ();

        /// Creates a material suitable for displaying a chunk of terrain using a ready-made composite map.
        Ogre::MaterialPtr generateForCompositeMap ();

        /// Creates a material suitable for rendering composite maps, i.e. for "baking" several layer textures
        /// into one. The main difference compared to a normal material is that no shading is applied at this point.
        Ogre::MaterialPtr generateForCompositeMapRTT ();

    private:
        Ogre::MaterialPtr create (bool renderCompositeMap, bool displayCompositeMap);

        std::vector<LayerInfo> mLayerList;
        std::vector<Ogre::TexturePtr> mBlendmapList;
        std::string mCompositeMap;
        bool mShaders;
        bool mShadows;
        bool mSplitShadows;
        bool mNormalMapping;
        bool mParallaxMapping;
    };

}

#endif

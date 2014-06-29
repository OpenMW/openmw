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
        bool hasLayers() { return mLayerList.size(); }
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

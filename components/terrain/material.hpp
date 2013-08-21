#ifndef COMPONENTS_TERRAIN_MATERIAL_H
#define COMPONENTS_TERRAIN_MATERIAL_H

#include <OgreMaterial.h>

namespace Terrain
{

    class MaterialGenerator
    {
    public:
        /// @param layerList layer textures
        /// @param blendmapList blend textures
        /// @param shaders Whether to use shaders. With a shader, blendmap packing can be used (4 channels instead of one),
        ///                so if this parameter is true, then the supplied blend maps are expected to be packed.
        MaterialGenerator (bool shaders);

        void setLayerList (const std::vector<std::string>& layerList) { mLayerList = layerList; }
        bool hasLayers() { return mLayerList.size(); }
        void setBlendmapList (const std::vector<Ogre::TexturePtr>& blendmapList) { mBlendmapList = blendmapList; }
        const std::vector<Ogre::TexturePtr>& getBlendmapList() { return mBlendmapList; }
        void setCompositeMap (const std::string& name) { mCompositeMap = name; }

        void enableShadows(bool shadows) { mShadows = shadows; }
        void enableSplitShadows(bool splitShadows) { mSplitShadows = splitShadows; }

        /// Creates a material suitable for displaying a chunk of terrain using alpha-blending.
        /// @param mat Material that will be replaced by the generated material. May be empty as well, in which case
        ///            a new material is created.
        Ogre::MaterialPtr generate (Ogre::MaterialPtr mat);

        /// Creates a material suitable for displaying a chunk of terrain using a ready-made composite map.
        /// @param mat Material that will be replaced by the generated material. May be empty as well, in which case
        ///            a new material is created.
        Ogre::MaterialPtr generateForCompositeMap (Ogre::MaterialPtr mat);

        /// Creates a material suitable for rendering composite maps, i.e. for "baking" several layer textures
        /// into one. The main difference compared to a normal material is that no shading is applied at this point.
        /// @param mat Material that will be replaced by the generated material. May be empty as well, in which case
        ///            a new material is created.
        Ogre::MaterialPtr generateForCompositeMapRTT (Ogre::MaterialPtr mat);

    private:
        Ogre::MaterialPtr create (Ogre::MaterialPtr mat, bool renderCompositeMap, bool displayCompositeMap);

        std::vector<std::string> mLayerList;
        std::vector<Ogre::TexturePtr> mBlendmapList;
        std::string mCompositeMap;
        bool mShaders;
        bool mShadows;
        bool mSplitShadows;
    };

}

#endif

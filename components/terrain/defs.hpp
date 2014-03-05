#ifndef COMPONENTS_TERRAIN_DEFS_HPP
#define COMPONENTS_TERRAIN_DEFS_HPP

namespace Terrain
{
    class QuadTreeNode;

    /// The alignment of the terrain
    enum Alignment
    {
        /// Terrain is in the X/Z plane
        Align_XZ = 0,
        /// Terrain is in the X/Y plane
        Align_XY = 1,
        /// Terrain is in the Y/Z plane.
        /// UNTESTED - use at own risk.
        /// Besides, X as up axis? What is wrong with you? ;)
        Align_YZ = 2
    };

    inline void convertPosition(Alignment align, float &x, float &y, float &z)
    {
        switch (align)
        {
        case Align_XY:
            return;
        case Align_XZ:
            std::swap(y, z);
            // This is since -Z should be going *into* the screen
            // If not doing this, we'd get wrong vertex winding
            z *= -1;
            return;
        case Align_YZ:
            std::swap(x, y);
            std::swap(y, z);
            return;
        }
    }

    enum Direction
    {
        North = 0,
        East = 1,
        South = 2,
        West = 3
    };

    struct LayerInfo
    {
        std::string mDiffuseMap;
        std::string mNormalMap;
        bool mParallax; // Height info in normal map alpha channel?
        bool mSpecular; // Specular info in diffuse map alpha channel?
    };

    struct LayerCollection
    {
        QuadTreeNode* mTarget;
        // Since we can't create a texture from a different thread, this only holds the raw texel data
        std::vector<Ogre::PixelBox> mBlendmaps;
        std::vector<LayerInfo> mLayers;
    };
}

#endif

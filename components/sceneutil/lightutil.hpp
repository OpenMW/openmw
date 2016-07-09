#ifndef OPENMW_COMPONENTS_LIGHTUTIL_H
#define OPENMW_COMPONENTS_LIGHTUTIL_H

namespace osg
{
    class Group;
}

namespace ESM
{
    struct Light;
}

namespace SceneUtil
{

    /// @brief Convert an ESM::Light to a SceneUtil::LightSource, and add it to a sub graph.
    /// @note If the sub graph contains a node named "AttachLight" (case insensitive), then the light is added to that.
    /// Otherwise, the light is added in the center of the node's bounds.
    /// @param node The sub graph to add a light to
    /// @param esmLight The light definition coming from the game files containing radius, color, flicker, etc.
    /// @param partsysMask Node mask to ignore when computing the sub graph's bounding box.
    /// @param lightMask Mask to assign to the newly created LightSource.
    /// @param isExterior Is the light outside? May be used for deciding which attenuation settings to use.
    /// @par Attenuation parameters come from the game INI file.
    void addLight (osg::Group* node, const ESM::Light* esmLight, unsigned int partsysMask, unsigned int lightMask, bool isExterior, bool outQuadInLin, bool useQuadratic,
                   float quadraticValue, float quadraticRadiusMult, bool useLinear, float linearRadiusMult,
                   float linearValue);

}

#endif

#ifndef OPENMW_COMPONENTS_LIGHTUTIL_H
#define OPENMW_COMPONENTS_LIGHTUTIL_H

#include <osg/ref_ptr>
#include <osg/Vec4f>

namespace osg
{
    class Group;
    class Light;
}

namespace ESM
{
    struct Light;
}

namespace SceneUtil
{
    class LightSource;

    /// @brief Set up global attenuation settings for an osg::Light.
    /// @param radius The radius of the light source.
    /// @param isExterior Is the light outside? May be used for deciding which attenuation settings to use.
    void configureLight (osg::Light *light, float radius, bool isExterior);

    /// @brief Convert an ESM::Light to a SceneUtil::LightSource, and add it to a sub graph.
    /// @note If the sub graph contains a node named "AttachLight" (case insensitive), then the light is added to that.
    /// Otherwise, the light is added in the center of the node's bounds.
    /// @param node The sub graph to add a light to
    /// @param esmLight The light definition coming from the game files containing radius, color, flicker, etc.
    /// @param partsysMask Node mask to ignore when computing the sub graph's bounding box.
    /// @param lightMask Mask to assign to the newly created LightSource.
    /// @param isExterior Is the light outside? May be used for deciding which attenuation settings to use.
    void addLight (osg::Group* node, const ESM::Light* esmLight, unsigned int partsysMask, unsigned int lightMask, bool isExterior);

    /// @brief Convert an ESM::Light to a SceneUtil::LightSource, and return it.
    /// @param esmLight The light definition coming from the game files containing radius, color, flicker, etc.
    /// @param lightMask Mask to assign to the newly created LightSource.
    /// @param isExterior Is the light outside? May be used for deciding which attenuation settings to use.
    /// @param ambient Ambient component of the light.
    osg::ref_ptr<LightSource> createLightSource (const ESM::Light* esmLight, unsigned int lightMask, bool isExterior, const osg::Vec4f& ambient=osg::Vec4f(0,0,0,1));

}

#endif

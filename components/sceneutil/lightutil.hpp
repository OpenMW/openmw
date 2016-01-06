#ifndef OPENMW_COMPONENTS_LIGHTUTIL_H
#define OPENMW_COMPONENTS_LIGHTUTIL_H

namespace osg
{
    class Light;
}

namespace SceneUtil
{

    /// @brief Configures a light's attenuation according to vanilla Morrowind attenuation settings.
    void configureLight(osg::Light* light, float radius, bool isExterior, bool outQuadInLin, bool useQuadratic,
                        float quadraticValue, float quadraticRadiusMult, bool useLinear, float linearRadiusMult,
                        float linearValue);

}

#endif

#ifndef OPENMW_COMPONENTS_SCENEUTIL_LIGHTINGMETHOD_H
#define OPENMW_COMPONENTS_SCENEUTIL_LIGHTINGMETHOD_H

namespace SceneUtil
{
    enum class LightingMethod
    {
        FFP,
        PerObjectUniform,
        SingleUBO,
    };
}

#endif

#include "core.h"

#ifdef SH_FRAGMENT_SHADER

    SH_BEGIN_PROGRAM
        shInput(float2, UV)
        shSampler2D(SceneBuffer)
        shUniform(float2, contrast_invGamma) @shSharedParameter(contrast_invGamma)
    SH_START_PROGRAM
    {
        shOutputColour(0) = shSample(SceneBuffer, UV);

        // contrast
        shOutputColour(0).xyz = (shOutputColour(0).xyz - float3(0.5,0.5,0.5)) * contrast_invGamma.x + float3(0.5,0.5,0.5);
        shOutputColour(0).xyz = shSaturate(shOutputColour(0).xyz);
        // gamma
        shOutputColour(0).xyz = pow(shOutputColour(0).xyz, contrast_invGamma.yyy);
    }

#endif

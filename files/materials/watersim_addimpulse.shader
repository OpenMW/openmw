#include "core.h"
#include "watersim_common.h"

    SH_BEGIN_PROGRAM
        shInput(float2, UV)
        shSampler2D(alphaMap)

    SH_START_PROGRAM
    {
		shOutputColour(0) = EncodeHeightmap(1.0);
		shOutputColour(0).a = shSample (alphaMap, UV.xy).a;
    }

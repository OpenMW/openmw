#include "core.h"

#define TEXTURE @shPropertyBool(has_texture)

#ifdef SH_VERTEX_SHADER

    SH_BEGIN_PROGRAM
#if TEXTURE
        shVertexInput(float2, uv0)
        shOutput(float2, UV)
#endif
        shColourInput(float4)
        shOutput(float4, colourPassthrough)

    SH_START_PROGRAM
    {
        shOutputPosition = float4(shInputPosition.xyz, 1.0);
#if TEXTURE
        UV.xy = uv0;
#endif
        colourPassthrough = colour;
    }

#else


    SH_BEGIN_PROGRAM
    
#if TEXTURE
        shSampler2D(diffuseMap)
        shInput(float2, UV)
#endif

        shInput(float4, colourPassthrough)
        
    SH_START_PROGRAM
    {
#if TEXTURE
        shOutputColour(0) = shSample(diffuseMap, UV.xy) * colourPassthrough;
#else
		shOutputColour(0) = colourPassthrough;
#endif
    }

#endif

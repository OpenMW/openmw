#include "core.h"

#define ALPHA @shPropertyBool(shadow_transparency)

#ifdef SH_VERTEX_SHADER

    SH_BEGIN_PROGRAM
#if ALPHA
        shVertexInput(float2, uv0)
        shOutput(float2, UV)
#endif
        shUniform(float4x4, wvp) @shAutoConstant(wvp, worldviewproj_matrix)
        shOutput(float2, depth)
    SH_START_PROGRAM
    {
	    // this is the view space position
	    shOutputPosition = shMatrixMult(wvp, shInputPosition);

	    // depth info for the fragment.
	    depth.x = shOutputPosition.z;
	    depth.y = shOutputPosition.w;

	    // clamp z to zero. seem to do the trick. :-/
	    shOutputPosition.z = max(shOutputPosition.z, 0.0);

#if ALPHA
	    UV = uv0;
#endif
    }

#else

    SH_BEGIN_PROGRAM
#if ALPHA
        shInput(float2, UV)
        shSampler2D(texture1)
#endif
        shInput(float2, depth)
    SH_START_PROGRAM
    {
	    float finalDepth = depth.x / depth.y;


#if ALPHA
        // use alpha channel of the first texture
        float alpha = shSample(texture1, UV).a;

        if (alpha < 0.5)
            discard;
#endif

	    shOutputColour(0) = float4(finalDepth, finalDepth, finalDepth, 1.0);
    }

#endif

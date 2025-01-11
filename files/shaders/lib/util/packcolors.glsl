#ifndef LIB_UTIL_PACKCOLORS
#define LIB_UTIL_PACKCOLORS

#define DEBUG 0
#define USE_SIGN_BIT 1

#define SCENE_PRECISSION 8.0
#define EXTRA_PRECISSION 12.0

#define EXTRA_PRECISSION2 (EXTRA_PRECISSION * EXTRA_PRECISSION)

    void fromFloat(float encoded, inout float a, inout float b, float extraBit)
    {

#if USE_SIGN_BIT
        if(encoded < 0.0)
        {
            encoded *= -1.0;
            extraBit += 0.5;
        }
#endif

        a = floor(encoded) / SCENE_PRECISSION + (extraBit * (1.0 / (EXTRA_PRECISSION * SCENE_PRECISSION)));
        b = encoded - floor(encoded);
    }

    bool decode(vec4 encoded, inout vec4 scene, inout vec4 normals)
    {
        scene = encoded;
        normals = vec4(0.0, 0.0, 0.0, 1.0);

        if (encoded.a <= 0.0)
        {
            encoded.a *= -1.0;

            float rBit, gBit, bBit;

            float bits = floor(encoded.a + 0.5);

            bBit = floor(bits / EXTRA_PRECISSION2);
            bits -= EXTRA_PRECISSION2 * bBit;

            gBit = floor(bits / EXTRA_PRECISSION);
            bits -= EXTRA_PRECISSION * gBit;

            rBit = floor(bits);

            fromFloat(encoded.r, scene.r, normals.r, rBit);
            fromFloat(encoded.g, scene.g, normals.g, gBit);
            fromFloat(encoded.b, scene.b, normals.b, bBit);

            return true;
        }

        return false;
    }

#ifndef POSTPROCESS

    uniform bool isReflection;
    uniform bool isRefraction;

#include "lib/material/blending.glsl"

    float toFloat(float a, float b, inout float extraBit)
    {
        float f = floor(a * SCENE_PRECISSION);

        float start = 1.0 / (SCENE_PRECISSION / f);
        float end = 1.0 / (SCENE_PRECISSION / (f + 1.0));
        float diff = 1.0 - clamp((end - a) / (end - start), 0.0, 1.0);

        extraBit = floor(diff * EXTRA_PRECISSION);

#if USE_SIGN_BIT
        float extraBit2 = floor(diff * (EXTRA_PRECISSION * 2.0));
        if (extraBit2 / 2.0 != extraBit)
            return -1.0 * (f + b);
#endif

        return f + b;
    }

    vec4 encode(vec4 scene, vec4 normals)
    {
        if (isReflection || isRefraction)
            return scene;

#if defined(BLEND)
        blendCombined(scene, normals);
#endif

        float rBit, gBit, bBit;

        return vec4(
            toFloat(scene.r, normals.r, rBit),
            toFloat(scene.g, normals.g, gBit),
            toFloat(scene.b, normals.b, bBit),
            -1.0 * (rBit + (EXTRA_PRECISSION * gBit) + (EXTRA_PRECISSION2 * bBit)));
    }

#endif
	
#endif

#ifndef LIB_MATERIAL_ALPHA
#define LIB_MATERIAL_ALPHA

#define FUNC_NEVER                          512 // 0x0200
#define FUNC_LESS                           513 // 0x0201
#define FUNC_EQUAL                          514 // 0x0202
#define FUNC_LEQUAL                         515 // 0x0203
#define FUNC_GREATER                        516 // 0x0204
#define FUNC_NOTEQUAL                       517 // 0x0205
#define FUNC_GEQUAL                         518 // 0x0206
#define FUNC_ALWAYS                         519 // 0x0207

uniform vec2 texSize;

float mipmapLevel(vec2 scaleduv)
{
    vec2 dUVdx = dFdx(scaleduv);
    vec2 dUVdy = dFdy(scaleduv);
    float maxDUVSquared = max(dot(dUVdx, dUVdx), dot(dUVdy, dUVdy));
    return max(0.0, 0.5 * log2(maxDUVSquared));
}

float coveragePreservingAlphaScale(sampler2D diffuseMap, vec2 uv)
{
    #if @adjustCoverage
        vec2 textureSize;
        #if @useGPUShader4
            textureSize = textureSize2D(diffuseMap, 0);
        #else
            textureSize = texSize;
        #endif
            return 1.0 + mipmapLevel(uv * textureSize) * 0.25;
    #else
        return 1.0;
    #endif
}

float alphaTest(float alpha, float ref)
{
    #if @alphaToCoverage 
        float coverageAlpha = (alpha - clamp(ref, 0.0001, 0.9999)) / max(fwidth(alpha), 0.0001) + 0.5;

        // Some functions don't make sense with A2C or are a pain to think about and no meshes use them anyway
        // Use regular alpha testing in such cases until someone complains.
        #if @alphaFunc == FUNC_NEVER
            discard;
        #elif @alphaFunc == FUNC_LESS
            return 1.0 - coverageAlpha;
        #elif @alphaFunc == FUNC_EQUAL
            if (alpha != ref)
                discard;
        #elif @alphaFunc == FUNC_LEQUAL
            return 1.0 - coverageAlpha;
        #elif @alphaFunc == FUNC_GREATER
            return coverageAlpha;
        #elif @alphaFunc == FUNC_NOTEQUAL
            if (alpha == ref)
                discard;
        #elif @alphaFunc == FUNC_GEQUAL
            return coverageAlpha;
        #endif
    #else
        #if @alphaFunc == FUNC_NEVER
            discard;
        #elif @alphaFunc == FUNC_LESS
            if (alpha >= ref)
                discard;
        #elif @alphaFunc == FUNC_EQUAL
            if (alpha != ref)
                discard;
        #elif @alphaFunc == FUNC_LEQUAL
            if (alpha > ref)
                discard;
        #elif @alphaFunc == FUNC_GREATER
            if (alpha <= ref)
                discard;
        #elif @alphaFunc == FUNC_NOTEQUAL
            if (alpha == ref)
                discard;
        #elif @alphaFunc == FUNC_GEQUAL
            if (alpha < ref)
                discard;
        #endif
    #endif

    return alpha;
}

#endif

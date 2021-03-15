
#define FUNC_NEVER                          512 // 0x0200
#define FUNC_LESS                           513 // 0x0201
#define FUNC_EQUAL                          514 // 0x0202
#define FUNC_LEQUAL                         515 // 0x0203
#define FUNC_GREATER                        516 // 0x0204
#define FUNC_NOTEQUAL                       517 // 0x0205
#define FUNC_GEQUAL                         518 // 0x0206
#define FUNC_ALWAYS                         519 // 0x0207

#if @alphaFunc != FUNC_ALWAYS && @alphaFunc != FUNC_NEVER
uniform float alphaRef;
#endif

float mipmapLevel(vec2 scaleduv)
{
    vec2 dUVdx = dFdx(scaleduv);
    vec2 dUVdy = dFdy(scaleduv);
    float maxDUVSquared = max(dot(dUVdx, dUVdx), dot(dUVdy, dUVdy));
    return max(0.0, 0.5 * log2(maxDUVSquared));
}

float coveragePreservingAlphaScale(sampler2D diffuseMap, vec2 uv)
{
    #if @alphaFunc != FUNC_ALWAYS && @alphaFunc != FUNC_NEVER
        vec2 textureSize;
        #if @useGPUShader4
            textureSize = textureSize2D(diffuseMap, 0);
        #else
            textureSize = vec2(256.0);
        #endif
            return 1.0 + mipmapLevel(uv * textureSize) * 0.25;
    #else
        return 1.0;
    #endif
}

void alphaTest()
{
    #if @alphaToCoverage 
        float coverageAlpha = (gl_FragData[0].a - clamp(alphaRef, 0.0001, 0.9999)) / max(fwidth(gl_FragData[0].a), 0.0001) + 0.5;

        // Some functions don't make sense with A2C or are a pain to think about and no meshes use them anyway
        // Use regular alpha testing in such cases until someone complains.
        #if @alphaFunc == FUNC_NEVER
            discard;
        #elif @alphaFunc == FUNC_LESS
            gl_FragData[0].a = 1.0 - coverageAlpha;
        #elif @alphaFunc == FUNC_EQUAL
            if (gl_FragData[0].a != alphaRef)
                discard;
        #elif @alphaFunc == FUNC_LEQUAL
            gl_FragData[0].a = 1.0 - coverageAlpha;
        #elif @alphaFunc == FUNC_GREATER
            gl_FragData[0].a = coverageAlpha;
        #elif @alphaFunc == FUNC_NOTEQUAL
            if (gl_FragData[0].a == alphaRef)
                discard;
        #elif @alphaFunc == FUNC_GEQUAL
            gl_FragData[0].a = coverageAlpha;
        #endif
    #else
        #if @alphaFunc == FUNC_NEVER
            discard;
        #elif @alphaFunc == FUNC_LESS
            if (gl_FragData[0].a >= alphaRef)
                discard;
        #elif @alphaFunc == FUNC_EQUAL
            if (gl_FragData[0].a != alphaRef)
                discard;
        #elif @alphaFunc == FUNC_LEQUAL
            if (gl_FragData[0].a > alphaRef)
                discard;
        #elif @alphaFunc == FUNC_GREATER
            if (gl_FragData[0].a <= alphaRef)
                discard;
        #elif @alphaFunc == FUNC_NOTEQUAL
            if (gl_FragData[0].a == alphaRef)
                discard;
        #elif @alphaFunc == FUNC_GEQUAL
            if (gl_FragData[0].a < alphaRef)
                discard;
        #endif
    #endif
}
#ifndef LIB_MATERIAL_BLENDING
#define LIB_MATERIAL_BLENDING

#if defined(BLEND)

#define FUNC_ZERO                            0
#define FUNC_ONE                             1
#define FUNC_SRC_COLOR                     768 // 0x0300
#define FUNC_ONE_MINUS_SRC_COLOR           769 // 0x0301
#define FUNC_SRC_ALPHA                     770 // 0x0302
#define FUNC_ONE_MINUS_SRC_ALPHA           771 // 0x0303
#define FUNC_DST_ALPHA                     772 // 0x0304
#define FUNC_ONE_MINUS_DST_ALPHA           773 // 0x0305

#define FUNC_DST_COLOR                     774 // 0x0306
#define FUNC_ONE_MINUS_DST_COLOR           775 // 0x0307
#define FUNC_SRC_ALPHA_SATURATE            776 // 0x0308

vec4 blend(vec4 source, vec4 destination) 
{
    float srcFactor, dstFactor;
    float srcColor, dstColor, srcAlpha, dstAlpha;
    float src[4];
    float dst[4];

    src[0] = source.r;
    src[1] = source.g;
    src[2] = source.b;
    src[3] = source.a;
    srcAlpha = source.a;

    dst[0] = destination.r;
    dst[1] = destination.g;
    dst[2] = destination.b;
    dst[3] = destination.a;
    dstAlpha = destination.a;

    float result[4];
    for (int i = 0; i < 3; i++)
    {
        srcColor = src[i];
        dstColor = dst[i];

        #if @srcBlendFunc == FUNC_ZERO
            srcFactor = 0.0;
        #elif @srcBlendFunc == FUNC_ONE
            srcFactor = 1.0;
        #elif @srcBlendFunc == FUNC_SRC_COLOR
            srcFactor = srcColor;
        #elif @srcBlendFunc == FUNC_ONE_MINUS_SRC_COLOR
            srcFactor = 1.0 - srcColor;
        #elif @srcBlendFunc == FUNC_SRC_ALPHA
            srcFactor = srcAlpha;
        #elif @srcBlendFunc == FUNC_ONE_MINUS_SRC_ALPHA
            srcFactor = 1.0 - srcAlpha;
        #elif @srcBlendFunc == FUNC_DST_ALPHA
            srcFactor = dstAlpha;
        #elif @srcBlendFunc == FUNC_ONE_MINUS_DST_ALPHA
            srcFactor = 1.0 - dstAlpha;
        #elif @srcBlendFunc == FUNC_DST_COLOR
            srcFactor = dstColor;
        #elif @srcBlendFunc == FUNC_ONE_MINUS_DST_COLOR
            srcFactor = 1.0 - dstColor;
        #endif

        #if @dstBlendFunc == FUNC_ZERO
            dstFactor = 0.0;
        #elif @dstBlendFunc == FUNC_ONE
            dstFactor = 1.0;
        #elif @dstBlendFunc == FUNC_SRC_COLOR
            dstFactor = srcColor;
        #elif @dstBlendFunc == FUNC_ONE_MINUS_SRC_COLOR
            dstFactor = 1.0 - srcColor;
        #elif @dstBlendFunc == FUNC_SRC_ALPHA
            dstFactor = srcAlpha;
        #elif @dstBlendFunc == FUNC_ONE_MINUS_SRC_ALPHA
            dstFactor = 1.0 - srcAlpha;
        #elif @dstBlendFunc == FUNC_DST_ALPHA
            dstFactor = dstAlpha;
        #elif @dstBlendFunc == FUNC_ONE_MINUS_DST_ALPHA
            dstFactor = 1.0 - dstAlpha;
        #elif @dstBlendFunc == FUNC_DST_COLOR
            dstFactor = dstColor;
        #elif @dstBlendFunc == FUNC_ONE_MINUS_DST_COLOR
            dstFactor = 1.0 - dstColor;
        #endif

        result[i] = (srcColor * srcFactor) + (dstColor * dstFactor);
    }

    return vec4(result[0], result[1], result[2], result[3]);
}

void blendCombined(inout vec4 scene, inout vec4 normals)
{
    vec4 lastData, lastScene, lastNormals;
    lastData = gl_LastFragData[0];
    bool decodingDone = decode(lastData, lastScene, lastNormals);

    scene = blend(scene, lastScene);

#if defined(PARTICLE) && PARTICLE
        normals = lastNormals;
#else
        normals = blend(normals, lastNormals);
#endif
}

#endif

#endif

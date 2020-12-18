
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

void alphaTest()
{
    #if @alphaFunc == FUNC_NEVER
        discard;
    #elif @alphaFunc == FUNC_LESS
        if (gl_FragData[0].a > alphaRef)
            discard;
    #elif @alphaFunc == FUNC_EQUAL
        if (gl_FragData[0].a != alphaRef)
            discard;
    #elif @alphaFunc == FUNC_LEQUAL
        if (gl_FragData[0].a >= alphaRef)
            discard;
    #elif @alphaFunc == FUNC_GREATER
        if (gl_FragData[0].a < alphaRef)
            discard;
    #elif @alphaFunc == FUNC_NOTEQUAL
        if (gl_FragData[0].a == alphaRef)
            discard;
    #elif @alphaFunc == FUNC_GEQUAL
        if (gl_FragData[0].a <= alphaRef)
            discard;
    #endif
}
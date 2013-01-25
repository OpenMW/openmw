#define gammaCorrectRead(v) pow(max(v, 0.00001f), float3(gammaCorrection,gammaCorrection,gammaCorrection))
#define gammaCorrectOutput(v) pow(max(v, 0.00001f), float3(1.f/gammaCorrection,1.f/gammaCorrection,1.f/gammaCorrection))



#if SH_HLSL == 1 || SH_CG == 1

    #define shTexture2D sampler2D
    #define shSample(tex, coord) tex2D(tex, coord)
    #define shCubicSample(tex, coord) texCUBE(tex, coord)
    #define shLerp(a, b, t) lerp(a, b, t)
    #define shSaturate(a) saturate(a)

    #define shSampler2D(name) , uniform sampler2D name : register(s@shCounter(0)) @shUseSampler(name)
    
    #define shSamplerCube(name) , uniform samplerCUBE name : register(s@shCounter(0)) @shUseSampler(name)

    #define shMatrixMult(m, v) mul(m, v)

    #define shUniform(type, name) , uniform type name

    #define shTangentInput(type) , in type tangent : TANGENT
    #define shVertexInput(type, name) , in type name : TEXCOORD@shCounter(1)
    #define shInput(type, name) , in type name : TEXCOORD@shCounter(1)
    #define shOutput(type, name) , out type name : TEXCOORD@shCounter(2)

    #define shNormalInput(type) , in type normal : NORMAL
    
    #define shColourInput(type) , in type colour : COLOR

    #ifdef SH_VERTEX_SHADER

        #define shOutputPosition oPosition
        #define shInputPosition iPosition


        #define SH_BEGIN_PROGRAM \
            void main( \
                  float4 iPosition : POSITION \
                , out float4 oPosition : POSITION

        #define SH_START_PROGRAM \
            ) \

    #endif

    #ifdef SH_FRAGMENT_SHADER

        #define shOutputColour(num) oColor##num

        #define shDeclareMrtOutput(num) , out float4 oColor##num : COLOR##num

        #define SH_BEGIN_PROGRAM \
            void main( \
                out float4 oColor0 : COLOR

        #define SH_START_PROGRAM \
            ) \

    #endif

#endif

#if SH_GLSL == 1

    @version 120

    #define float2 vec2
    #define float3 vec3
    #define float4 vec4
    #define int2 ivec2
    #define int3 ivec3
    #define int4 ivec4
    #define shTexture2D sampler2D
    #define shSample(tex, coord) texture2D(tex, coord)
    #define shCubicSample(tex, coord) textureCube(tex, coord)
    #define shLerp(a, b, t) mix(a, b, t)
    #define shSaturate(a) clamp(a, 0.0, 1.0)

    #define shUniform(type, name) uniform type name;

    #define shSampler2D(name) uniform sampler2D name; @shUseSampler(name)

    #define shSamplerCube(name) uniform samplerCube name; @shUseSampler(name)

    #define shMatrixMult(m, v) (m * v)

    #define shOutputPosition gl_Position

    #define float4x4 mat4
    #define float3x3 mat3
    
    // GLSL 1.3
    #if 0
    
    // automatically recognized by ogre when the input name equals this
    #define shInputPosition vertex

    #define shOutputColour(num) oColor##num

    #define shTangentInput(type) in type tangent;
    #define shVertexInput(type, name) in type name;
    #define shInput(type, name) in type name;
    #define shOutput(type, name) out type name;

    // automatically recognized by ogre when the input name equals this
    #define shNormalInput(type) in type normal;
    #define shColourInput(type) in type colour;

    #ifdef SH_VERTEX_SHADER

        #define SH_BEGIN_PROGRAM \
            in float4 vertex;
        #define SH_START_PROGRAM \
            void main(void)

    #endif

    #ifdef SH_FRAGMENT_SHADER

        #define shDeclareMrtOutput(num) out vec4 oColor##num;

        #define SH_BEGIN_PROGRAM \
            out float4 oColor0;
        #define SH_START_PROGRAM \
            void main(void)


    #endif
    
    #endif
    
    // GLSL 1.2
    
    #if 1
    
    // automatically recognized by ogre when the input name equals this
    #define shInputPosition vertex

    #define shOutputColour(num) gl_FragData[num]

    #define shTangentInput(type) attribute type tangent;
    #define shVertexInput(type, name) attribute type name;
    #define shInput(type, name) varying type name;
    #define shOutput(type, name) varying type name;

    // automatically recognized by ogre when the input name equals this
    #define shNormalInput(type) attribute type normal;
    #define shColourInput(type) attribute type colour;

    #ifdef SH_VERTEX_SHADER

        #define SH_BEGIN_PROGRAM \
            attribute vec4 vertex; 
        #define SH_START_PROGRAM \
            void main(void)

    #endif

    #ifdef SH_FRAGMENT_SHADER

        #define shDeclareMrtOutput(num) 

        #define SH_BEGIN_PROGRAM
        
        #define SH_START_PROGRAM \
            void main(void)


    #endif
    
    #endif
#endif

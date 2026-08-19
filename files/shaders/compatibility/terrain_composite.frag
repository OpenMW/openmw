#version 120

varying vec2 uv;

uniform sampler2D diffuseMap;

uniform mat4 texMat0;
uniform mat4 texMat1;

#if @blendMap
uniform sampler2D blendMap;
#endif

void main()
{
    vec2 adjustedUV = (texMat0 * vec4(uv, 0.0, 1.0)).xy;

    vec4 diffuseTex = texture2D(diffuseMap, adjustedUV);
    gl_FragData[0] = vec4(diffuseTex.xyz, 1.0);

#if @blendMap
    vec2 blendMapUV = (texMat1 * vec4(uv, 0.0, 1.0)).xy;
    gl_FragData[0].a *= texture2D(blendMap, blendMapUV).a;
#endif
}

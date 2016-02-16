#version 120

#if @diffuseMap
uniform sampler2D diffuseMap;
varying vec2 diffuseMapUV;
#endif

varying float depth;

void main()
{
#if @diffuseMap
    gl_FragData[0] = texture2D(diffuseMap, diffuseMapUV);
#else
    gl_FragData[0] = vec4(1.0, 1.0, 1.0, 1.0);
#endif

    float fogValue = clamp((depth - gl_Fog.start) * gl_Fog.scale, 0.0, 1.0);
    gl_FragData[0].xyz = mix(gl_FragData[0].xyz, gl_Fog.color.xyz, fogValue);
}

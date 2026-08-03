#version 120

uniform sampler2D diffuseMap;
varying vec2 diffuseMapUV;

#if @alphaMap
uniform sampler2D alphaMap;
varying vec2 alphaMapUV;
#endif

void main()
{
    vec4 diffuseTex = texture2D(diffuseMap, diffuseMapUV);
    float alpha = 1.0;
    
#if @alphaMap
    alpha = texture2D(alphaMap, alphaMapUV).a;
#endif

    gl_FragColor = vec4(diffuseTex.rgb, diffuseTex.a * alpha);
}
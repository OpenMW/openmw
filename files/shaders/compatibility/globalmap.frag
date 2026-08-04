#version 120

uniform sampler2D diffuseMap;
uniform sampler2D alphaMap;

varying vec2 alphaMapUV;
varying vec2 diffuseMapUV;

void main()
{
    vec4 diffuseTex = texture2D(diffuseMap, diffuseMapUV);
    float alpha = texture2D(alphaMap, alphaMapUV).a;
    gl_FragColor = vec4(diffuseTex.rgb, diffuseTex.a * alpha);
}

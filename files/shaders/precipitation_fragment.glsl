#version 120

uniform sampler2D diffuseMap;
varying vec2 diffuseMapUV;

#include "vertexcolors.glsl"

void main()
{
    gl_FragData[0].rgb = vec3(1.0);
    gl_FragData[0].a = texture2D(diffuseMap, diffuseMapUV).a * getDiffuseColor().a;

    if (gl_FragData[0].a <= 0.5)
        discard;
}
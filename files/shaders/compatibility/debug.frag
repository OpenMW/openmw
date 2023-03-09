#version 120

#include "vertexcolors.glsl"

varying vec3 vertexNormal;

uniform bool useAdvancedShader;

void main()
{
    vec3 lightDir = normalize(vec3(-1., -0.5, -2.));

    float lightAttenuation = dot(-lightDir, vertexNormal) * 0.5 + 0.5;

    if(!useAdvancedShader)
    {
        gl_FragData[0] = getDiffuseColor();
    }
    else
    {
        gl_FragData[0] = vec4(passColor.xyz * lightAttenuation, 1.);
    }
}

#version 120

#include "lib/core/fragment.h.glsl"

#include "lib/material/vertexcolors.glsl"

centroid varying vec4 passColor;

varying vec3 vertexNormal;

uniform bool useAdvancedShader = false;

void main()
{
    Material material = getMaterial();

    vec3 lightDir = normalize(vec3(-1., -0.5, -2.));

    float lightAttenuation = dot(-lightDir, vertexNormal) * 0.5 + 0.5;

    if(!useAdvancedShader)
    {
        gl_FragData[0] = getDiffuseColor(material, passColor);
    }
    else
    {
        gl_FragData[0] = vec4(passColor.xyz * lightAttenuation, 1.);
    }
}

#version 120

#include "lib/core/vertex.h.glsl"

uniform vec3 color;
uniform vec3 trans;
uniform vec3 scale;
uniform bool useNormalAsColor;
uniform bool useAdvancedShader;

centroid varying vec4 passColor;
varying vec3 vertexNormal;

void main()
{
    if(!useAdvancedShader)
    {
        gl_Position = modelToClip( vec4(gl_Vertex));
        vertexNormal = vec3(1., 1., 1.);
        passColor  = gl_Color;
    }
    else
    {
        gl_Position = modelToClip( vec4(gl_Vertex.xyz * scale + trans,1));

        vertexNormal = useNormalAsColor ? vec3(1., 1., 1.) : gl_Normal.xyz;
        vec3 colorOut = useNormalAsColor? gl_Normal.xyz : color;
        passColor = vec4(colorOut, 1.);
    }
}

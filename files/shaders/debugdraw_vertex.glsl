#version 120
#include "openmw_vertex.h.glsl"

uniform vec3 color;
uniform vec3 trans;
uniform vec3 scale;
uniform int  useNormalAsColor;
uniform int  useAdvancedShader = 0;

varying vec3 vertexColor;
varying vec3 vertexNormal;

void main()
{
    gl_Position = mw_modelToClip( vec4(gl_Vertex.xyz * scale + trans,1));

    if(useAdvancedShader == 0)
    {
        vertexNormal = vec3(1., 1., 1.);
        vertexColor  = gl_Color.xyz;
    }
    else
    {
        vertexNormal = useNormalAsColor == 1 ? vec3(1., 1., 1.) : gl_Normal.xyz;
        vertexColor = useNormalAsColor == 1 ? gl_Normal.xyz : color.xyz;
    }
}

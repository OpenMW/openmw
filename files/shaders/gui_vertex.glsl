#version @GLSLVersion

varying vec2 diffuseMapUV;
varying vec4 passColor;

void main()
{
    gl_Position = vec4(gl_Vertex.xyz, 1.0);
    diffuseMapUV = (gl_TextureMatrix[0] * gl_MultiTexCoord0).xy;
    passColor = gl_Color;
}

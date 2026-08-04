#version 120

varying vec2 diffuseMapUV;
varying vec2 alphaMapUV;

void main()
{
    gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;

    diffuseMapUV = gl_MultiTexCoord0.xy;
    alphaMapUV = gl_MultiTexCoord1.xy;
}

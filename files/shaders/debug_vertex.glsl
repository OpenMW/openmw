#version 120

uniform mat4 projectionMatrix;

centroid varying vec4 passColor;

void main()
{
    gl_Position = projectionMatrix * (gl_ModelViewMatrix * gl_Vertex);

    passColor = gl_Color;
}

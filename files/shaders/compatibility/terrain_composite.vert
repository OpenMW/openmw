#version 120

varying vec2 uv;

void main(void)
{
    gl_Position = gl_ModelViewMatrix * gl_Vertex;

    uv = gl_MultiTexCoord0.xy;
}

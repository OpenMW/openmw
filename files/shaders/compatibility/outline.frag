#version 120

uniform vec4 color;

void main()
{
    gl_FragData[0] = color;
}
#version 120

varying vec4 passColor;

void main()
{
    gl_FragData[0] = passColor;
}

#version 430 core

layout (location = 0) in vec4 Position;
layout (location = 3) in vec4 Color;
layout (location = 8) in vec3 TexCoord0;

out vec4 passColor;
out vec2 uv;

void main()
{
    gl_Position = vec4(Position.xyz, 1.0);
    uv = TexCoord0.xy;
    passColor = Color;
}

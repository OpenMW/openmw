#version 430 core

layout(location = 0) out vec4 Color;

layout(location = 0) uniform sampler2D diffuseMap;

in vec4 passColor;
in vec2 uv;

void main()
{
    Color = texture(diffuseMap, uv) * passColor;
}

#version 330 compatibility

uniform mat4 projectionMatrix;

uniform vec3 color;
uniform vec3 trans;
uniform vec3 scale;
uniform int useNormalAsColor;

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;

out vec3 vertexColor;
out vec3 vertexNormal;

void main()
{
    gl_Position = projectionMatrix * gl_ModelViewMatrix * vec4(aPos * scale + trans, 1.);

    vertexNormal = useNormalAsColor == 1 ? vec3(1., 1., 1.) : aNormal;
    vertexColor = useNormalAsColor == 1 ? aNormal : color.xyz;
}

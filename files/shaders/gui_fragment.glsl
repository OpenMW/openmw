#version @GLSLVersion

uniform sampler2D diffuseMap;

varying vec2 diffuseMapUV;
varying vec4 passColor;

void main()
{
    gl_FragData[0] = texture2D(diffuseMap, diffuseMapUV) * passColor;
}

#version 120

varying vec2 uv;
uniform samplerCube cubeMap;
uniform int mapping;

#include "lib/util/coordinates.glsl"

void main(void)
{
    vec3 c;

    if (mapping == 0)
        c = sphericalCoords(uv);
    else if (mapping == 1)
        c = cylindricalCoords(uv);
    else
        c = planetCoords(uv);

    gl_FragData[0] = textureCube(cubeMap,c);
}

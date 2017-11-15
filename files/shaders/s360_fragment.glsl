#version 120

varying vec2 uv;
uniform samplerCube cubeMap;

#define PI 3.1415926535

vec3 cylindricalCoords(vec2 coords)
{
    return normalize(vec3(cos(-1 * coords.x * 2 * PI),sin(-1 * coords.x * 2 * PI),coords.y * 2.0 - 1.0));
}

void main(void)
{
    vec3 c;
    c.x = uv.x * 2.0 - 1.0;
    c.y = uv.y * 2.0 - 1.0;
    c.z = 1.0;

    gl_FragData[0] = textureCube(cubeMap,vec3(cylindricalCoords(uv)));
}

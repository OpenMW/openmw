#version 120

varying vec2 uv;
uniform samplerCube cubeMap;

void main(void)
{
    gl_FragData[0] = textureCube(cubeMap,vec3(uv.x * 2.0 - 1.0,uv.y * 2.0 - 1.0,1));
}

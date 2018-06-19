#version 120

varying vec2 uv;
uniform samplerCube cubeMap;
uniform int mapping;

#define PI 3.1415926535

vec3 sphericalCoords(vec2 coords)
{
    coords.x = -1 * coords.x * 2 * PI;
    coords.y = (coords.y - 0.5) * PI;
 
    vec3 result = vec3(0.0,cos(coords.y),sin(coords.y));
    result = vec3(cos(coords.x) * result.y,sin(coords.x) * result.y,result.z);
 
    return result;
}

vec3 cylindricalCoords(vec2 coords)
{
    return normalize(vec3(cos(-1 * coords.x * 2 * PI),sin(-1 * coords.x * 2 * PI),coords.y * 2.0 - 1.0));
}

vec3 planetCoords(vec2 coords)
{
    vec2 fromCenter = coords - vec2(0.5,0.5);

    float magnitude = length(fromCenter);

    fromCenter = normalize(fromCenter);

    float dotProduct = dot(fromCenter,vec2(0.0,1.0));

    coords.x = coords.x > 0.5 ? 0.5 - (dotProduct + 1.0) / 4.0 : 0.5 + (dotProduct + 1.0) / 4.0;
    coords.y = max(0.0,1.0 - pow(magnitude / 0.5,0.5));
    return sphericalCoords(coords);
}

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

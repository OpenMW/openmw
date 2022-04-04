#version @GLSLVersion

#include "skypasses.glsl"

uniform int pass;
uniform sampler2D diffuseMap;
uniform sampler2D maskMap;      // PASS_MOON
uniform float opacity;          // PASS_CLOUDS, PASS_ATMOSPHERE_NIGHT
uniform vec4 moonBlend;         // PASS_MOON
uniform vec4 atmosphereFade;    // PASS_MOON

varying vec2 diffuseMapUV;
varying vec4 passColor;

void paintAtmosphere(inout vec4 color)
{
    color = gl_FrontMaterial.emission;
    color.a *= passColor.a;
}

void paintAtmosphereNight(inout vec4 color)
{
    color = texture2D(diffuseMap, diffuseMapUV);
    color.a *= passColor.a * opacity;
}

void paintClouds(inout vec4 color)
{
    color = texture2D(diffuseMap, diffuseMapUV);
    color.a *= passColor.a * opacity;
    color.xyz = clamp(color.xyz * gl_FrontMaterial.emission.xyz, 0.0, 1.0);

    // ease transition between clear color and atmosphere/clouds
    color = mix(vec4(gl_Fog.color.xyz, color.a), color, passColor.a);
}

void paintMoon(inout vec4 color)
{
    vec4 phase = texture2D(diffuseMap, diffuseMapUV);
    vec4 mask = texture2D(maskMap, diffuseMapUV);

    vec4 blendedLayer = phase * moonBlend;
    color = vec4(blendedLayer.xyz + atmosphereFade.xyz, atmosphereFade.a * mask.a);
}

void paintSun(inout vec4 color)
{
    color = texture2D(diffuseMap, diffuseMapUV);
    color.a *= gl_FrontMaterial.diffuse.a;
}

void paintSunflashQuery(inout vec4 color)
{
    const float threshold = 0.8;

    color = texture2D(diffuseMap, diffuseMapUV);
    if (color.a <= threshold)
        discard;
}

void paintSunglare(inout vec4 color)
{
    color = gl_FrontMaterial.emission;
    color.a = gl_FrontMaterial.diffuse.a;
}

void main()
{
    vec4 color = vec4(0.0);

    if (pass == PASS_ATMOSPHERE)
        paintAtmosphere(color);
    else if (pass == PASS_ATMOSPHERE_NIGHT)
        paintAtmosphereNight(color);
    else if (pass == PASS_CLOUDS)
        paintClouds(color);
    else if (pass == PASS_MOON)
        paintMoon(color);
    else if (pass == PASS_SUN)
        paintSun(color);
    else if (pass == PASS_SUNFLASH_QUERY)
        paintSunflashQuery(color);
    else if (pass == PASS_SUNGLARE)
        paintSunglare(color);

    gl_FragData[0] = color;
}

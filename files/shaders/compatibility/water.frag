#version 120

#if @useUBO
    #extension GL_ARB_uniform_buffer_object : require
#endif

#if @useGPUShader4
    #extension GL_EXT_gpu_shader4: require
#endif

#include "lib/core/fragment.h.glsl"

#define REFRACTION @refraction_enabled

// Inspired by Blender GLSL Water by martinsh ( https://devlog-martinsh.blogspot.de/2012/07/waterundewater-shader-wip.html )

// tweakables -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --

const float VISIBILITY = 2500.0;

const float BIG_WAVES_X = 0.1; // strength of big waves
const float BIG_WAVES_Y = 0.1;

const float MID_WAVES_X = 0.1; // strength of middle sized waves
const float MID_WAVES_Y = 0.1;
const float MID_WAVES_RAIN_X = 0.2;
const float MID_WAVES_RAIN_Y = 0.2;

const float SMALL_WAVES_X = 0.1; // strength of small waves
const float SMALL_WAVES_Y = 0.1;
const float SMALL_WAVES_RAIN_X = 0.3;
const float SMALL_WAVES_RAIN_Y = 0.3;

const float WAVE_CHOPPYNESS = 0.05;                // wave choppyness
const float WAVE_SCALE = 75.0;                     // overall wave scale

const float BUMP = 0.5;                            // overall water surface bumpiness
const float BUMP_RAIN = 2.5;
const float REFL_BUMP = 0.10;                      // reflection distortion amount
const float REFR_BUMP = 0.07;                      // refraction distortion amount

const float SCATTER_AMOUNT = 0.3;                  // amount of sunlight scattering
const vec3 SCATTER_COLOUR = vec3(0.0,1.0,0.95);    // colour of sunlight scattering

const vec3 SUN_EXT = vec3(0.45, 0.55, 0.68);       //sunlight extinction

const float SPEC_HARDNESS = 256.0;                 // specular highlights hardness

const float BUMP_SUPPRESS_DEPTH = 300.0;           // at what water depth bumpmap will be suppressed for reflections and refractions (prevents artifacts at shores)

const vec2 WIND_DIR = vec2(0.5f, -0.8f);
const float WIND_SPEED = 0.2f;

const vec3 WATER_COLOR = vec3(0.090195, 0.115685, 0.12745);

const float WOBBLY_SHORE_FADE_DISTANCE = 6200.0;   // fade out wobbly shores to mask precision errors, the effect is almost impossible to see at a distance

// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -

vec2 normalCoords(vec2 uv, float scale, float speed, float time, float timer1, float timer2, vec3 previousNormal)
{
  return uv * (WAVE_SCALE * scale) + WIND_DIR * time * (WIND_SPEED * speed) -(previousNormal.xy/previousNormal.zz) * WAVE_CHOPPYNESS + vec2(time * timer1,time * timer2);
}

uniform sampler2D rippleMap;
uniform vec3 playerPos;

varying vec3 worldPos;

varying vec2 rippleMapUV;

varying vec4 position;
varying float linearDepth;

uniform sampler2D normalMap;

uniform float osg_SimulationTime;

uniform float near;
uniform float far;

uniform float rainIntensity;

uniform vec2 screenRes;

#define PER_PIXEL_LIGHTING 0

#include "shadows_fragment.glsl"
#include "lib/light/lighting.glsl"
#include "fog.glsl"
#include "lib/water/fresnel.glsl"
#include "lib/water/rain_ripples.glsl"
#include "lib/view/depth.glsl"

void main(void)
{
    vec2 UV = worldPos.xy / (8192.0*5.0) * 3.0;
    UV.y *= -1.0;

    float shadow = unshadowedLightRatio(linearDepth);

    vec2 screenCoords = gl_FragCoord.xy / screenRes;

    #define waterTimer osg_SimulationTime

    vec3 normal0 = 2.0 * texture2D(normalMap,normalCoords(UV, 0.05, 0.04, waterTimer, -0.015, -0.005, vec3(0.0,0.0,0.0))).rgb - 1.0;
    vec3 normal1 = 2.0 * texture2D(normalMap,normalCoords(UV, 0.1,  0.08, waterTimer,  0.02,   0.015, normal0)).rgb - 1.0;
    vec3 normal2 = 2.0 * texture2D(normalMap,normalCoords(UV, 0.25, 0.07, waterTimer, -0.04,  -0.03,  normal1)).rgb - 1.0;
    vec3 normal3 = 2.0 * texture2D(normalMap,normalCoords(UV, 0.5,  0.09, waterTimer,  0.03,   0.04,  normal2)).rgb - 1.0;
    vec3 normal4 = 2.0 * texture2D(normalMap,normalCoords(UV, 1.0,  0.4,  waterTimer, -0.02,   0.1,   normal3)).rgb - 1.0;
    vec3 normal5 = 2.0 * texture2D(normalMap,normalCoords(UV, 2.0,  0.7,  waterTimer,  0.1,   -0.06,  normal4)).rgb - 1.0;

    vec4 rainRipple;

    if (rainIntensity > 0.01)
      rainRipple = rainCombined(position.xy/1000.0, waterTimer) * clamp(rainIntensity, 0.0, 1.0);
    else
      rainRipple = vec4(0.0);

    vec3 rippleAdd = rainRipple.xyz * 10.0;

    float distToCenter = length(rippleMapUV - vec2(0.5));
    float blendClose = smoothstep(0.001, 0.02, distToCenter);
    float blendFar = 1.0 - smoothstep(0.3, 0.4, distToCenter);
    float distortionLevel = 2.0;
    rippleAdd += distortionLevel * vec3(texture2D(rippleMap, rippleMapUV).ba * blendFar * blendClose, 0.0);

    vec2 bigWaves = vec2(BIG_WAVES_X,BIG_WAVES_Y);
    vec2 midWaves = mix(vec2(MID_WAVES_X,MID_WAVES_Y),vec2(MID_WAVES_RAIN_X,MID_WAVES_RAIN_Y),rainIntensity);
    vec2 smallWaves = mix(vec2(SMALL_WAVES_X,SMALL_WAVES_Y),vec2(SMALL_WAVES_RAIN_X,SMALL_WAVES_RAIN_Y),rainIntensity);
    float bump = mix(BUMP,BUMP_RAIN,rainIntensity);

    vec3 normal = (normal0 * bigWaves.x + normal1 * bigWaves.y + normal2 * midWaves.x +
                   normal3 * midWaves.y + normal4 * smallWaves.x + normal5 * smallWaves.y + rippleAdd);
    normal = normalize(vec3(-normal.x * bump, -normal.y * bump, normal.z));

    vec3 lVec = normalize((gl_ModelViewMatrixInverse * vec4(lcalcPosition(0).xyz, 0.0)).xyz);
    vec3 cameraPos = (gl_ModelViewMatrixInverse * vec4(0,0,0,1)).xyz;
    vec3 vVec = normalize(position.xyz - cameraPos.xyz);

    float sunFade = length(gl_LightModel.ambient.xyz);

    // fresnel
    float ior = (cameraPos.z>0.0)?(1.333/1.0):(1.0/1.333); // air to water; water to air
    float fresnel = clamp(fresnel_dielectric(vVec, normal, ior), 0.0, 1.0);

    float radialise = 1.0;

#if @radialFog
    float radialDepth = distance(position.xyz, cameraPos);
    // TODO: Figure out how to properly radialise refraction depth and thus underwater fog
    // while avoiding oddities when the water plane is close to the clipping plane
    // radialise = radialDepth / linearDepth;
#else
    float radialDepth = 0.0;
#endif

    vec2 screenCoordsOffset = normal.xy * REFL_BUMP;
#if REFRACTION
    float depthSample = linearizeDepth(sampleRefractionDepthMap(screenCoords), near, far) * radialise;
    float depthSampleDistorted = linearizeDepth(sampleRefractionDepthMap(screenCoords-screenCoordsOffset), near, far) * radialise;
    float surfaceDepth = linearizeDepth(gl_FragCoord.z, near, far) * radialise;
    float realWaterDepth = depthSample - surfaceDepth;  // undistorted water depth in view direction, independent of frustum
    screenCoordsOffset *= clamp(realWaterDepth / BUMP_SUPPRESS_DEPTH,0,1);
#endif
    // reflection
    vec3 reflection = sampleReflectionMap(screenCoords + screenCoordsOffset).rgb;

    // specular
    float specular = pow(max(dot(reflect(vVec, normal), lVec), 0.0),SPEC_HARDNESS) * shadow;

    vec3 waterColor = WATER_COLOR * sunFade;

    vec4 sunSpec = lcalcSpecular(0);

    // artificial specularity to make rain ripples more noticeable
    vec3 skyColorEstimate = vec3(max(0.0, mix(-0.3, 1.0, sunFade)));
    vec3 rainSpecular = abs(rainRipple.w)*mix(skyColorEstimate, vec3(1.0), 0.05)*0.5;

#if REFRACTION
    // no alpha here, so make sure raindrop ripple specularity gets properly subdued
    rainSpecular *= clamp(fresnel*6.0 + specular * sunSpec.w, 0.0, 1.0);

    // refraction
    vec3 refraction = sampleRefractionMap(screenCoords - screenCoordsOffset).rgb;
    vec3 rawRefraction = refraction;

    // brighten up the refraction underwater
    if (cameraPos.z < 0.0)
        refraction = clamp(refraction * 1.5, 0.0, 1.0);
    else
        refraction = mix(refraction, waterColor, clamp(depthSampleDistorted/VISIBILITY, 0.0, 1.0));

    // sunlight scattering
    // normal for sunlight scattering
    vec3 lNormal = (normal0 * bigWaves.x * 0.5 + normal1 * bigWaves.y * 0.5 + normal2 * midWaves.x * 0.2 +
                    normal3 * midWaves.y * 0.2 + normal4 * smallWaves.x * 0.1 + normal5 * smallWaves.y * 0.1 + rippleAdd);
    lNormal = normalize(vec3(-lNormal.x * bump, -lNormal.y * bump, lNormal.z));
    float sunHeight = lVec.z;
    vec3 scatterColour = mix(SCATTER_COLOUR*vec3(1.0,0.4,0.0), SCATTER_COLOUR, clamp(1.0-exp(-sunHeight*SUN_EXT), 0.0, 1.0));
    vec3 lR = reflect(lVec, lNormal);
    float lightScatter = clamp(dot(lVec,lNormal)*0.7+0.3, 0.0, 1.0) * clamp(dot(lR, vVec)*2.0-1.2, 0.0, 1.0) * SCATTER_AMOUNT * sunFade * clamp(1.0-exp(-sunHeight), 0.0, 1.0);
    gl_FragData[0].xyz = mix( mix(refraction,  scatterColour,  lightScatter),  reflection,  fresnel) + specular * sunSpec.xyz + rainSpecular;
    gl_FragData[0].w = 1.0;

    // wobbly water: hard-fade into refraction texture at extremely low depth, with a wobble based on normal mapping
    vec3 normalShoreRippleRain = texture2D(normalMap,normalCoords(UV, 2.0, 2.7, -1.0*waterTimer,  0.05,  0.1,  normal3)).rgb - 0.5
                               + texture2D(normalMap,normalCoords(UV, 2.0, 2.7,      waterTimer,  0.04, -0.13, normal4)).rgb - 0.5;
    float verticalWaterDepth = realWaterDepth * mix(abs(vVec.z), 1.0, 0.2); // an estimate
    float shoreOffset = verticalWaterDepth - (normal2.r + mix(0.0, normalShoreRippleRain.r, rainIntensity) + 0.15)*8.0;
    float fuzzFactor = min(1.0, 1000.0/surfaceDepth) * mix(abs(vVec.z), 1.0, 0.2);
    shoreOffset *= fuzzFactor;
    shoreOffset = clamp(mix(shoreOffset, 1.0, clamp(linearDepth / WOBBLY_SHORE_FADE_DISTANCE, 0.0, 1.0)), 0.0, 1.0);
    gl_FragData[0].xyz = mix(rawRefraction, gl_FragData[0].xyz, shoreOffset);
#else
    gl_FragData[0].xyz = mix(reflection,  waterColor,  (1.0-fresnel)*0.5) + specular * sunSpec.xyz + rainSpecular;
    gl_FragData[0].w = clamp(fresnel*6.0 + specular * sunSpec.w, 0.0, 1.0);     //clamp(fresnel*2.0 + specular * gl_LightSource[0].specular.w, 0.0, 1.0);
#endif

    gl_FragData[0] = applyFogAtDist(gl_FragData[0], radialDepth, linearDepth, far);

#if !@disableNormals
    gl_FragData[1].rgb = normalize(gl_NormalMatrix * normal) * 0.5 + 0.5;
#endif

    applyShadowDebugOverlay();
}

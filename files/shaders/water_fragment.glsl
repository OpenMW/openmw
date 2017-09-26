#version 120

#define REFRACTION @refraction_enabled

// Inspired by Blender GLSL Water by martinsh ( http://devlog-martinsh.blogspot.de/2012/07/waterundewater-shader-wip.html )

// tweakables -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --

const float VISIBILITY = 2.5;

const float BIG_WAVES_X = 0.1; // strength of big waves
const float BIG_WAVES_Y = 0.1;

const float MID_WAVES_X = 0.1; // strength of middle sized waves
const float MID_WAVES_Y = 0.1;

const float SMALL_WAVES_X = 0.1; // strength of small waves
const float SMALL_WAVES_Y = 0.1;

const float WAVE_CHOPPYNESS = 0.05;                // wave choppyness
const float WAVE_SCALE = 75.0;                     // overall wave scale

const float BUMP = 0.5;                            // overall water surface bumpiness
const float REFL_BUMP = 0.10;                      // reflection distortion amount
const float REFR_BUMP = 0.07;                      // refraction distortion amount

const float SCATTER_AMOUNT = 0.3;                  // amount of sunlight scattering
const vec3 SCATTER_COLOUR = vec3(0.0,1.0,0.95);    // colour of sunlight scattering

const vec3 SUN_EXT = vec3(0.45, 0.55, 0.68);       //sunlight extinction

const float SPEC_HARDNESS = 256.0;                 // specular highlights hardness

const float BUMP_SUPPRESS_DEPTH = 0.3;             // at what water depth bumpmap will be supressed for reflections and refractions (prevents artifacts at shores)

const vec2 WIND_DIR = vec2(0.5f, -0.8f);
const float WIND_SPEED = 0.2f;

const vec3 WATER_COLOR = vec3(0.090195, 0.115685, 0.12745);

// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -

float fresnel_dielectric(vec3 Incoming, vec3 Normal, float eta)
{
    float c = abs(dot(Incoming, Normal));
    float g = eta * eta - 1.0 + c * c;
    float result;

    if(g > 0.0) {
        g = sqrt(g);
        float A =(g - c)/(g + c);
        float B =(c *(g + c)- 1.0)/(c *(g - c)+ 1.0);
        result = 0.5 * A * A *(1.0 + B * B);
    }
    else
        result = 1.0;  /* TIR (no refracted component) */

    return result;
}

varying vec3 screenCoordsPassthrough;
varying vec4 position;
varying float depthPassthrough;

uniform sampler2D normalMap;

uniform sampler2D reflectionMap;
#if REFRACTION
uniform sampler2D refractionMap;
uniform sampler2D refractionDepthMap;
#endif
                
uniform float osg_SimulationTime;

uniform float near;
uniform float far;
uniform vec3 nodePosition;

float frustumDepth;

float linearizeDepth(float depth)  // takes <0,1> non-linear depth value and returns <0,1> linearized value
  {
    float z_n = 2.0 * depth - 1.0;
    depth = 2.0 * near * far / (far + near - z_n * frustumDepth);
    return depth / frustumDepth;
  }

void main(void)
{
    frustumDepth = abs(far - near);
    vec3 worldPos = position.xyz + nodePosition.xyz;
    vec2 UV = worldPos.xy / (8192.0*5.0) * 3.0;
    UV.y *= -1.0;

    float shadow = 1.0;

    vec2 screenCoords = screenCoordsPassthrough.xy / screenCoordsPassthrough.z;
    screenCoords.y = (1.0-screenCoords.y);

    vec2 nCoord = vec2(0.0,0.0);

    #define waterTimer osg_SimulationTime

    nCoord = UV * (WAVE_SCALE * 0.05) + WIND_DIR * waterTimer * (WIND_SPEED*0.04);
    vec3 normal0 = 2.0 * texture2D(normalMap,  nCoord + vec2(-waterTimer*0.015,-waterTimer*0.005)).rgb - 1.0;
    nCoord = UV * (WAVE_SCALE * 0.1) + WIND_DIR * waterTimer * (WIND_SPEED*0.08)-(normal0.xy/normal0.zz)*WAVE_CHOPPYNESS;
    vec3 normal1 = 2.0 * texture2D(normalMap,  nCoord + vec2(+waterTimer*0.020,+waterTimer*0.015)).rgb - 1.0;

    nCoord = UV * (WAVE_SCALE * 0.25) + WIND_DIR * waterTimer * (WIND_SPEED*0.07)-(normal1.xy/normal1.zz)*WAVE_CHOPPYNESS;
    vec3 normal2 = 2.0 * texture2D(normalMap,  nCoord + vec2(-waterTimer*0.04,-waterTimer*0.03)).rgb - 1.0;
    nCoord = UV * (WAVE_SCALE * 0.5) + WIND_DIR * waterTimer * (WIND_SPEED*0.09)-(normal2.xy/normal2.z)*WAVE_CHOPPYNESS;
    vec3 normal3 = 2.0 * texture2D(normalMap,  nCoord + vec2(+waterTimer*0.03,+waterTimer*0.04)).rgb - 1.0;

    nCoord = UV * (WAVE_SCALE* 1.0) + WIND_DIR * waterTimer * (WIND_SPEED*0.4)-(normal3.xy/normal3.zz)*WAVE_CHOPPYNESS;
    vec3 normal4 = 2.0 * texture2D(normalMap,  nCoord + vec2(-waterTimer*0.02,+waterTimer*0.1)).rgb - 1.0;
    nCoord = UV * (WAVE_SCALE * 2.0) + WIND_DIR * waterTimer * (WIND_SPEED*0.7)-(normal4.xy/normal4.zz)*WAVE_CHOPPYNESS;
    vec3 normal5 = 2.0 * texture2D(normalMap,  nCoord + vec2(+waterTimer*0.1,-waterTimer*0.06)).rgb - 1.0;



    vec3 normal = (normal0 * BIG_WAVES_X + normal1 * BIG_WAVES_Y +
			normal2 * MID_WAVES_X + normal3 * MID_WAVES_Y +
			normal4 * SMALL_WAVES_X + normal5 * SMALL_WAVES_Y);

    normal = normalize(vec3(normal.x * BUMP, normal.y * BUMP, normal.z));

    normal = vec3(-normal.x, -normal.y, normal.z);

    // normal for sunlight scattering
    vec3 lNormal = (normal0 * BIG_WAVES_X*0.5 + normal1 * BIG_WAVES_Y*0.5 +
		normal2 * MID_WAVES_X*0.2 + normal3 * MID_WAVES_Y*0.2 +
		normal4 * SMALL_WAVES_X*0.1 + normal5 * SMALL_WAVES_Y*0.1).xyz;
    lNormal = normalize(vec3(lNormal.x * BUMP, lNormal.y * BUMP, lNormal.z));
    lNormal = vec3(-lNormal.x, -lNormal.y, lNormal.z);


    vec3 lVec = normalize((gl_ModelViewMatrixInverse * vec4(gl_LightSource[0].position.xyz, 0.0)).xyz);

    vec3 cameraPos = (gl_ModelViewMatrixInverse * vec4(0,0,0,1)).xyz;
    vec3 vVec = normalize(position.xyz - cameraPos.xyz);

    float isUnderwater = (cameraPos.z > 0.0) ? 0.0 : 1.0;

    // sunlight scattering
    vec3 pNormal = vec3(0,0,1);
    vec3 lR = reflect(lVec, lNormal);
    vec3 llR = reflect(lVec, pNormal);

    float sunHeight = lVec.z;
    float sunFade = length(gl_LightModel.ambient.xyz);

    float s = clamp(dot(lR, vVec)*2.0-1.2, 0.0, 1.0);
    float lightScatter = shadow * clamp(dot(lVec,lNormal)*0.7+0.3, 0.0, 1.0) * s * SCATTER_AMOUNT * sunFade * clamp(1.0-exp(-sunHeight), 0.0, 1.0);
    vec3 scatterColour = mix(vec3(SCATTER_COLOUR)*vec3(1.0,0.4,0.0),  SCATTER_COLOUR,  clamp(1.0-exp(-sunHeight*SUN_EXT), 0.0, 1.0));

    // fresnel
    float ior = (cameraPos.z>0.0)?(1.333/1.0):(1.0/1.333); //air to water; water to air
    float fresnel = fresnel_dielectric(vVec, normal, ior);

    fresnel = clamp(fresnel, 0.0, 1.0);

#if REFRACTION
    float normalization = frustumDepth / 1000;
    float depthSample = linearizeDepth(texture2D(refractionDepthMap,screenCoords).x) * normalization;
    float depthSampleDistorted = linearizeDepth(texture2D(refractionDepthMap,screenCoords-(normal.xy*REFR_BUMP)).x) * normalization;
    float surfaceDepth = linearizeDepth(gl_FragCoord.z) * normalization;
    float realWaterDepth = depthSample - surfaceDepth;  // undistorted water depth in view direction, independent of frustum
 
    float shore = clamp(realWaterDepth / BUMP_SUPPRESS_DEPTH,0,1);
#else
    float shore = 1.0;
#endif
    // reflection
    vec3 reflection = texture2D(reflectionMap, screenCoords+(normal.xy*REFL_BUMP*shore)).rgb;

    // refraction
#if REFRACTION
    vec3 refraction = texture2D(refractionMap, screenCoords-(normal.xy*REFR_BUMP*shore)).rgb;

    // brighten up the refraction underwater
    refraction = (cameraPos.z < 0.0) ? clamp(refraction * 1.5, 0.0, 1.0) : refraction;
#endif
    // specular
    vec3 R = reflect(vVec, normal);
    float specular = pow(max(dot(R, lVec), 0.0),SPEC_HARDNESS) * shadow;

    vec3 waterColor = WATER_COLOR;
    waterColor = waterColor * length(gl_LightModel.ambient.xyz);

#if REFRACTION
    if (cameraPos.z > 0.0)
        refraction = mix(refraction, waterColor, clamp(depthSampleDistorted/VISIBILITY, 0.0, 1.0));

    gl_FragData[0].xyz = mix( mix(refraction,  scatterColour,  lightScatter),  reflection,  fresnel) + specular * gl_LightSource[0].specular.xyz;
#else
    gl_FragData[0].xyz = mix(reflection,  waterColor,  (1.0-fresnel)*0.5) + specular * gl_LightSource[0].specular.xyz;
#endif

    // fog
    float fogValue = clamp((depthPassthrough - gl_Fog.start) * gl_Fog.scale, 0.0, 1.0);
    gl_FragData[0].xyz = mix(gl_FragData[0].xyz,  gl_Fog.color.xyz,  fogValue);

#if REFRACTION
    gl_FragData[0].w = 1.0;
#else
    gl_FragData[0].w = clamp(fresnel*2.0 + specular, 0.0, 1.0);
#endif
}

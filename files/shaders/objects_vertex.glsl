#version 120

#if @diffuseMap
varying vec2 diffuseMapUV;
#endif

#if @darkMap
varying vec2 darkMapUV;
#endif

#if @detailMap
varying vec2 detailMapUV;
#endif

#if @decalMap
varying vec2 decalMapUV;
#endif

#if @emissiveMap
varying vec2 emissiveMapUV;
#endif

#if @normalMap
varying vec2 normalMapUV;
varying vec4 passTangent;
#endif

#if @envMap
varying vec2 envMapUV;
#endif

#if @bumpMap
varying vec2 bumpMapUV;
#endif

#if @specularMap
varying vec2 specularMapUV;
#endif

varying float euclideanDepth;
varying float linearDepth;

#define PER_PIXEL_LIGHTING (@normalMap || @forcePPL)

#if !PER_PIXEL_LIGHTING
centroid varying vec4 lighting;
centroid varying vec3 shadowDiffuseLighting;
#endif
centroid varying vec4 passColor;
varying vec3 passViewPos;
varying vec3 passNormal;

#include "shadows_vertex.glsl"

#include "lighting.glsl"


#if @grassAnimation
uniform float osg_SimulationTime;
uniform mat4 osg_ViewMatrixInverse;
uniform bool isGrass;
uniform float windSpeed;
uniform float Rotz;

vec2 rotate(vec2 v, float a)
{
    float s = sin(a);
    float c = cos(a);
    mat2 m = mat2(c, -s, s, c);
    return m * v;
}

vec2 grassDisplacement(vec4 worldpos, float h)
{
    vec2 windDirection = vec2(1.0);
    vec3 FootPos = osg_ViewMatrixInverse[3].xyz;
    vec3 WindVec = vec3(windSpeed * windDirection, 1.0);

    float v = length(WindVec);
    vec2 displace = vec2(2.0 * WindVec + 0.1);
    vec2 harmonics = vec2(0.0);

    harmonics += vec2((1.0 - 0.10*v) * sin(1.0*osg_SimulationTime + worldpos.xy / 1100.0));
    harmonics += vec2((1.0 - 0.04*v) * cos(2.0*osg_SimulationTime + worldpos.xy / 750.0));
    harmonics += vec2((1.0 + 0.14*v) * sin(3.0*osg_SimulationTime + worldpos.xy / 500.0));
    harmonics += vec2((1.0 + 0.28*v) * sin(5.0*osg_SimulationTime + worldpos.xy / 200.0));

    float d = length(worldpos.xy - FootPos.xy);
    vec2 stomp = vec2(0.0);
    if(d < 150.0) stomp = (60.0 / d - 0.4) * (worldpos.xy - FootPos.xy);
    return clamp(0.004 * h, 0.0, 1.0) * (harmonics * displace + stomp);
}
#endif

void main(void)
{
    vec4 viewPos = (gl_ModelViewMatrix * gl_Vertex);

#if @grassAnimation
if(isGrass)
{
    vec4 displacedVertex = gl_Vertex;
    vec4 worldPos = osg_ViewMatrixInverse * vec4(viewPos.xyz, 1.0);
    float height = 1.0-(gl_ModelViewMatrix[0].z-gl_Vertex.z);
    vec2 displ = grassDisplacement(worldPos, height);
    vec2 grassVertex = min(vec2(10.0), displ);

    displacedVertex.xy += rotate(grassVertex.xy, -1.0*Rotz);
    gl_Position = (gl_ModelViewProjectionMatrix * displacedVertex);
}
else
#endif

    gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;

    gl_ClipVertex = viewPos;
    euclideanDepth = length(viewPos.xyz);
    linearDepth = gl_Position.z;

#if (@envMap || !PER_PIXEL_LIGHTING || @shadows_enabled)
    vec3 viewNormal = normalize((gl_NormalMatrix * gl_Normal).xyz);
#endif

#if @envMap
    vec3 viewVec = normalize(viewPos.xyz);
    vec3 r = reflect( viewVec, viewNormal );
    float m = 2.0 * sqrt( r.x*r.x + r.y*r.y + (r.z+1.0)*(r.z+1.0) );
    envMapUV = vec2(r.x/m + 0.5, r.y/m + 0.5);
#endif

#if @diffuseMap
    diffuseMapUV = (gl_TextureMatrix[@diffuseMapUV] * gl_MultiTexCoord@diffuseMapUV).xy;
#endif

#if @darkMap
    darkMapUV = (gl_TextureMatrix[@darkMapUV] * gl_MultiTexCoord@darkMapUV).xy;
#endif

#if @detailMap
    detailMapUV = (gl_TextureMatrix[@detailMapUV] * gl_MultiTexCoord@detailMapUV).xy;
#endif

#if @decalMap
    decalMapUV = (gl_TextureMatrix[@decalMapUV] * gl_MultiTexCoord@decalMapUV).xy;
#endif

#if @emissiveMap
    emissiveMapUV = (gl_TextureMatrix[@emissiveMapUV] * gl_MultiTexCoord@emissiveMapUV).xy;
#endif

#if @normalMap
    normalMapUV = (gl_TextureMatrix[@normalMapUV] * gl_MultiTexCoord@normalMapUV).xy;
    passTangent = gl_MultiTexCoord7.xyzw;
#endif

#if @bumpMap
    bumpMapUV = (gl_TextureMatrix[@bumpMapUV] * gl_MultiTexCoord@bumpMapUV).xy;
#endif

#if @specularMap
    specularMapUV = (gl_TextureMatrix[@specularMapUV] * gl_MultiTexCoord@specularMapUV).xy;
#endif

#if !PER_PIXEL_LIGHTING
    lighting = doLighting(viewPos.xyz, viewNormal, gl_Color, shadowDiffuseLighting);
#endif
    passColor = gl_Color;
    passViewPos = viewPos.xyz;
    passNormal = gl_Normal.xyz;

#if (@shadows_enabled)
    setupShadowCoords(viewPos, viewNormal);
#endif
}

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

#if @pointsprite
uniform float axisScale;
uniform float visibilityDistance;
varying vec3 basic_prop; // _alive, _current_size, _current_alpha
#endif

varying float euclideanDepth;
varying float linearDepth;

#define PER_PIXEL_LIGHTING (@normalMap || @forcePPL)

#if !PER_PIXEL_LIGHTING
centroid varying vec4 lighting;
centroid varying vec3 shadowDiffuseLighting;
#else
centroid varying vec4 passColor;
#endif
varying vec3 passViewPos;
varying vec3 passNormal;

#include "shadows_vertex.glsl"

#include "lighting.glsl"

void main(void)
{
    vec4 viewPos = (gl_ModelViewMatrix * gl_Vertex);
    gl_Position = gl_ProjectionMatrix * viewPos;

    gl_ClipVertex = viewPos;

#if @pointsprite
    basic_prop = gl_MultiTexCoord0.xyz;

    float ecDepth = -viewPos.z;
    gl_PointSize = axisScale * basic_prop.y / ecDepth;

    if (visibilityDistance > 0.0)
    {
        if (ecDepth <= 0.0 || ecDepth >= visibilityDistance)
            basic_prop.x = -1.0;
    }
    vec3 viewNormal = normalize(gl_NormalMatrix * vec3(0.0, 0.0, 1.0));
#else
    vec3 viewNormal = normalize((gl_NormalMatrix * gl_Normal).xyz);
#endif

    euclideanDepth = length(viewPos.xyz);
    linearDepth = gl_Position.z;
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

    vec4 color = gl_Color;

#if @pointsprite
    color.a *= basic_prop.z;
    passNormal = vec3(0.0, 0.0, 1.0);
#else
    passNormal = gl_Normal.xyz;
#endif
#if !PER_PIXEL_LIGHTING
    lighting = doLighting(viewPos.xyz, viewNormal, color, shadowDiffuseLighting);
#else
    passColor = color;
#endif

    passViewPos = viewPos.xyz;

    setupShadowCoords(viewPos, viewNormal);
}

#version 120
#pragma import_defines(TERRAIN, OBJECT, DdiffuseMap, DnormalMap, DblendMap, DwriteNormals, DTerrainPass, Dlastpass)

varying vec2 diffuseMapUV;
varying float alphaPassthrough;

#include "lib/core/vertex.h.glsl"
#include "vertexcolors.glsl"

varying vec3 passNormal;
varying vec3 passViewPos;

#include "compatibility/normals.glsl"
varying vec2 uv;

void main()
{
    gl_Position = modelToClip(gl_Vertex);

    vec4 viewPos = modelToView(gl_Vertex);
    gl_ClipVertex = viewPos;

    if (colorMode == 2)
        alphaPassthrough = gl_Color.a;
    else
        alphaPassthrough = gl_FrontMaterial.diffuse.a;

    uv = gl_MultiTexCoord0.xy;

    passNormal = gl_Normal.xyz;
    passViewPos = viewPos.xyz;

    normalToViewMatrix = gl_NormalMatrix;

#if defined(DnormalMap) && DnormalMap
    mat3 tbnMatrix = generateTangentSpace(vec4(1.0, 0.0, 0.0, 1.0), passNormal);
    tbnMatrix[0] = normalize(cross(tbnMatrix[2], tbnMatrix[1])); // note, now we need to re-cross to derive tangent again because it wasn't orthonormal
    normalToViewMatrix *= tbnMatrix;
#endif

}

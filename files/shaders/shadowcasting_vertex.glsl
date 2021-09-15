#version 120

varying float alphaPassthrough;

uniform int colorMode;

void main(void)
{
    gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;

    vec4 viewPos = (gl_ModelViewMatrix * gl_Vertex);
    gl_ClipVertex = viewPos;

#if @diffuseMap
    diffuseMapUV = (gl_TextureMatrix[0] * gl_MultiTexCoord0).xy;
#endif

    if (colorMode == 2)
        alphaPassthrough = gl_Color.a;
    else
        // This is uniform, so if it's too low, we might be able to put the position/clip vertex outside the view frustum and skip the fragment shader and rasteriser
        alphaPassthrough = gl_FrontMaterial.diffuse.a;
}

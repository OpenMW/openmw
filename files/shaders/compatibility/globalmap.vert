#version 120

varying vec2 diffuseMapUV;

#if @alphaMap

varying vec2 alphaMapUV;

#endif

void main()
{
    gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;

    diffuseMapUV = gl_MultiTexCoord0.xy;

#if @alphaMap
    alphaMapUV = gl_MultiTexCoord1.xy;
#endif
}

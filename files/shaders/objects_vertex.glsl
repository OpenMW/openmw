#version 120

#if @diffuseMap
varying vec2 diffuseMapUV;
#endif

varying float depth;
    
void main(void)
{
    gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
    depth = gl_Position.z;

#if @diffuseMap
    diffuseMapUV = gl_MultiTexCoord@diffuseMapUV.xy;
#endif
}

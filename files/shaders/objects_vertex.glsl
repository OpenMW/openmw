#version 120

#if @diffuseMap
varying vec2 diffuseMapUV;
#endif
    
void main(void)
{
    gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;

#if @diffuseMap
    diffuseMapUV = gl_MultiTexCoord@diffuseMapUV.xy;
#endif
}

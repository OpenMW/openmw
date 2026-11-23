#version 120

uniform float osg_FrameTime;
uniform vec2 turnCenter;
uniform float turnSpeed;

varying vec2 diffuseMapUV;

void main()
{
    float angle = turnSpeed * osg_FrameTime;
    mat2 rotation = mat2(cos(angle), sin(angle), -sin(angle), cos(angle));
    vec2 position = turnCenter + rotation * (gl_Vertex.xy - turnCenter);
    gl_Position = gl_ModelViewProjectionMatrix * vec4(position, gl_Vertex.zw);
    diffuseMapUV = gl_MultiTexCoord0.xy;
}

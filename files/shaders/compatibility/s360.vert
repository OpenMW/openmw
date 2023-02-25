#version 120

varying vec2 uv;

void main(void)
{
    gl_Position = gl_Vertex;
    uv = (gl_Vertex.xy * vec2(1.0,-1.0) + vec2(1.0)) / 2;
}

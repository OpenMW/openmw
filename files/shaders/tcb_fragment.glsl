#version 120

void main()
{
    gl_FragDepth = (log(gl_TexCoord[6].z + 1.0) / log(1000000000.0 + 1.0));
}

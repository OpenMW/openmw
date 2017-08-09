#version 120
    
varying vec3  screenCoordsPassthrough;
varying vec4  position;
varying float  depthPassthrough;

void main(void)
{
    gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;

    mat4 scalemat = mat4(0.5, 0.0, 0.0, 0.0,
		 0.0, -0.5, 0.0, 0.0,
		 0.0, 0.0, 0.5, 0.0,
		 0.5, 0.5, 0.5, 1.0);

    vec4 texcoordProj = ((scalemat) * ( gl_Position));
    screenCoordsPassthrough = vec3(texcoordProj.x, texcoordProj.y, texcoordProj.w);

    position = gl_Vertex;

    depthPassthrough = gl_Position.z;
}

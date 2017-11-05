#version 120
    
varying vec3  screenCoordsPassthrough;
varying vec4  position;
varying float  depthPassthrough;

uniform int shadowTextureUnit0;
varying vec4 shadowSpaceCoords;

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

	// This matrix has the opposite handedness to the others used here, so multiplication must have the vector to the left. Alternatively it could be transposed after construction, but that's extra work for the GPU just to make the code look a tiny bit cleaner.
	mat4 eyePlaneMat = mat4(gl_EyePlaneS[shadowTextureUnit0], gl_EyePlaneT[shadowTextureUnit0], gl_EyePlaneR[shadowTextureUnit0], gl_EyePlaneQ[shadowTextureUnit0]);
	shadowSpaceCoords = (gl_ModelViewMatrix * gl_Vertex) * eyePlaneMat;
}

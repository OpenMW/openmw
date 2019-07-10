uniform float axisScale;
uniform float visibilityDistance;
varying vec3 basic_prop; // _alive, _current_size, _current_alpha

//TOFIX
#define SIZESCALE 1000

void main(void)
{

    basic_prop = gl_MultiTexCoord0.xyz;
    vec4 ecPos = gl_ModelViewMatrix * gl_Vertex;

    float ecDepth = -ecPos.z;
    gl_PointSize = SIZESCALE * axisScale * basic_prop.y / (ecDepth);

    if (visibilityDistance > 0.0)
    {
        if (ecDepth <= 0.0 || ecDepth >= visibilityDistance)
            basic_prop.x = -1.0;
    }

    gl_Position = gl_ProjectionMatrix * ecPos;
    gl_ClipVertex = ecPos;

    vec4 color = gl_Color;
    color.a *= basic_prop.z;

    gl_FrontColor = color;
    gl_BackColor = gl_FrontColor;

};

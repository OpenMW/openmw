uniform sampler2D baseTexture;
varying vec3 basic_prop;

void main(void)
{
    if (basic_prop.x < 0.0) discard;
    gl_FragColor = gl_Color* texture2D(baseTexture, gl_PointCoord.xy);
};

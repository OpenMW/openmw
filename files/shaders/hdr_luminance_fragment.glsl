#version 120

varying vec2 uv;
uniform sampler2D sceneTex;

void main()
{
    float lum = dot(texture2D(sceneTex, uv).rgb, vec3(0.2126, 0.7152, 0.0722));

    if (lum < @epsilon)
    {
        gl_FragColor.r = 0.0;
        return;
    }

    gl_FragColor.r = clamp((log2(lum) - @minLog) * @invLogLumRange, 0.0, 1.0);
}

#version 120

uniform sampler2D diffuseMap;

varying vec2 diffuseMapUV;
varying float alphaPassthrough;
varying vec3 passNormal;

void main()
{
    float alpha = texture2D(diffuseMap, diffuseMapUV).a * alphaPassthrough;

    const float alphaRef = 0.499;

    vec3 viewNormal = normalize(gl_NormalMatrix * passNormal);
    gl_FragData[0] = vec4(viewNormal * 0.5 + 0.5, 1.0);

    if (alpha < alphaRef)
        discard;

    // DO NOT write to color!
    // This is in charge of only updating depth buffer.
}

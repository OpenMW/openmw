#version 120

uniform sampler2D diffuseMap;

varying vec2 diffuseMapUV;
varying float alphaPassthrough;

void main()
{
    float alpha = texture2D(diffuseMap, diffuseMapUV).a * alphaPassthrough;

    const float alphaRef = 0.499;

    if (alpha < alphaRef)
        discard;

    // DO NOT write to color!
    // This is in charge of only updating depth buffer.
}

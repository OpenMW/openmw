uniform float far;

#if @skyBlending
uniform sampler2D sky;
uniform float skyBlendingStart;
#endif

vec4 applyFogAtDist(vec4 color, float euclideanDist, float linearDist)
{
#if @radialFog
    float dist = euclideanDist;
#else
    float dist = abs(linearDist);
#endif
#if @exponentialFog
    float fogValue = 1.0 - exp(-2.0 * max(0.0, dist - gl_Fog.start/2.0) / (gl_Fog.end - gl_Fog.start/2.0));
#else
    float fogValue = clamp((dist - gl_Fog.start) * gl_Fog.scale, 0.0, 1.0);
#endif
    color.xyz = mix(color.xyz, gl_Fog.color.xyz, fogValue);

#if @skyBlending && !@useOVR_multiview
    float fadeValue = clamp((far - dist) / (far - skyBlendingStart), 0.0, 1.0);
    vec3 skyColor = texture2D(sky, gl_FragCoord.xy / screenRes).xyz;
    color.xyz = mix(skyColor, color.xyz, fadeValue * fadeValue);
#endif

    return color;
}

vec4 applyFogAtPos(vec4 color, vec3 pos)
{
    return applyFogAtDist(color, length(pos), pos.z);
}

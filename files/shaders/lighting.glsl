#define MAX_LIGHTS 8

vec3 perLight(int lightIndex, vec3 viewPos, vec3 viewNormal, vec4 diffuse, vec3 ambient)
{
    vec3 lightDir;
    float d;

	lightDir = gl_LightSource[lightIndex].position.xyz - (viewPos.xyz * gl_LightSource[lightIndex].position.w);
    d = length(lightDir);
    lightDir = normalize(lightDir);

    return (ambient * gl_LightSource[lightIndex].ambient.xyz + diffuse.xyz * gl_LightSource[lightIndex].diffuse.xyz * max(dot(viewNormal.xyz, lightDir), 0.0)) * clamp(1.0 / (gl_LightSource[lightIndex].constantAttenuation + gl_LightSource[lightIndex].linearAttenuation * d + gl_LightSource[lightIndex].quadraticAttenuation * d * d), 0.0, 1.0);
}

#ifdef FRAGMENT
vec4 doLighting(vec3 viewPos, vec3 viewNormal, vec4 vertexColor, float shadowing)
#else
vec4 doLighting(vec3 viewPos, vec3 viewNormal, vec4 vertexColor)
#endif
{
#if @colorMode == 3
    vec4 diffuse = gl_FrontMaterial.diffuse;
    vec3 ambient = vertexColor.xyz;
#elif @colorMode == 2
    vec4 diffuse = vertexColor;
    vec3 ambient = vertexColor.xyz;
#else
    vec4 diffuse = gl_FrontMaterial.diffuse;
    vec3 ambient = gl_FrontMaterial.ambient.xyz;
#endif
    vec4 lightResult = vec4(0.0, 0.0, 0.0, diffuse.a);

#ifdef FRAGMENT
	lightResult.xyz += perLight(0, viewPos, viewNormal, diffuse, ambient) * shadowing;
#else
	lightResult.xyz += perLight(0, viewPos, viewNormal, diffuse, ambient);
#endif
    for (int i=1; i<MAX_LIGHTS; ++i)
    {
        lightResult.xyz += perLight(i, viewPos, viewNormal, diffuse, ambient);
    }

    lightResult.xyz += gl_LightModel.ambient.xyz * ambient;

#if @colorMode == 1
    lightResult.xyz += vertexColor.xyz;
#else
    lightResult.xyz += gl_FrontMaterial.emission.xyz;
#endif

#if @clamp
    lightResult = clamp(lightResult, vec4(0.0, 0.0, 0.0, 0.0), vec4(1.0, 1.0, 1.0, 1.0));
#else
    lightResult = max(lightResult, 0.0);
#endif
    return lightResult;
}


vec3 getSpecular(vec3 viewNormal, vec3 viewDirection, float shininess, vec3 matSpec)
{
    vec3 lightDir = normalize(gl_LightSource[0].position.xyz);
    float NdotL = max(dot(viewNormal, lightDir), 0.0);
    if (NdotL < 0)
        return vec3(0,0,0);
    vec3 halfVec = normalize(lightDir - viewDirection);
    return pow(max(dot(viewNormal, halfVec), 0.0), 128) * gl_LightSource[0].specular.xyz * matSpec;
}

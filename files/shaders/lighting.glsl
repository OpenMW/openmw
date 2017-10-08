#define MAX_LIGHTS 8

void perLight(out vec3 ambientOut, out vec3 diffuseOut, int lightIndex, vec3 viewPos, vec3 viewNormal, vec4 diffuse, vec3 ambient)
{
    vec3 lightDir;
	float lightDistance;

    lightDir = gl_LightSource[lightIndex].position.xyz - (viewPos.xyz * gl_LightSource[lightIndex].position.w);
    lightDistance = length(lightDir);
    lightDir = normalize(lightDir);
	float illumination = clamp(1.0 / (gl_LightSource[lightIndex].constantAttenuation + gl_LightSource[lightIndex].linearAttenuation * lightDistance + gl_LightSource[lightIndex].quadraticAttenuation * lightDistance * lightDistance), 0.0, 1.0);

    ambientOut = ambient * gl_LightSource[lightIndex].ambient.xyz * illumination;
    diffuseOut = diffuse.xyz * gl_LightSource[lightIndex].diffuse.xyz * max(dot(viewNormal.xyz, lightDir), 0.0) * illumination;
}

#if PER_PIXEL_LIGHTING
vec4 doLighting(vec3 viewPos, vec3 viewNormal, vec4 vertexColor, float shadowing)
#else
vec4 doLighting(vec3 viewPos, vec3 viewNormal, vec4 vertexColor, out vec3 shadowDiffuse)
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

    vec3 diffuseLight, ambientLight;
    perLight(ambientLight, diffuseLight, 0, viewPos, viewNormal, diffuse, ambient);
#if PER_PIXEL_LIGHTING
    lightResult.xyz += ambientLight + diffuseLight * shadowing;
#else
    shadowDiffuse = diffuseLight;
	lightResult.xyz += ambientLight;
#endif
    for (int i=1; i<MAX_LIGHTS; ++i)
    {
        perLight(ambientLight, diffuseLight, i, viewPos, viewNormal, diffuse, ambient);
        lightResult.xyz += ambientLight + diffuseLight;
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

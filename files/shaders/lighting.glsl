#define MAX_LIGHTS 8

uniform int colorMode;

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
    vec4 diffuse;
    vec3 ambient;
    if (colorMode == 3)
    {
        diffuse = gl_FrontMaterial.diffuse;
        ambient = vertexColor.xyz;
    }
    else if (colorMode == 2)
    {
        diffuse = vertexColor;
        ambient = vertexColor.xyz;
    }
    else
    {
        diffuse = gl_FrontMaterial.diffuse;
        ambient = gl_FrontMaterial.ambient.xyz;
    }
    vec4 lightResult = vec4(0.0, 0.0, 0.0, diffuse.a);

    vec3 diffuseLight, ambientLight;
    perLight(ambientLight, diffuseLight, 0, viewPos, viewNormal, diffuse, ambient);
#if PER_PIXEL_LIGHTING
    lightResult.xyz += diffuseLight * shadowing - diffuseLight; // This light gets added a second time in the loop to fix Mesa users' slowdown, so we need to negate its contribution here.
#else
    shadowDiffuse = diffuseLight;
    lightResult.xyz -= shadowDiffuse; // This light gets added a second time in the loop to fix Mesa users' slowdown, so we need to negate its contribution here.
#endif
    for (int i=0; i<MAX_LIGHTS; ++i)
    {
        perLight(ambientLight, diffuseLight, i, viewPos, viewNormal, diffuse, ambient);
        lightResult.xyz += ambientLight + diffuseLight;
    }

    lightResult.xyz += gl_LightModel.ambient.xyz * ambient;

    if (colorMode == 1)
        lightResult.xyz += vertexColor.xyz;
    else
        lightResult.xyz += gl_FrontMaterial.emission.xyz;

#if @clamp
    lightResult = clamp(lightResult, vec4(0.0), vec4(1.0));
#else
    lightResult = max(lightResult, 0.0);
#endif
    return lightResult;
}


vec3 getSpecular(vec3 viewNormal, vec3 viewDirection, float shininess, vec3 matSpec)
{
    vec3 lightDir = normalize(gl_LightSource[0].position.xyz);
    float NdotL = dot(viewNormal, lightDir);
    if (NdotL <= 0.0)
        return vec3(0.,0.,0.);
    vec3 halfVec = normalize(lightDir - viewDirection);
    float NdotH = dot(viewNormal, halfVec);
    return pow(max(NdotH, 0.0), max(1e-4, shininess)) * gl_LightSource[0].specular.xyz * matSpec;
}

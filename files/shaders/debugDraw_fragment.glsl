#version 330 compatibility

in vec3 vertexColor;
in vec3 vertexNormal;

out vec4 fragColor;

void main()
{
    vec3 lightDir = normalize(vec3(-1.,-2.,-0.5));

    float lightAttenuation = dot(-lightDir, vertexNormal) * 0.5  + 0.5; 

    fragColor = vec4(vertexColor * lightAttenuation, 1.);
}

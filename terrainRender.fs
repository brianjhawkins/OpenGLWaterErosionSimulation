#version 460 core
out vec4 FragColor;

in vec3 Normal;
in vec3 FragPos;
in vec2 TexCoords;
in vec3 VertexColor;
in vec3 VertexSpecularColor;
in float VertexShininess;

struct DirLight{
	vec3 direction;

	// phong lighting variables
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
};

uniform vec3 viewPos;
uniform DirLight dirLight;

vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir);

void main()
{    
    // properties
	vec3 norm = normalize(Normal);
	vec3 viewDir = normalize(viewPos - FragPos);

	// Directional Light
	vec3 result = CalcDirLight(dirLight, norm, viewDir);

	FragColor = vec4(result, 1.0f);
}

vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir){
	vec3 lightDir = normalize(-light.direction);

	// Diffuse Shading
	float diff = max(dot(normal, lightDir), 0.0f);

	// Specular Shading
	vec3 reflectDir = reflect(-lightDir, normal);
	float spec = pow(max(dot(viewDir, reflectDir), 0.0f), VertexShininess);

	// Combine Results
	vec3 ambient = light.ambient * VertexColor;
	vec3 diffuse = light.diffuse * diff * VertexColor;
	vec3 specular = light.specular * spec * VertexSpecularColor;

	return (ambient + diffuse + specular);
}
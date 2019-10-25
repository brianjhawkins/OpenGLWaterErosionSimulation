#version 460 core
out vec4 FragColor;

in vec3 Normal;
in vec3 FragPos;
in vec2 TexCoords;

struct DirLight{
	vec3 direction;

	// phong lighting variables
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
};

uniform vec3 viewPos;
uniform sampler2D terrainTexture;
uniform vec3 terrainColor;
uniform vec3 terrainSpecularColor;
uniform vec3 waterColor;
uniform vec3 waterSpecularColor;
uniform float terrainShininess;
uniform float waterShininess;
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
	vec4 terrainValue = texture(terrainTexture, TexCoords);
	vec3 renderColor;
	vec3 specularColor;
	float shininess;
	
	if(terrainValue.r > 0.0f){
		renderColor = mix(waterColor, terrainColor, clamp(0.8f - terrainValue.r * 50, 0.0f, 1.0f));
		specularColor = waterSpecularColor;
		shininess = waterShininess;
	}
	else{
		renderColor = terrainColor;
		specularColor = terrainSpecularColor;
		shininess = terrainShininess;
	}

	normalize(renderColor);

	// Diffuse Shading
	float diff = max(dot(normal, lightDir), 0.0f);

	// Specular Shading
	vec3 reflectDir = reflect(-lightDir, normal);
	float spec = pow(max(dot(viewDir, reflectDir), 0.0f), shininess);

	// Combine Results
	vec3 ambient = light.ambient * renderColor;
	vec3 diffuse = light.diffuse * diff * renderColor;
	vec3 specular = light.specular * spec * specularColor;

	return normalize(ambient + diffuse + specular);
}
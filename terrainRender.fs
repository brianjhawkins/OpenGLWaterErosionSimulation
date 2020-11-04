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
uniform float baseTerrainFrequency;
uniform float baseTerrainAmplitude;
uniform float baseGrassFrequency;
uniform float baseGrassAmplitude;

vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir);
float Random(vec2 coordinates);
float Noise(vec2 coordinates);

void main()
{    
	vec3 result = vec3(0, 0, 0);

    // properties
	vec3 norm = normalize(Normal);

	if(VertexColor.r > VertexColor.g){
		result += Noise(TexCoords * baseTerrainFrequency) * baseTerrainAmplitude;
		result += Noise(TexCoords * baseTerrainFrequency * 3.0f) * baseTerrainAmplitude / 2.0f;
		result += Noise(TexCoords * baseTerrainFrequency * 9.0f) * baseTerrainAmplitude / 4.0f;
	}
	else{
		result += Noise(TexCoords * baseGrassFrequency) * baseGrassAmplitude;
		result += Noise(TexCoords * baseGrassFrequency * 14.0f) * baseGrassAmplitude / 2.0f;
		result += Noise(TexCoords * baseGrassFrequency * 30.0f) * baseGrassAmplitude / 4.0f;
	}

	vec3 viewDir = normalize(viewPos - FragPos);

	// Directional Light
	result += CalcDirLight(dirLight, norm, viewDir);

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

// 2D Random
float Random (vec2 st) {
    return fract(sin(dot(st.xy,
                         vec2(12.9898,78.233)))
                 * 43758.5453123);
}

// 2D Noise based on Morgan McGuire @morgan3d
// https://www.shadertoy.com/view/4dS3Wd
float Noise (vec2 st) {
    vec2 i = floor(st);
    vec2 f = fract(st);

    // Four corners in 2D of a tile
    float a = Random(i);
    float b = Random(i + vec2(1.0, 0.0));
    float c = Random(i + vec2(0.0, 1.0));
    float d = Random(i + vec2(1.0, 1.0));

    // Smooth Interpolation

    // Cubic Hermine Curve.  Same as SmoothStep()
    vec2 u = f*f*(3.0-2.0*f);
    // u = smoothstep(0.,1.,f);

    // Mix 4 coorners percentages
    return mix(a, b, u.x) +
            (c - a)* u.y * (1.0 - u.x) +
            (d - b) * u.x * u.y;
}
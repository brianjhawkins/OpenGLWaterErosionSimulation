#version 460 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoords;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

uniform sampler2D columnDataTexture;
uniform sampler2D waterDataTexture;
uniform float size;

uniform vec3 terrainColor;
uniform vec3 vegetationColor;
uniform vec3 waterColor;
uniform vec3 terrainSpecularColor;
uniform vec3 waterSpecularColor;
uniform float terrainShininess;
uniform float waterShininess;

out vec3 Normal;
out vec3 FragPos;
out vec2 TexCoords;
out vec3 VertexColor;
out vec3 VertexSpecularColor;
out float VertexShininess;
out float Opacity;

float Height(vec4 v){
	return (v.r + v.b + v.a) * size;
}

void main()
{
	vec4 columnDataTextureValue = texture(columnDataTexture, aTexCoords);
	vec4 waterDataTextureValue = texture(waterDataTexture, aTexCoords);
	
	vec3 newPosition = aPos;
	float newY = Height(columnDataTextureValue);
	newPosition.y = newY;
	
	vec3 newNormal;
	vec2 textureSize = textureSize(columnDataTexture, 0);
	vec2 texelSize = vec2(1.0f / textureSize.x, 1.0f / textureSize.y);
	vec4 left = textureOffset(columnDataTexture, aTexCoords, ivec2(-1, 0));
	vec4 right = textureOffset(columnDataTexture, aTexCoords, ivec2(1, 0));
	vec4 top = textureOffset(columnDataTexture, aTexCoords, ivec2(0, 1));
	vec4 bottom = textureOffset(columnDataTexture, aTexCoords, ivec2(0, -1));

	VertexColor = mix(waterColor, terrainColor, waterDataTextureValue.r * 10);
	VertexSpecularColor = waterSpecularColor;
	VertexShininess = waterShininess;

	Opacity = mix(0.3f, 1.0f, waterDataTextureValue.r * 10);

	float centerHeight = Height(columnDataTextureValue);
	float leftHeight = Height(left);
	float rightHeight = Height(right);
	float topHeight = Height(top);
	float bottomHeight = Height(bottom);

	if(left.r == 0){
		leftHeight = centerHeight;
	}

	if(right.r == 0){
		rightHeight = centerHeight;
	}

	if(top.r == 0){
		topHeight = centerHeight;
	}

	if(bottom.r == 0){
		bottomHeight = centerHeight;
	}

	newNormal = vec3(leftHeight - rightHeight, 2 * texelSize.x, bottomHeight - topHeight);
	newNormal = normalize(newNormal);

    gl_Position = projection * view * model * vec4(newPosition, 1.0);
	Normal = mat3(transpose(inverse(model))) * newNormal;
	FragPos = vec3(model * vec4(newPosition, 1.0));
	TexCoords = aTexCoords;	
}
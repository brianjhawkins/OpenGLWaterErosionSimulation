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

float Height(vec4 c, vec4 w){
	return (c.r + c.g + w.a + c.b + c.a) * size;
}

void main()
{
	vec4 columnDataTextureValue = texture(columnDataTexture, aTexCoords);
	vec4 waterDataTextureValue = texture(waterDataTexture, aTexCoords);
	
	vec3 newPosition = aPos;
	float newY = Height(columnDataTextureValue, waterDataTextureValue);
	newPosition.y = newY;
	
	vec3 newNormal;
	vec2 textureSize = textureSize(columnDataTexture, 0);
	vec2 texelSize = vec2(1.0f / textureSize.x, 1.0f / textureSize.y);
	vec4 leftColumnData = textureOffset(columnDataTexture, aTexCoords, ivec2(-1, 0));
	vec4 leftWaterData = textureOffset(waterDataTexture, aTexCoords, ivec2(-1, 0));
	vec4 rightColumnData = textureOffset(columnDataTexture, aTexCoords, ivec2(1, 0));
	vec4 rightWaterData = textureOffset(waterDataTexture, aTexCoords, ivec2(1, 0));
	vec4 topColumnData = textureOffset(columnDataTexture, aTexCoords, ivec2(0, 1));
	vec4 topWaterData = textureOffset(waterDataTexture, aTexCoords, ivec2(0, 1));
	vec4 bottomColumnData = textureOffset(columnDataTexture, aTexCoords, ivec2(0, -1));
	vec4 bottomWaterData = textureOffset(waterDataTexture, aTexCoords, ivec2(0, -1));

	VertexColor = mix(waterColor, terrainColor, waterDataTextureValue.r * 10);
	VertexSpecularColor = waterSpecularColor;
	VertexShininess = waterShininess;

	Opacity = mix(0.3f, 1.0f, (waterDataTextureValue.r + waterDataTextureValue.g) * 10);

	float centerHeight = newY;
	float leftHeight = Height(leftColumnData, leftWaterData);
	float rightHeight = Height(rightColumnData, rightWaterData);
	float topHeight = Height(topColumnData, topWaterData);
	float bottomHeight = Height(bottomColumnData, bottomWaterData);

	if(leftColumnData.r == 0){
		leftHeight = centerHeight;
	}

	if(rightColumnData.r == 0){
		rightHeight = centerHeight;
	}

	if(topColumnData.r == 0){
		topHeight = centerHeight;
	}

	if(bottomColumnData.r == 0){
		bottomHeight = centerHeight;
	}

	newNormal = vec3(leftHeight - rightHeight, 2 * texelSize.x, bottomHeight - topHeight);
	newNormal = normalize(newNormal);

    gl_Position = projection * view * model * vec4(newPosition, 1.0);
	Normal = mat3(transpose(inverse(model))) * newNormal;
	FragPos = vec3(model * vec4(newPosition, 1.0));
	TexCoords = aTexCoords;	
}
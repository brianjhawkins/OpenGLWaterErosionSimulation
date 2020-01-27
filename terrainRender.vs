#version 460 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoords;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

uniform sampler2D terrainTexture;
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

void main()
{
	vec4 terrainTextureValue = texture(terrainTexture, aTexCoords);
	
	vec3 newPosition = aPos;
	float newY = (terrainTextureValue.r + terrainTextureValue.g) * size;
	newPosition.y = newY;
	
	vec3 newNormal;
	vec2 textureSize = textureSize(terrainTexture, 0);
	vec2 texelSize = vec2(1.0f / textureSize.x, 1.0f / textureSize.y);
	vec2 left = textureOffset(terrainTexture, aTexCoords, ivec2(-1, 0)).rg;
	vec2 right = textureOffset(terrainTexture, aTexCoords, ivec2(1, 0)).rg;
	vec2 top = textureOffset(terrainTexture, aTexCoords, ivec2(0, 1)).rg;
	vec2 bottom = textureOffset(terrainTexture, aTexCoords, ivec2(0, -1)).rg;

	float leftHeight;
	float rightHeight;
	float topHeight;
	float bottomHeight;

	vec3 tempTerrainColor = mix(terrainColor, vegetationColor, max(0, terrainTextureValue.a * 5));

	if(terrainTextureValue.r > 0.0f){
		VertexColor = mix(waterColor, tempTerrainColor, clamp(0.8f - terrainTextureValue.r * 30, 0.0f, 1.0f));
		VertexSpecularColor = waterSpecularColor;
		VertexShininess = waterShininess;

		leftHeight = (left.r + left.g) * size;
		rightHeight = (right.r + right.g) * size;
		topHeight = (top.r + top.g) * size;
		bottomHeight = (bottom.r + bottom.g) * size;
	} else {
		VertexColor = tempTerrainColor;
		VertexSpecularColor = terrainSpecularColor;
		VertexShininess = terrainShininess;

		leftHeight = left.g * size;
		rightHeight = right.g * size;
		topHeight = top.g * size;
		bottomHeight = bottom.g * size;	
	}

	newNormal = vec3(leftHeight - rightHeight, 2 * texelSize.x, bottomHeight - topHeight);
	newNormal = normalize(newNormal);

    gl_Position = projection * view * model * vec4(newPosition, 1.0);
	Normal = mat3(transpose(inverse(model))) * newNormal;
	FragPos = vec3(model * vec4(newPosition, 1.0));
	TexCoords = aTexCoords;	
}
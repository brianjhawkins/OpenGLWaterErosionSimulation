#version 460 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

uniform sampler2D terrainTexture;
uniform float size;

out vec3 Normal;
out vec3 FragPos;
out vec2 TexCoords;

void main()
{
	vec4 terrainTextureValue = texture(terrainTexture, aTexCoords);
	
	vec3 newPosition = aPos;
	float newY = (terrainTextureValue.x + terrainTextureValue.y) * size;
	newPosition.y = newY;
	
	vec3 newNormal;
	vec2 textureSize = textureSize(terrainTexture, 0);
	vec2 texelSize = vec2(1.0f / textureSize.x, 1.0f / textureSize.y);
	vec4 adjacentTerrainTextureValue1 = textureOffset(terrainTexture, aTexCoords, ivec2(1, 0));
	vec4 adjacentTerrainTextureValue2 = textureOffset(terrainTexture, aTexCoords, ivec2(0, 1));
	float height1 = (adjacentTerrainTextureValue1.x + adjacentTerrainTextureValue1.y) * size;
	float height2 = (adjacentTerrainTextureValue2.x + adjacentTerrainTextureValue2.y) * size;
	vec3 tangent1 = normalize(vec3(texelSize.x, height1 - newY, 0));
	vec3 tangent2 = normalize(vec3(0, height1 - newY, texelSize.y));
	newNormal = normalize(cross(tangent2, tangent1));

    gl_Position = projection * view * model * vec4(newPosition, 1.0);
	Normal = mat3(transpose(inverse(model))) * newNormal;
	FragPos = vec3(model * vec4(newPosition, 1.0));
	TexCoords = aTexCoords;
}
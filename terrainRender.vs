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
	vec2 left = textureOffset(terrainTexture, aTexCoords, ivec2(-1, 0)).xy;
	vec2 right = textureOffset(terrainTexture, aTexCoords, ivec2(1, 0)).xy;
	vec2 up = textureOffset(terrainTexture, aTexCoords, ivec2(0, 1)).xy;
	vec2 down = textureOffset(terrainTexture, aTexCoords, ivec2(0, -1)).xy;
	float leftHeight = (left.x + left.y) * size;
	float rightHeight = (right.x + right.y) * size;
	float upHeight = (up.x + up.y) * size;
	float downHeight = (down.x + down.y) * size;
	vec3 vLeft = normalize(vec3(-texelSize.x, leftHeight - newY, 0.0f));
	vec3 vRight = normalize(vec3(texelSize.x, rightHeight - newY, 0.0f));
	vec3 vUp = normalize(vec3(0.0f, upHeight - newY, texelSize.y));
	vec3 vDown = normalize(vec3(0.0f, downHeight - newY, -texelSize.y));
	vec3 averageNormal = (cross(vUp, vRight) + cross(vRight, vDown) + cross(vDown, vLeft) + cross(vLeft, vUp)) / -4;
	newNormal = normalize(averageNormal);

    gl_Position = projection * view * model * vec4(newPosition, 1.0);
	Normal = mat3(transpose(inverse(model))) * newNormal;
	FragPos = vec3(model * vec4(newPosition, 1.0));
	TexCoords = aTexCoords;
}
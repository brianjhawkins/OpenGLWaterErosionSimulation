#version 460 core
layout(location = 0) out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D CDTexture;

void main()
{    
    FragColor = texture(CDTexture, TexCoords);
}
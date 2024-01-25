#version 330 core

layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexture;

uniform mat4 projection;
uniform mat4 view;

out vec2 texCoord;
void main()
{
    gl_Position = projection * view * vec4(aPos, 0.0, 1.0);
    texCoord = aTexture;
}
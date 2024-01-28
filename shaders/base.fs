#version 330 core

out vec4 FragColor;

in vec2 texCoord;

uniform sampler2D texture1;
uniform vec3 color;
uniform bool inverted;

void main()
{
    if(inverted)
        FragColor = texture(texture1, texCoord);
    else 
        FragColor = vec4(color, 1.0f) * texture(texture1, texCoord);
}
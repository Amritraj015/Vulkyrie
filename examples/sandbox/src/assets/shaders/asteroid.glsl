#type vertex
#version 460 core

layout(location = 0) in vec3 aPos;
layout(location = 2) in vec2 aTexCoords;
layout(location = 3) in mat4 instanceMatrix;

out vec2 TexCoords;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 orbitRotation;

void main()
{
    gl_Position = projection * view * orbitRotation * instanceMatrix * vec4(aPos, 1.0);
    TexCoords = aTexCoords;
}

#type fragment
#version 460 core

out vec4 FragColor;

in vec2 TexCoords;

layout(binding = 0) uniform sampler2D texture_diffuse;

void main()
{
    FragColor = texture(texture_diffuse, TexCoords);
}

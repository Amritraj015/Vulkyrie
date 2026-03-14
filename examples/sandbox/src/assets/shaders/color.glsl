#type vertex
#version 460 core

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTextureCoords;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    gl_Position = projection * view * model * vec4(aPosition, 1.0);
}

#type fragment
#version 460 core
out vec4 frag_color;

uniform vec3 color;

void main()
{
    frag_color = vec4(color, 1.0);
}

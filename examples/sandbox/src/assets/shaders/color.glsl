#type vertex
#version 460 core

layout(location = 0) in vec3 position;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    gl_Position = projection * view * model * vec4(position, 1.0);
}

#type fragment
#version 460 core
out vec4 frag_color;

layout(location = 0) uniform vec3 color;

void main()
{
    frag_color = vec4(color, 1.0);
}

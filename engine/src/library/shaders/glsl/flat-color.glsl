#type fragment
#version 460 core

layout(location = 0) in vec3 position;

uniform mat4 model;
uniform mat4 projection;
uniform mat4 view;

void main() {
    gl_Position = projection * view * model * vec4(position, 1.0f);
}

#type vertex
#version 460 core

out vec4 fragment_color;

uniform mat4 color;

void main() {
    fragment_color = color;
}

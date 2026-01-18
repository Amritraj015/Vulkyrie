#type vertex
#version 460 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 aColor;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 vColor;

void main() {
    vColor = aColor;
    gl_Position = projection * view * model * vec4(position, 1.0);
}

#type fragment
#version 460 core

in vec3 vColor;
out vec4 FragColor;

void main() {
    FragColor = vec4(vColor, 1.0);
}

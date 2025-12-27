#version 330 core

layout(location = 0) in vec3 triangleCoordinates;
layout(location = 1) in vec3 vertexColor;

out vec3 ourColor;

void main()
{
    gl_Position = vec4(triangleCoordinates, 1.0);
    ourColor = vertexColor;
}

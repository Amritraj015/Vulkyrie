#type vertex
#version 460 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec2 texture_coordinates;

out vec2 TexCoord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    gl_Position = projection * view * model * vec4(position, 1.0);
    TexCoord = vec2(texture_coordinates.x, texture_coordinates.y);
}

#type fragment
#version 460 core
out vec4 FragColor;

in vec2 TexCoord;

layout(binding = 0) uniform sampler2D texture1;

void main()
{
    FragColor = texture(texture1, TexCoord);
    // FragColor = vec4(vec3(gl_FragCoord.z), 1.0);
}

// float near = 0.1;
// float far = 100.0;
//
// float LinearizeDepth(float depth)
// {
//     float z = depth * 2.0 - 1.0; // back to NDC
//     return (2.0 * near * far) / (far + near - z * (far - near));
// }
//
// void main()
// {
//     float depth = LinearizeDepth(gl_FragCoord.z) / far; // divide by far for demonstration
//     FragColor = vec4(vec3(depth), 1.0);
// }

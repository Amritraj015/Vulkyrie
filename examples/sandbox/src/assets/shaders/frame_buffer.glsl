#type vertex
#version 460 core
layout(location = 0) in vec2 position;
layout(location = 1) in vec2 texture_coordinates;

out vec2 TexCoords;

void main()
{
    gl_Position = vec4(position.x, position.y, 0.0, 1.0);
    TexCoords = texture_coordinates;
}

#type fragment
#version 460 core
out vec4 FragColor;

in vec2 TexCoords;

layout(binding = 0) uniform sampler2D screenTexture;

const float offset = 1.0 / 300.0;

void main()
{
    vec2 offsets[9] = vec2[](
            vec2(-offset, offset), // top-left
            vec2(0.0f, offset), // top-center
            vec2(offset, offset), // top-right
            vec2(-offset, 0.0f), // center-left
            vec2(0.0f, 0.0f), // center-center
            vec2(offset, 0.0f), // center-right
            vec2(-offset, -offset), // bottom-left
            vec2(0.0f, -offset), // bottom-center
            vec2(offset, -offset) // bottom-right
        );

    float kernel[9] = float[](
            1, 1, 1,
            1, -8, 1,
            1, 1, 1
        );

    vec3 sampleTex[9];
    for (int i = 0; i < 9; i++)
    {
        sampleTex[i] = vec3(texture(screenTexture, TexCoords.st + offsets[i]));
    }
    vec3 col = vec3(0.0);
    for (int i = 0; i < 9; i++)
        col += sampleTex[i] * kernel[i];

    FragColor = vec4(col, 1.0);
}

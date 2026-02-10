#type vertex
#version 460 core

layout(location = 0) in vec3 aPos;
// layout(location = 1) in vec3 aNormal;
layout(location = 1) in vec2 aTexCoords;

out SurfaceData {
    vec3 FragPos;
    // vec3 Normal;
    vec2 TexCoords;
} surface;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

void main() {
    surface.FragPos = aPos;
    // surface.Normal = aNormal;
    surface.TexCoords = aTexCoords;
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}

#type fragment
#version 460 core

out vec4 FragColor;

in SurfaceData {
    vec3 FragPos;
    // vec3 Normal;
    vec2 TexCoords;
} surface;

layout(binding = 0) uniform sampler2D surfaceTexture;
layout(binding = 1) uniform sampler2D normalMap;
uniform vec3 viewPos;
uniform vec3 lightPos;

void main() {
    vec3 color = texture(surfaceTexture, surface.TexCoords).rgb;
    vec3 normal = texture(normalMap, surface.TexCoords).rgb;
    normal = normalize(normal * 2.0 - 1.0); // Transform from [0,1] to [-1,1]

    // Ambient
    vec3 ambient = 0.5 * color;

    // Diffuse
    vec3 lightDir = normalize(lightPos - surface.FragPos);
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = diff * color;

    // Specular
    vec3 viewDir = normalize(viewPos - surface.FragPos);
    float spec = 0.0;
    vec3 halfWayDir = normalize(lightDir + viewDir);
    spec = pow(max(dot(normal, halfWayDir), 0.0), 32.0);

    vec3 specular = spec * vec3(0.3); // Assuming white specular highlights

    FragColor = vec4(ambient + diffuse + specular, 1.0);
}

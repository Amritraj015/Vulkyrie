#type vertex
#version 460 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoords;

out SurfaceData {
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoords;
} surface;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

void main() {
    surface.FragPos = aPos;
    surface.Normal = aNormal;
    surface.TexCoords = aTexCoords;
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}

#type fragment
#version 460 core

out vec4 FragColor;

in SurfaceData {
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoords;
} surface;

uniform sampler2D surfaceTexture;
uniform vec3 viewPos;
uniform vec3 lightPos;
uniform bool useBlinnPhong;

void main() {
    vec3 color = texture(surfaceTexture, surface.TexCoords).rgb;

    // Ambient
    vec3 ambient = 0.5 * color;

    // Diffuse
    vec3 lightDir = normalize(lightPos - surface.FragPos);
    vec3 normal = normalize(surface.Normal);
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = diff * color;

    // Specular
    vec3 viewDir = normalize(viewPos - surface.FragPos);
    float spec = 0.0;

    if (useBlinnPhong) {
        vec3 halfWayDir = normalize(lightDir + viewDir);
        spec = pow(max(dot(normal, halfWayDir), 0.0), 32.0);
    } else {
        vec3 reflectDir = reflect(-lightDir, normal);
        spec = pow(max(dot(viewDir, reflectDir), 0.0), 8.0);
    }

    vec3 specular = spec * vec3(0.3); // Assuming white specular highlights

    FragColor = vec4(ambient + diffuse + specular, 1.0);
}

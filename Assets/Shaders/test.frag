#version 450    // Vrersion 4.5

layout(location = 0) in vec3 fragColor;     // Interpoated color from vertex (Location must match).
layout(location = 0) out vec4 outColor;     // Final output color (Must also have location).

void main() {
    outColor = vec4(fragColor, 1.0);
}
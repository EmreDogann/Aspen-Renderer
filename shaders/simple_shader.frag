#version 450

// layout(location = 0) in vec3 fragColor;

// Outputting to location 0.
// out = variable to be used as an output.
// vec4 = variable type.
// outColor = variable name.
layout (location = 0) out vec4 outColor;

layout(push_constant) uniform Push {
    mat2 transform;
    vec2 offset;
    vec3 color;
} push;

void main() {
    outColor = vec4(push.color, 1.0); // RGBA
}
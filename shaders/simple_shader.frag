#version 450

// Outputting to location 0.
// out = variable to be used as an output.
// vec4 = variable type.
// outColor = variable name.
layout (location = 0) out vec4 outColor;

void main() {
    outColor = vec4(1.0, 1.0, 0.0, 1.0); // RGBA
}
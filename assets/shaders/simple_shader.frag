#version 450

// Outputting to location 0.
// in = input data from the location into a variable
// out = variable to be used as an output.
// vec4 = variable type.
// outColor = variable name.
layout (location = 0) in vec3 fragColor;
layout (location = 0) out vec4 outColor;

layout(push_constant) uniform Push {
    mat4 modelMatrix; // projection * view * matrix
    mat4 normalMatrix;
} push;

void main() {
    outColor = vec4(fragColor, 1.0); // RGBA
}
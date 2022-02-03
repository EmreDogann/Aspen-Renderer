#version 450 // GLSL version 4.50

layout(location = 0) in vec2 position;
layout(location = 1) in vec3 color;

// layout(location = 0) out vec3 fragColor;

layout(push_constant) uniform Push {
    mat2 transform;
    vec2 offset;
    vec3 color;
} push;

// gl_Positions is the default output variable.
// gl_VertexIndex contains the current vertex index for everytime the main() function is executed.
void main() {
    gl_Position = vec4(push.transform * position + push.offset, 0.0, 1.0);    // x, y, z, w
    // fragColor = color;
}
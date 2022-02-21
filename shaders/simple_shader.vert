#version 450 // GLSL version 4.50

// Input
layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;

// Output
layout(location = 0) out vec3 fragColor;

// Push Constants
layout(push_constant) uniform Push {
    mat4 transform;
    vec3 color;
} push;

// gl_Positions is the default output variable.
// gl_VertexIndex contains the current vertex index for everytime the main() function is executed.
void main() {
    // Note: The last component for the homogenous coordinate determines if this is a position or a direction.
    // w = 1 refers to a position.
    // w = 0 refers to a direction.
    gl_Position = push.transform * vec4(position, 1.0);
    fragColor = color;
}
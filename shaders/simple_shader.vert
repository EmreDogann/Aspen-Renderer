#version 450 // GLSL version 4.50

vec2 positions[3] = vec2[] (
    vec2(0.0, -0.5),
    vec2(0.5, 0.5),
    vec2(-0.5, 0.5)
);

// gl_Positions is the default output variable.
// gl_VertexIndex contains the current vertex index for everytime the main() function is executed.
void main() {
    gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);    // x, y, z, w
}
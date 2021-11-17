#version 450 // GLSL version 4.50

layout(location = 0) in vec2 position;

// gl_Positions is the default output variable.
// gl_VertexIndex contains the current vertex index for everytime the main() function is executed.
void main() {
    gl_Position = vec4(position, 0.0, 1.0);    // x, y, z, w
}
#version 450 // GLSL version 4.50

// Input
layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec2 uv;

// Output
out gl_PerVertex 
{
    vec4 gl_Position;   
};

// Push Constants
layout(push_constant) uniform Push {
    mat4 MVPMatrix; // projection * view * model matrix
} push;

// gl_Positions is the default output variable.
// gl_VertexIndex contains the current vertex index for everytime the main() function is executed.
void main() {
    // w = 1 refers to a position. w = 0 refers to a direction.
    gl_Position = push.MVPMatrix * vec4(position, 1.0);
}
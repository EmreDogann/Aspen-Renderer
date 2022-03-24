#version 450 // GLSL version 4.50
#extension GL_ARB_gpu_shader_int64 : enable

// Input
layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec2 uv;

// Output
layout(location = 0) out uint64_t outId;

struct PointLight {
  vec4 position;
  vec4 color;  // w is intensity
};

// Uniform Buffer Object (UBO)
layout(set = 0, binding = 0) uniform GlobalUbo {
    mat4 projectionMatrix;
    mat4 viewMatrix;
    mat4 inverseViewMatrix;
    PointLight lights[10];
    vec3 ambientLightColor;
} ubo;

// Push Constants
layout(push_constant) uniform Push {
    mat4 modelMatrix; // projection * view * matrix
    uint64_t objectId;
} push;

invariant gl_Position;

// gl_Positions is the default output variable.
// gl_VertexIndex contains the current vertex index for everytime the main() function is executed.
void main() {
    vec4 worldPosition = push.modelMatrix * vec4(position, 1.0);
    gl_Position = ubo.projectionMatrix * ubo.viewMatrix * worldPosition;
    outId = push.objectId;
}
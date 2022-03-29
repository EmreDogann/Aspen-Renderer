#version 450 // GLSL version 4.50
#extension GL_ARB_separate_shader_objects : enable

// Input
layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec2 uv;

// Output
layout(location = 0) out vec3 outColor;
layout(location = 1) out vec3 outWorldPosition;
layout(location = 2) out vec3 outNormal;
layout(location = 3) out vec2 outUV;
layout(location = 4) out vec3 outLocalPos;
layout(location = 5) out vec3 outLightPos;

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

// Dynamic Uniform Buffer Object
layout(set = 1, binding = 0) uniform DynamicUbo {
    mat4 modelMatrix; // projection * view * matrix
    mat4 normalMatrix;
} dynamicUbo;

invariant gl_Position;

// gl_Positions is the default output variable.
// gl_VertexIndex contains the current vertex index for everytime the main() function is executed.
void main() {
    vec4 worldPosition = dynamicUbo.modelMatrix * vec4(position, 1.0);

    // w = 1 refers to a position. w = 0 refers to a direction.
    gl_Position = ubo.projectionMatrix * ubo.viewMatrix * worldPosition;

    // Transform the vertex normals from local/object space to world space.
    // Here we cast the 4x4 model matrix to a 3x3 matrix, which will just cut off the 4th row and column (the translation component) from the matrix.
    // This is fine in this case because the normal is just a direction and so we do not care about it's position in world space.
    // NOTE: This formal of computing world normals only works if scale is uniform (sx == sy == sz).
    // vec3 normalWorldSpace = normalize(mat3(push.modelMatrix) * normal);

    // The correct way is to use the transpose of the inverse of the model matrix (normal matrix) to convert normals into world space.
    // Calculating the inverse of a matrix per vertex in a shader can be expensive and should be avoided.
    // mat3 normalMatrix = transpose(inverse(mat3(push.modelMatrix)));
    // vec3 normalWorldSpace = normalize(normalMatrix * normal);

    // Instead, we compute the normal matrix on the CPU side.
    outNormal = normalize(mat3(dynamicUbo.normalMatrix) * normal);
    outWorldPosition = worldPosition.xyz;
    outColor = color;
    outUV = uv;
    outLocalPos = position;
    outLightPos = ubo.lights[0].position.xyz;
}
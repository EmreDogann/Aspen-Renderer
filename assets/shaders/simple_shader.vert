#version 450 // GLSL version 4.50

// Input
layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec2 uv;

// Output
layout(location = 0) out vec3 fragColor;

// Uniform Buffer Object (UBO)
layout(set = 0, binding = 0) uniform Ubo {
    mat4 projectionViewMatrix;
    vec3 lightDirection;
} ubo;

// Push Constants
layout(push_constant) uniform Push {
    mat4 modelMatrix; // projection * view * matrix
    mat4 normalMatrix;
} push;

const float AMBIENT  = 0.05;

// gl_Positions is the default output variable.
// gl_VertexIndex contains the current vertex index for everytime the main() function is executed.
void main() {
    // Note: The last component for the homogenous coordinate determines if this is a position or a direction.
    // w = 1 refers to a position. w = 0 refers to a direction.
    gl_Position = ubo.projectionViewMatrix * push.modelMatrix * vec4(position, 1.0);

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
    vec3 normalWorldSpace = normalize(mat3(push.normalMatrix) * normal);

    // If the dot product is negative, that means the normal is facing away from the light. In this case, we just want to light intensity to be 0.
    float lightIntensity = AMBIENT + max(dot(normalWorldSpace, ubo.lightDirection), 0);

    fragColor = lightIntensity * color;
}
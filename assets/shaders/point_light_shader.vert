#version 450

// Outputs
layout(location = 0) out vec2 fragOffset;

// Hard-coded low-poly circles for point lights.
// Two triangles to for a plane.
const vec3 OFFSETS[6] = vec3[](
    vec3(-1.0, -1.0, 0.0), // Top-left: Remember, in Vulkan, the y axis points down.
    vec3(-1.0, 1.0, 0.0), // Bottom-Left
    vec3(1.0, -1.0, 0.0), // Top-Right
    vec3(1.0, -1.0, 0.0), // Top-Right
    vec3(-1.0, 1.0, 0.0), // Bottom-Left
    vec3(1.0, 1.0, 0.0) // Bottom-Right
);

struct PointLight {
    vec4 position; // ignore w
    vec4 color;  // w is intensity
};

// Uniform Buffer Object (UBO)
layout(set = 0, binding = 0) uniform GlobalUbo {
    mat4 projectionMatrix;
    mat4 viewMatrix;
    mat4 inverseProjectionMatrix;
    mat4 inverseViewMatrix;
    PointLight lights[10];
    vec3 ambientLightColor;
} ubo;

layout(push_constant) uniform Push {
    vec4 color;  // w is intensity
    vec3 position;
    float radius;
} push;

void main() {
    vec3 offset = OFFSETS[gl_VertexIndex];
    fragOffset = offset.xy;
    
    // Calculate the position of the triangle vertices in camera/view space.
    vec4 lightInCameraSpace = ubo.viewMatrix * vec4(push.position, 1.0);
    vec4 positionInCameraSpace = lightInCameraSpace + push.radius * vec4(fragOffset, 0.0, 0.0);

    // vec3 cameraRightWorld = {ubo.viewMatrix[0][0], ubo.viewMatrix[1][0], ubo.viewMatrix[2][0]};
    // vec3 cameraUpWorld = {ubo.viewMatrix[0][1], ubo.viewMatrix[1][1], ubo.viewMatrix[2][1]};

    // vec3 positionWorld = push.position
    // + offset.x * push.radius * cameraRightWorld
    // + offset.y * push.radius * cameraUpWorld;

    gl_Position = ubo.projectionMatrix * positionInCameraSpace;
}
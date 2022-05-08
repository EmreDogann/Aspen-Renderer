#version 450

// Input
layout(location = 0) in vec2 fragOffset;

// Output
layout (location = 0) out vec4 outColor;

struct PointLight {
    vec4 position; // ignore w
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

layout(push_constant) uniform Push {
    vec4 color;  // w is intensity
    vec3 position;
    float radius;
} push;

void main() {
    // Discard billboard fragments outside the radius. Will create a circle of size radius.
    float alpha = 1.0 - sqrt(dot(fragOffset, fragOffset)); // Calculate the distance of the fragment from the centre of the billboard.
    if (alpha <= 0.0) {
        discard; // Don't render this fragment.
    }
    outColor = vec4(push.color.xyz * push.color.w, 1);
}
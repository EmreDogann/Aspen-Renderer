#version 450

// Outputting to location 0.
// in = input data from the location into a variable
// out = variable to be used as an output.
// vec4 = variable type.
// outColor = variable name.
layout (location = 0) in vec3 fragColor;
layout (location = 1) in vec3 fragPositionWorld;
layout (location = 2) in vec3 fragNormalWorld;

// Outputs
layout (location = 0) out vec4 outColor;

// Uniform Buffer Object (UBO)
layout(set = 0, binding = 0) uniform Ubo {
    mat4 projectionViewMatrix;
    vec4 ambientLightColor;
    vec4 lightColor;
    vec3 lightPosition;
} ubo;

// Push Constants
layout(push_constant) uniform Push {
    mat4 modelMatrix; // projection * view * matrix
    mat4 normalMatrix;
} push;

void main() {
        vec3 directionToLight = ubo.lightPosition - fragPositionWorld;
    float attenuation = 1.0 / dot(directionToLight, directionToLight); // distance squared

    vec3 lightColor = ubo.lightColor.xyz * ubo.lightColor.w * attenuation;
    vec3 ambientLight = ubo.ambientLightColor.xyz * ubo.ambientLightColor.w;

    // If the dot product is negative, that means the normal is facing away from the light. In this case, we just want to light intensity to be 0.
    // NOTE: the fragNormalWorld input value needs to be normalized in the fragment shader, even if it was already normalized in the vertex shader.
    // This is because when the rasterizer linearly interpolates the fragment normals, they are not necessarily going to be normalized.
    vec3 diffuseLight = lightColor * max(dot(normalize(fragNormalWorld), normalize(directionToLight)), 0);

    outColor = vec4((diffuseLight + ambientLight) * fragColor, 1.0); // RGBA
}
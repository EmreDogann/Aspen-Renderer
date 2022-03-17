#version 450

// Outputting to location 0.
// in = input data from the location into a variable
// out = variable to be used as an output.
// vec4 = variable type.
// outColor = variable name.
layout (location = 0) in vec3 fragColor;
layout (location = 1) in vec3 fragWorldPosition;
layout (location = 2) in vec3 fragNormal;

// Outputs
layout (location = 0) out vec4 outColor;

struct PointLight {
  vec4 position;
  vec4 color;  // w is intensity
};

// Uniform Buffer Object (UBO)
layout(set = 0, binding = 0) uniform GlobalUbo {
    mat4 projectionMatrix;
    mat4 viewMatrix;
    mat4 inverseViewMatrix;
    vec3 ambientLightColor;
    int numLights;
    PointLight lights[10];
} ubo;

// Push Constants
layout(push_constant) uniform Push {
    mat4 modelMatrix; // projection * view * matrix
    mat4 normalMatrix;
} push;

void main() {
    // vec3 directionToLight = ubo.lightPosition - fragPositionWorld;
    // float attenuation = 1.0 / dot(directionToLight, directionToLight); // distance squared

    // vec3 lightColor = ubo.lightColor.xyz * ubo.lightColor.w * attenuation;
    // vec3 ambientLight = ubo.ambientLightColor.xyz * ubo.ambientLightColor.w;

    // // If the dot product is negative, that means the normal is facing away from the light. In this case, we just want to light intensity to be 0.
    // // NOTE: the fragNormalWorld input value needs to be normalized in the fragment shader, even if it was already normalized in the vertex shader.
    // // This is because when the rasterizer linearly interpolates the fragment normals, they are not necessarily going to be normalized.
    // vec3 diffuseLight = ambientLight + lightColor * max(dot(normalize(fragNormalWorld), normalize(directionToLight)), 0);

    // Blinn-Phong Lighting Model
    vec3 diffuseLighting = ubo.ambientLightColor;
    vec3 specularLighting = vec3(0.0);
    vec3 surfaceNormal = normalize(fragNormal);

    vec3 cameraPositionWorld = ubo.inverseViewMatrix[3].xyz;

    // Loop through point lights.
    for (int i = 0; i < ubo.numLights; i++) {
        PointLight light = ubo.lights[i];

        vec3 directionToLight = light.position.xyz - fragWorldPosition;
        float attenuation = light.color.w / dot(directionToLight, directionToLight); // distance squared
        float cosAngIncidence = dot(surfaceNormal, directionToLight);
        cosAngIncidence = clamp(cosAngIncidence, 0, 1);

        // Diffuse lighting
        diffuseLighting += light.color.xyz * attenuation * cosAngIncidence;

        // Specular lighting
        vec3 viewDirection = normalize(cameraPositionWorld - fragWorldPosition);
        vec3 halfAngle = normalize(directionToLight + viewDirection);
        float blinnTerm = dot(surfaceNormal, halfAngle);
        blinnTerm = clamp(blinnTerm, 0, 1);
        blinnTerm = cosAngIncidence != 0.0 ? blinnTerm : 0.0;
        blinnTerm = pow(blinnTerm, 32.0);
        specularLighting += light.color.xyz * attenuation * blinnTerm;
    }

    outColor = vec4((specularLighting + diffuseLighting) * fragColor, 1.0); // RGBA
}
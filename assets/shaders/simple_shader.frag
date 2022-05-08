#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_nonuniform_qualifier : require

// Outputting to location 0.
// in = input data from the location into a variable
// out = variable to be used as an output.
// vec4 = variable type.
// outColor = variable name.
layout(location = 0) in vec3 inColor;
layout(location = 1) in vec3 inWorldPosition;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec2 inUV;
layout(location = 4) in vec3 inLocalPos;
layout(location = 5) in vec3 inLightPos;

// Outputs
layout (location = 0) out vec4 outColor;

// Specialization Constant
layout (constant_id = 0) const int numLights = 10;

struct PointLight {
  vec4 position;
  vec4 color;  // w is intensity
};

// Uniform Buffer Object (UBO)
layout(set = 0, binding = 0) uniform GlobalUbo {
    mat4 projectionMatrix;
    mat4 viewMatrix;
    mat4 inverseViewMatrix;
    PointLight lights[numLights];
    vec3 ambientLightColor;
} ubo;

// Shadows
layout(set = 2, binding = 0) uniform samplerCube samplerShadowCubeMap;

// Textures
layout(set = 3, binding = 0) uniform sampler2D samplerTextures[];

// Push Constants
layout(push_constant) uniform Push {
    int imageIndex;
} push;

// Constants
#define SHADOW_BIAS 0.0001
#define SHADOW_OPACITY 0.5

// From https://stackoverflow.com/questions/10786951/omnidirectional-shadow-mapping-with-depth-cubemap
// Convert the light-to-fragment vector to a depth value so it can be compared with a sampled depth value.
float VectorToDepthValue(vec3 Vec) {
    vec3 AbsVec = abs(Vec);
    float LocalZcomp = max(AbsVec.x, max(AbsVec.y, AbsVec.z));

    const float f = 25.0;
    const float n = 0.01;
    float NormZComp = (f+n) / (f-n) - (2*f*n)/(f-n)/LocalZcomp;
    return (NormZComp + 1.0) * 0.5;
}

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
    vec3 surfaceNormal = normalize(inNormal);

    vec3 cameraPositionWorld = ubo.inverseViewMatrix[3].xyz;

    // Loop through point lights.
    for (int i = 0; i < numLights; i++) {
        PointLight light = ubo.lights[i];

        vec3 directionToLight = light.position.xyz - inWorldPosition;
        float attenuation = light.color.w / dot(directionToLight, directionToLight); // distance squared
        float cosAngIncidence = dot(surfaceNormal, directionToLight);
        cosAngIncidence = clamp(cosAngIncidence, 0, 1);

        // Diffuse lighting
        diffuseLighting += light.color.xyz * attenuation * cosAngIncidence;

        // Specular lighting
        vec3 viewDirection = normalize(cameraPositionWorld - inWorldPosition);
        vec3 halfAngle = normalize(directionToLight + viewDirection);
        float blinnTerm = dot(surfaceNormal, halfAngle);
        blinnTerm = clamp(blinnTerm, 0, 1);
        blinnTerm = cosAngIncidence != 0.0 ? blinnTerm : 0.0;
        blinnTerm = pow(blinnTerm, 32.0);
        specularLighting += light.color.xyz * attenuation * blinnTerm;
    }

    // Shadow
	vec3 lightVec = inWorldPosition - inLightPos;
    float sampledDist = texture(samplerShadowCubeMap, lightVec).x;
    float dist = VectorToDepthValue(inLightPos - inWorldPosition);
    // float dist = length(lightVec);

	// Check if fragment is in shadow
    float bias = max(0.0001 * (1.0 - clamp(dot(surfaceNormal, (inLightPos - inWorldPosition)), 0, 1)), 0.0001);
    float shadow = (sampledDist + bias > dist) ? 1.0 : 0.1;

    outColor = vec4(texture(samplerTextures[push.imageIndex], inUV).xyz * (specularLighting + diffuseLighting) * shadow, 1.0); // RGBA
    // outColor = vec4((specularLighting + diffuseLighting) * inColor * shadow, 1.0); // RGBA
}
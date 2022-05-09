#version 460
#extension GL_EXT_ray_tracing : require

// Ray Payload
struct HitPayload {
  vec3 hitValue;
};
layout(location = 0) rayPayloadEXT HitPayload rPayload;

// Push constant structure for the ray tracer
struct PushConstantRay {
  vec4  clearColor;
  vec3  lightPosition;
  float lightIntensity;
};

layout(push_constant) uniform _PushConstantRay {
  PushConstantRay pcRay;
};

void main() {
    rPayload.hitValue = pcRay.clearColor.rgb;
}
#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_GOOGLE_include_directive : enable

// Includes
#include "raycommon.glsl"

// Ray Payload
layout(location = 0) rayPayloadInEXT HitPayload rPayload;

// Push constant structure for the ray tracer
struct PushConstantRay {
  vec4 clearColor;
  vec3 lightPosition;
  float lightIntensity;
  int textureMapping;
  int shadows;
  float shadowBias;
  float shadowOpacity;
  int useReflections;
  int useRefractions;
  float minRange;
	int totalBounces;
};

layout(push_constant) uniform _PushConstantRay {
  PushConstantRay pcRay;
};

void main() {
    rPayload.hitValue = pcRay.clearColor.rgb;
}
#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_nonuniform_qualifier : enable

#include "material.glsl"
#include "raycommon.glsl"

// Incoming Ray Payload
layout(location = 0) rayPayloadInEXT HitPayload rPayload;

// Ray Hit attributes
hitAttributeEXT vec2 attribs;

// Outgoing Ray Payload
layout(location = 1) rayPayloadEXT bool isShadowed;

layout(binding = 0, set = 1) uniform accelerationStructureEXT topLevelAS;
layout(binding = 2, set = 1) readonly buffer VertexArray { float Vertices[]; };
layout(binding = 3, set = 1) readonly buffer IndexArray { uint Indices[]; };
layout(binding = 4, set = 1) readonly buffer OffsetArray { uvec2[] Offsets; };
layout(binding = 5, set = 1) readonly buffer MaterialArray { Material[] Materials; };

// Textures
layout(set = 2, binding = 0) uniform sampler2D samplerTextures[];

vec2 Mix(vec2 a, vec2 b, vec2 c, vec3 barycentrics) {
	return a * barycentrics.x + b * barycentrics.y + c * barycentrics.z;
};

vec3 Mix(vec3 a, vec3 b, vec3 c, vec3 barycentrics) {
    return a * barycentrics.x + b * barycentrics.y + c * barycentrics.z;
};

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

struct Vertex {
  vec3 Position;
  vec3 Color;
  vec3 Normal;
  vec2 TexCoord;
  uint MaterialIndex;
};

Vertex UnpackVertex(uint index) {
	const uint vertexSize = 12;
	const uint offset = index * vertexSize;
	
	Vertex v;
	
	v.Position = vec3(Vertices[offset + 0], Vertices[offset + 1], Vertices[offset + 2]);
  	v.Color = vec3(Vertices[offset + 3], Vertices[offset + 4], Vertices[offset + 5]);
	v.Normal = vec3(Vertices[offset + 6], Vertices[offset + 7], Vertices[offset + 8]);
	v.TexCoord = vec2(Vertices[offset + 9], Vertices[offset + 10]);
	v.MaterialIndex = floatBitsToInt(Vertices[offset + 11]);

	return v;
};

vec3 computeDiffuse(Material m, vec3 lightDir, vec3 normal) {
  // Lambertian
  float dotNL = max(dot(normal, lightDir), 0.0);
  vec3  c     = m.diffuse.rgb * dotNL;
  vec3 ambient = vec3(0.02);
  return c + ambient;
}

vec3 computeSpecular(Material m, vec3 viewDir, vec3 lightDir, vec3 normal) {
  // Compute specular only if not in shadow
  const float kPi        = 3.14159265;
  const float kShininess = m.specular;

  // Specular
  const float kEnergyConservation = (2.0 + kShininess) / (2.0 * kPi);
  vec3        V                   = normalize(-viewDir);
  vec3        R                   = reflect(-lightDir, normal);
  float       specular            = kEnergyConservation * pow(max(dot(V, R), 0.0), kShininess);

  return vec3(specular);
}

void main() {
  	// Get the material.
	const uvec2 offsets = Offsets[gl_InstanceCustomIndexEXT];
	const uint indexOffset = offsets.x;
	const uint vertexOffset = offsets.y;
	const Vertex v0 = UnpackVertex(vertexOffset + Indices[indexOffset + gl_PrimitiveID * 3 + 0]);
	const Vertex v1 = UnpackVertex(vertexOffset + Indices[indexOffset + gl_PrimitiveID * 3 + 1]);
	const Vertex v2 = UnpackVertex(vertexOffset + Indices[indexOffset + gl_PrimitiveID * 3 + 2]);
	const Material material = Materials[v0.MaterialIndex];

	// Compute the ray hit point properties.
	const vec3 barycentrics = vec3(1.0 - attribs.x - attribs.y, attribs.x, attribs.y);
	const vec3 position = Mix(v0.Position, v1.Position, v2.Position, barycentrics);
	const vec3 color = Mix(v0.Color, v1.Color, v2.Color, barycentrics);
	const vec3 normal = Mix(v0.Normal, v1.Normal, v2.Normal, barycentrics);
	const vec2 texCoord = Mix(v0.TexCoord, v1.TexCoord, v2.TexCoord, barycentrics);

	// Computing the coordinates of the hit position
	const vec3 worldPos = vec3(gl_ObjectToWorldEXT * vec4(position, 1.0));  // Transforming the position to world space
	// Computing the normal at hit position
	const vec3 worldNormal = normalize(vec3(normal * gl_WorldToObjectEXT));  // Transforming the normal to world space

	// Vector toward the light
	vec3  L;
	float lightIntensity = pcRay.lightIntensity;
	float lightDistance  = 10000.0;

	vec3 lDir      = pcRay.lightPosition - worldPos;
	lightDistance  = length(lDir);
	lightIntensity = pcRay.lightIntensity / (lightDistance * lightDistance);
	L              = normalize(lDir);

	// ScatterPayload sPayload = Scatter(material, lDir, worldNormal, texCoord, gl_HitTEXT, rPayload.randomSeed);
	// rPayload.hitValue = sPayload.ColorAndDistance.rgb * (material.diffuseTextureId == -1 ? color : vec3(1.0));

	vec3 diffuse = computeDiffuse(material, L, worldNormal);

	vec3  specular    = vec3(0);
  	float attenuation = 1;

	// Tracing shadow ray only if the light is visible from the surface
	if(pcRay.shadows == 1 && dot(worldNormal, L) > 0) {
		float tMin   = pcRay.shadowBias;
		float tMax   = lightDistance;
		vec3  origin = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT;
		vec3  rayDir = L;
		// gl_RayFlagsSkipClosestHitShaderEXT: Will not invoke the hit shader, only the miss shader
		// gl_RayFlagsOpaqueEXT : Will not call the any hit shader, so all objects will be opaque
		// gl_RayFlagsTerminateOnFirstHitEXT : The first hit is always good.
		uint  flags = gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsOpaqueEXT | gl_RayFlagsSkipClosestHitShaderEXT;

		isShadowed = true;

		traceRayEXT(topLevelAS,  // acceleration structure
				flags,       // rayFlags
				0xFF,        // cullMask
				0,           // sbtRecordOffset
				0,           // sbtRecordStride
				1,           // missIndex
				origin,      // ray origin
				tMin,        // ray min range
				rayDir,      // ray direction
				tMax,        // ray max range
				1            // payload (location = 1)
		);

		if(isShadowed) {
			attenuation = pcRay.shadowOpacity;
		} else {
			// Specular
			specular = computeSpecular(material, gl_WorldRayDirectionEXT, L, worldNormal);
		}
	}

	// Reflection
	if(pcRay.useReflections == 1 && material.materialModel == MaterialMetallic) {
		vec3 origin = worldPos;
		vec3 rayDir = reflect(gl_WorldRayDirectionEXT, worldNormal);
		rPayload.attenuation *= attenuation;
		rPayload.done      = 0;
		rPayload.rayOrigin = origin;
		rPayload.rayDir    = rayDir;
	} else if (pcRay.useRefractions == 1 && material.materialModel == MaterialDielectric) { // Refractions
		const float dot = dot(gl_WorldRayDirectionEXT, worldNormal);
		const vec3 outwardNormal = dot > 0 ? -worldNormal : worldNormal;
		const float niOverNt = dot > 0 ? material.refractionIndex : 1 / material.refractionIndex;
		const float cosine = dot > 0 ? material.refractionIndex * dot : -dot;

		const vec3 refracted = refract(gl_WorldRayDirectionEXT, outwardNormal, niOverNt);
		rPayload.attenuation *= attenuation;
		rPayload.done      = 0;
		rPayload.rayOrigin = worldPos;
		rPayload.rayDir    = refracted;
	}

	rPayload.hitValue = lightIntensity * attenuation * (diffuse + specular);

	if (pcRay.textureMapping == 1) {
		rPayload.hitValue *= (material.diffuseTextureId >= 0 ? texture(samplerTextures[nonuniformEXT(material.diffuseTextureId)], texCoord).rgb : color);
	} else {
		rPayload.hitValue *= color;
	}
}

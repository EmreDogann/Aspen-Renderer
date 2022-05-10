#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : enable

// Ray Payload
struct HitPayload {
  vec3 hitValue;
};
layout(location = 0) rayPayloadInEXT HitPayload rPayload;
hitAttributeEXT vec2 attribs;

struct Vertex {
  vec3 Position;
  vec3 Color;
  vec3 Normal;
  vec2 TexCoord;
  int TextureIndex;
};

layout(binding = 2, set = 1) readonly buffer VertexArray { float Vertices[]; };
layout(binding = 3, set = 1) readonly buffer IndexArray { uint Indices[]; };
layout(binding = 4, set = 1) readonly buffer OffsetArray { uvec2[] Offsets; };
layout(binding = 5, set = 1) readonly buffer TextureIDArray { int[] TextureIDs; };

// Textures
layout(set = 2, binding = 0) uniform sampler2D samplerTextures[];

vec2 Mix(vec2 a, vec2 b, vec2 c, vec3 barycentrics) {
	return a * barycentrics.x + b * barycentrics.y + c * barycentrics.z;
}

vec3 Mix(vec3 a, vec3 b, vec3 c, vec3 barycentrics) {
    return a * barycentrics.x + b * barycentrics.y + c * barycentrics.z;
}

// Push constant structure for the ray tracer
struct PushConstantRay {
  vec4  clearColor;
  vec3  lightPosition;
  float lightIntensity;
};

layout(push_constant) uniform _PushConstantRay {
  PushConstantRay pcRay;
};

Vertex UnpackVertex(uint index) {
	const uint vertexSize = 12;
	const uint offset = index * vertexSize;
	
	Vertex v;
	
	v.Position = vec3(Vertices[offset + 0], Vertices[offset + 1], Vertices[offset + 2]);
  	v.Color = vec3(Vertices[offset + 3], Vertices[offset + 4], Vertices[offset + 5]);
	v.Normal = vec3(Vertices[offset + 6], Vertices[offset + 7], Vertices[offset + 8]);
	v.TexCoord = vec2(Vertices[offset + 9], Vertices[offset + 10]);
	v.TextureIndex = floatBitsToInt(Vertices[offset + 11]);

	return v;
};

vec3 computeDiffuse(vec3 lightDir, vec3 normal) {
  // Lambertian
  float dotNL = max(dot(normal, lightDir), 0.0);
  vec3  c     = vec3(0.8, 0.8, 0.8) * dotNL;
  vec3 ambient = vec3(0.2);
//   if(mat.illum >= 1) {
//     c += mat.ambient;
//   }
  return c + ambient;
}

vec3 computeSpecular(vec3 viewDir, vec3 lightDir, vec3 normal) {
  // Compute specular only if not in shadow
  const float kPi        = 3.14159265;
  const float kShininess = 2.0;

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
	const int textureID = TextureIDs[v0.TextureIndex];

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
	float lightDistance  = 100000.0;

	vec3 lDir      = pcRay.lightPosition - worldPos;
	lightDistance  = length(lDir);
	lightIntensity = pcRay.lightIntensity / (lightDistance * lightDistance);
	L              = normalize(lDir);

	vec3 diffuse = computeDiffuse(L, worldNormal);
	vec3 specular = computeSpecular(gl_WorldRayDirectionEXT, L, worldNormal);

	if (textureID >= 0) {
		rPayload.hitValue = (diffuse + specular) * texture(samplerTextures[textureID], texCoord).rgb;
	} else {
		rPayload.hitValue = color * (diffuse + specular);
	}
	// rPayload.hitValue = (diffuse + specular);

	// rPayload.hitValue = texture(samplerTextures[nonuniformEXT(int(offsets.z))], texCoord).xyz;

	// rPayload.hitValue = color;
	// rPayload.hitValue = vec3(texCoord, 1.0);

  	// rPayload.hitValue = vec3(0.0, 0.0, 0.0);
}

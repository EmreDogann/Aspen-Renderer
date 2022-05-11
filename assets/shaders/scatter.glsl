// From: https://github.com/GPSnoopy/RayTracingInVulkan/blob/master/assets/shaders/Scatter.glsl

#extension GL_EXT_nonuniform_qualifier : require
#extension GL_GOOGLE_include_directive : enable

#include "random.glsl"

// Polynomial approximation by Christophe Schlick
float Schlick(const float cosine, const float refractionIndex) {
	float r0 = (1 - refractionIndex) / (1 + refractionIndex);
	r0 *= r0;
	return r0 + (1 - r0) * pow(1 - cosine, 5);
}

// Lambertian
ScatterPayload ScatterLambertian(const Material m, const vec3 direction, const vec3 normal, const vec2 texCoord, const float t, inout uint seed) {
	const bool isScattered = dot(direction, normal) < 0;
	const vec4 texColor = m.diffuseTextureId >= 0 ? texture(samplerTextures[nonuniformEXT(m.diffuseTextureId)], texCoord) : vec4(1);
	const vec4 colorAndDistance = vec4(m.diffuse.rgb * texColor.rgb, t);
	const vec4 scatter = vec4(normal + RandomInUnitSphere(seed), isScattered ? 1 : 0);

	return ScatterPayload(colorAndDistance, scatter);
}

// Metallic
ScatterPayload ScatterMetallic(const Material m, const vec3 direction, const vec3 normal, const vec2 texCoord, const float t, inout uint seed) {
	const vec3 reflected = reflect(direction, normal);
	const bool isScattered = dot(reflected, normal) > 0;

	const vec4 texColor = m.diffuseTextureId >= 0 ? texture(samplerTextures[nonuniformEXT(m.diffuseTextureId)], texCoord) : vec4(1);
	const vec4 colorAndDistance = vec4(m.diffuse.rgb * texColor.rgb, t);
	const vec4 scatter = vec4(reflected + m.fuzziness*RandomInUnitSphere(seed), isScattered ? 1 : 0);

	return ScatterPayload(colorAndDistance, scatter);
}

// Dielectric
ScatterPayload ScatterDieletric(const Material m, const vec3 direction, const vec3 normal, const vec2 texCoord, const float t, inout uint seed) {
	const float dot = dot(direction, normal);
	const vec3 outwardNormal = dot > 0 ? -normal : normal;
	const float niOverNt = dot > 0 ? m.refractionIndex : 1 / m.refractionIndex;
	const float cosine = dot > 0 ? m.refractionIndex * dot : -dot;

	const vec3 refracted = refract(direction, outwardNormal, niOverNt);
	const float reflectProb = refracted != vec3(0) ? Schlick(cosine, m.refractionIndex) : 1;

	const vec4 texColor = m.diffuseTextureId >= 0 ? texture(samplerTextures[nonuniformEXT(m.diffuseTextureId)], texCoord) : vec4(1);
	
	return RandomFloat(seed) < reflectProb
		? ScatterPayload(vec4(texColor.rgb, t), vec4(reflect(direction, normal), 1))
		: ScatterPayload(vec4(texColor.rgb, t), vec4(refracted, 1));
}

// Diffuse Light
ScatterPayload ScatterDiffuseLight(const Material m, const float t, inout uint seed) {
	const vec4 colorAndDistance = vec4(m.diffuse.rgb, t);
	const vec4 scatter = vec4(1, 0, 0, 0);

	return ScatterPayload(colorAndDistance, scatter);
}

ScatterPayload Scatter(const Material m, const vec3 direction, const vec3 normal, const vec2 texCoord, const float t, inout uint seed) {
	const vec3 normDirection = normalize(direction);

	switch (m.materialModel) {
		case MaterialLambertian:
			return ScatterLambertian(m, normDirection, normal, texCoord, t, seed);
		case MaterialMetallic:
			return ScatterMetallic(m, normDirection, normal, texCoord, t, seed);
		case MaterialDielectric:
			return ScatterDieletric(m, normDirection, normal, texCoord, t, seed);
		case MaterialDiffuseLight:
			return ScatterDiffuseLight(m, t, seed);
	}
}
// From: https://github.com/GPSnoopy/RayTracingInVulkan/blob/master/assets/shaders/Material.glsl

const uint MaterialLambertian = 0;
const uint MaterialMetallic = 1;
const uint MaterialDielectric = 2;
const uint MaterialIsotropic = 3;
const uint MaterialDiffuseLight = 4;

struct Material {
	vec4 diffuse;
	int diffuseTextureId;
	float fuzziness;
	float specular;
	float refractionIndex;
	uint materialModel;
};
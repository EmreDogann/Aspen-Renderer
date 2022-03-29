#version 450 // GLSL version 4.50
#extension GL_EXT_multiview : enable
// #extension GL_EXT_debug_printf : enable

// Input
layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec2 uv;

layout(location = 0) out vec4 outPos;
layout(location = 1) out vec3 outLightPos;

// Uniform Buffer Object
layout(set = 0, binding = 0) uniform LightUbo {
    mat4 projectionMatrix;
    mat4 viewMatries[6];
} lightUbo;


// Dynamic Uniform Buffer Object
layout(set = 1, binding = 0) uniform DynamicUbo {
    mat4 modelMatrix; // projection * view * matrix
    mat4 normalMatrix;
} dynamicUbo;

// Push Constants
layout(push_constant) uniform Push {
    vec4 lightPos;
} push;

out gl_PerVertex 
{
	vec4 gl_Position;
};

// gl_Positions is the default output variable.
// gl_VertexIndex contains the current vertex index for everytime the main() function is executed.
void main() {
    // debugPrintfEXT("The ViewIndex is %i\n", gl_ViewIndex);
    // vec3 flippedPosition = vec3(-position.x, position.y, position.z);
    gl_Position = lightUbo.projectionMatrix * lightUbo.viewMatries[gl_ViewIndex] * dynamicUbo.modelMatrix * vec4(position, 1.0);
    outPos = vec4(position, 1.0);
    outLightPos = push.lightPos.xyz;
}
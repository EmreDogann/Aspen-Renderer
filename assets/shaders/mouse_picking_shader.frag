#version 450
#extension GL_ARB_gpu_shader_int64 : enable

// Outputting to location 0.
// in = input data from the location into a variable
// out = variable to be used as an output.
// vec4 = variable type.
// outColor = variable name.
// flat = This value will not be interpolated.
layout (location = 0) flat in uint64_t inId;

// Output
layout (location = 0) out vec4 outColor;

// Shader Storage Buffer Object (SSBO)
layout(std140, set = 1, binding = 0) buffer MousePickingStorageBuffer {
    int64_t objectId;
} ssbo;

void main() {
    ssbo.objectId = int64_t(inId);
    outColor = vec4(inId, inId, inId, 1.0);
}
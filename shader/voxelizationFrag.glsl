#version 430 core

in vec3 fragPos;
in vec3 projectPos;
in vec4 box;

layout (std430, binding = 0) buffer CntBuffer
{
    uint cnt[];
};

uniform float step;
uniform vec3 boxMin;
uniform vec3 resolution;

out vec4 fragColor;

void main()
{
    if (projectPos.x < box.x || projectPos.y < box.y || projectPos.x > box.z || projectPos.y > box.w)
        discard;

    uint x = uint((fragPos.x - boxMin.x) / step);
    uint y = uint((fragPos.y - boxMin.y) / step);
    uint z = uint((fragPos.z - boxMin.z) / step);
    uint index = uint(y * (resolution.z * resolution.x) + z * resolution.x + x);
    atomicAdd(cnt[index], 1);
    fragColor = vec4(0.0, 1.0, 0.0, 1.0);
}
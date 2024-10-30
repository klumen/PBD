#version 430 core

layout (local_size_x = 1024, local_size_y = 1, local_size_z = 1) in;

struct Particle
{
    vec3 force;
    float mass;
    vec3 velocity;
    float massInv;
    vec3 position;
    uint phase;
    vec3 predict;
    float contactMass;
    vec3 corr;
    float height;
};
layout (std430, binding = 0) buffer particleBuffer
{
    Particle particle[];
};

layout (std430, binding = 1) buffer tableBuffer
{
    uvec2 table[];
};

layout (std430, binding = 2) buffer orderBuffer
{
    uvec2 order[];
};

layout (std430, binding = 3) buffer neighborBuffer
{
    uint neighbor[];
};

layout (std430, binding = 4) buffer countBuffer
{
    uint count[];
};

layout (std430, binding = 7) buffer omegaBuffer
{
    vec4 omega[];
};

layout (std140, binding = 0) uniform neighborUBO
{
    float cellSize;
    uint tableSize;
    uint maxNeighbors;
};

uniform uint head;
uniform uint particleNum;
uniform float h;
uniform float vorticity;
uniform float deltaTime;

vec3 spiky(vec3 r)
{
    float r_norm = length(r);

    if (r_norm < h && r_norm > 0.0)
        return -45.0 / (acos(-1.0) * pow(h, 6.0)) * pow(h - r_norm, 2.0) * r / r_norm;
    return vec3(0.0, 0.0, 0.0);
}

void main()
{
    uint i = gl_GlobalInvocationID.x;
    uint pi = head + i;
    if (i >= particleNum) return;

    vec3 eta = vec3(0.0);
    for (uint j = 0; j < count[pi]; ++j)
    {
        const uint ni = neighbor[pi * maxNeighbors + j];
        if (particle[ni].phase == 3)
        {
            vec3 grad = spiky(particle[pi].predict - particle[ni].predict);
            eta += omega[ni - head].w * grad;
        }
    }

    if (length(eta) == 0.0)
        return;

    vec3 N = normalize(eta);
    vec3 f = vorticity * cross(N, omega[i].xyz);
    particle[pi].velocity += f * deltaTime;
}
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

layout (std430, binding = 6) buffer lambdaBuffer
{
    float lambda[];
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
uniform float rho0;

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

    particle[pi].corr = vec3(0.0);
    for (uint j = 0; j < count[pi]; ++j)
    {
        const uint ni = neighbor[pi * maxNeighbors + j];
        if (particle[ni].phase == 3)
        {
            const vec3 gradC_j = -particle[ni].mass / rho0 * spiky(particle[pi].predict - particle[ni].predict);
            particle[pi].corr -= (lambda[i] + lambda[ni - head]) * gradC_j;
        }
        else
        {
            const vec3 gradC_j = -particle[ni].mass / rho0 * spiky(particle[pi].predict - particle[ni].predict);
            particle[pi].corr -= lambda[i] * gradC_j;
        }
    }
}
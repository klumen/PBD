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

layout (std430, binding = 5) buffer rhoBuffer
{
    float rho[];
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
uniform float viscosity;

float poly6(vec3 r)
{
     float r_norm = length(r);

    if (r_norm < h)
        return 315.0 / (64.0 * acos(-1.0) * pow(h, 9.0)) * pow(h * h - r_norm * r_norm, 3.0);
    return 0.0;
}

void main()
{
    uint i = gl_GlobalInvocationID.x;
    uint pi = head + i;
    if (i >= particleNum) return;

    for (uint j = 0; j < count[pi]; ++j)
    {
        const uint ni = neighbor[pi * maxNeighbors + j];
        if (particle[ni].phase == 3)
        {
            vec3 deltaVelocity = particle[ni].velocity - particle[pi].velocity;
            particle[pi].velocity += viscosity * (particle[ni].mass / rho[ni - head]) * deltaVelocity * poly6(particle[pi].predict - particle[ni].predict);
        }
        else
        {
            particle[pi].velocity -= viscosity * (particle[ni].mass / rho[i]) * particle[pi].velocity * poly6(particle[pi].predict - particle[ni].predict);
        }
    }
}
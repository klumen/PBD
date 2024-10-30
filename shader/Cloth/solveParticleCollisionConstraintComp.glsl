#version 430 core
#extension GL_NV_shader_atomic_float : enable

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

layout (std430, binding = 3) buffer neighborBuffer
{
    uint neighbor[];
};

layout (std430, binding = 4) buffer countBuffer
{
    uint count[];
};

layout (std140, binding = 0) uniform neighborUBO
{
    float cellSize;
    uint tableSize;
    uint maxNeighbors;
};

layout (std430, binding = 8) buffer nBuffer
{
    uint n[];
};

uniform uint head;
uniform uint particleNum;
uniform float radius;
uniform float mu_s;
uniform float mu_k;

void main()
{
    const uint i = gl_GlobalInvocationID.x;
    const uint pi = i + head;
    if (i >= particleNum) return;
    n[i] = 1;

    for (uint j = 0; j < count[pi]; ++j)
    {
        const uint pj = neighbor[pi * maxNeighbors + j];

        vec3 xij = particle[pj].predict - particle[pi].predict;
        float dis = radius - length(xij);
        if (dis <= 0.0) continue;

        if (particle[pi].massInv + particle[pj].massInv == 0.0)
            continue;

        vec3 nij = normalize(xij);
        vec3 corr = (dis * nij) / (particle[pi].massInv + particle[pj].massInv);
        vec3 corri = particle[pi].massInv * corr;
        vec3 corrj = particle[pj].massInv * corr;
        
        atomicAdd(particle[pi].corr.x, -corri.x);
        atomicAdd(particle[pi].corr.y, -corri.y);
        atomicAdd(particle[pi].corr.z, -corri.z);
        atomicAdd(particle[pj].corr.x, corrj.x);
        atomicAdd(particle[pj].corr.y, corrj.y);
        atomicAdd(particle[pj].corr.z, corrj.z);
        // particle[pi].corr -= particle[pi].massInv * corr;
        // particle[pj].corr += particle[pj].massInv * corr;

        vec3 deltaPosition = (particle[pi].predict + particle[pi].corr - particle[pi].position) - (particle[pj].predict + particle[pj].corr - particle[pj].position);
        vec3 deltaPosition_t = deltaPosition - dot(deltaPosition, nij) * nij;
        float deltaLengh = length(deltaPosition_t);
        if (deltaLengh < mu_s * dis)
        {
            deltaPosition = deltaPosition_t;
        }
        else
        {
            deltaPosition = deltaPosition_t * min(1.0, mu_k * dis / deltaLengh);
        }
        
        corr = deltaPosition / (particle[pi].massInv + particle[pj].massInv);
        corri = particle[pi].massInv * corr;
        corrj = particle[pj].massInv * corr;

        atomicAdd(particle[pi].corr.x, -corri.x);
        atomicAdd(particle[pi].corr.y, -corri.y);
        atomicAdd(particle[pi].corr.z, -corri.z);
        atomicAdd(particle[pj].corr.x, corrj.x);
        atomicAdd(particle[pj].corr.y, corrj.y);
        atomicAdd(particle[pj].corr.z, corrj.z);
        // particle[pi].corr -= particle[pi].massInv * corr;
        // particle[pj].corr += particle[pj].massInv * corr;
    }
}
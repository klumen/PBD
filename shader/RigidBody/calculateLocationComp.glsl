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
layout (std430, binding = 6) buffer locationSumBuffer
{
    vec4 locationSum[];
};

uniform uint head;
uniform uint particleNum;

shared vec3 localLocationSum[1024];

void main()
{
    uint i = gl_GlobalInvocationID.x;
    uint particleId = i + head;
    uint localId = gl_LocalInvocationID.x;
    if (i >= particleNum)
    {
        localLocationSum[localId] = vec3(0.0);
    }
    else
    {
        particle[particleId].predict += particle[particleId].corr;
        particle[particleId].corr = vec3(0.0);
        localLocationSum[localId] = particle[particleId].mass * particle[particleId].predict;
    }
    barrier();

    for (uint stride = 1; stride < 1024; stride *= 2)
    {
        if (localId % (2 * stride) == 0)
        {
            localLocationSum[localId] += localLocationSum[localId + stride];
        }
        barrier();
    }

    if (localId == 0)
        locationSum[gl_WorkGroupID.x] = vec4(localLocationSum[0], 0);
}
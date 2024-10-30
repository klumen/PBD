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
layout (std430, binding = 7) buffer rBuffer
{
    vec4 r[];
};
layout (std430, binding = 8) buffer rotationSumBuffer
{
    mat4 rotationSum[];
};

uniform uint head;
uniform uint particleNum;
uniform vec3 location;

shared mat3 localRotationSum[1024];

void main()
{
    uint i = gl_GlobalInvocationID.x;
    uint particleId = i + head;
    uint localId = gl_LocalInvocationID.x;
    if (i >= particleNum)
    {
        localRotationSum[localId] = mat3(0.0);
    }
    else
    {
        localRotationSum[localId][0][0] = particle[particleId].mass * (particle[particleId].predict - location)[0] * r[i][0];
        localRotationSum[localId][0][1] = particle[particleId].mass * (particle[particleId].predict - location)[0] * r[i][1];
        localRotationSum[localId][0][2] = particle[particleId].mass * (particle[particleId].predict - location)[0] * r[i][2];
        localRotationSum[localId][1][0] = particle[particleId].mass * (particle[particleId].predict - location)[1] * r[i][0];
        localRotationSum[localId][1][1] = particle[particleId].mass * (particle[particleId].predict - location)[1] * r[i][1];
        localRotationSum[localId][1][2] = particle[particleId].mass * (particle[particleId].predict - location)[1] * r[i][2];
        localRotationSum[localId][2][0] = particle[particleId].mass * (particle[particleId].predict - location)[2] * r[i][0];
        localRotationSum[localId][2][1] = particle[particleId].mass * (particle[particleId].predict - location)[2] * r[i][1];
        localRotationSum[localId][2][2] = particle[particleId].mass * (particle[particleId].predict - location)[2] * r[i][2];
    }
    barrier();

    for (uint stride = 1; stride < 1024; stride *= 2)
    {
        if (localId % (2 * stride) == 0)
        {
            localRotationSum[localId] += localRotationSum[localId + stride];
        }
        barrier();
    }

    if (localId == 0)
        rotationSum[gl_WorkGroupID.x] = mat4(localRotationSum[0]);
}
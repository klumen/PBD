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

struct Attach
{
    vec3 position;
    uint particleID;
};
layout (std430, binding = 7) buffer attachSSBO
{
    Attach attach[];
};

layout (std430, binding = 8) buffer nBuffer
{
    uint n[];
};

layout (std430, binding = 9) buffer rBuffer
{
    float r[];
};

uniform uint head;
uniform uint particleNum;
uniform uint attachNum;

void main()
{
    uint i = gl_GlobalInvocationID.x;
    if (i >= particleNum) return;
    const uint pi = head + i;

    n[i] = 0;
    for (uint j = 0; j < attachNum; j++)
    {
        if (i == attach[j].particleID)
        {
            particle[pi].predict = attach[j].position;
            return;
        }
    }

    for (uint j = 0; j < attachNum; j++)
    {
        vec3 dir = attach[j].position - particle[pi].predict;
        const float dis = length(dir) - r[j * particleNum + i];
        if (dis <= 0.0) continue;

        particle[pi].corr += dis * normalize(dir);
        n[i] += 1;
    }
}
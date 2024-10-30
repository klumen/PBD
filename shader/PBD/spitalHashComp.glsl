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

layout (std140, binding = 0) uniform neighborUBO
{
    float cellSize;
    uint tableSize;
    uint maxNeighbors;
};

uniform uint particleNum;

const int p1 = 73856093;
const int p2 = 19349663;
const int p3 = 83492791;

uint hash(ivec3 pos)
{
    return uint((p1 * pos.x) ^ (p2 * pos.y) ^ (p3 * pos.z)) % tableSize;
}

void main()
{
    uint i = gl_GlobalInvocationID.x;
    if (i < particleNum)
    {
        order[i] = uvec2(0);
        order[i + particleNum] = uvec2(0);

        ivec3 cellPos = ivec3(floor(particle[i].predict / cellSize));
        table[i][0] = hash(cellPos);
        table[i][1] = i;
    }
    else
    {
        table[i] = uvec2(-1);
    }
}
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

layout (std140, binding = 0) uniform neighborUBO
{
    float cellSize;
    uint tableSize;
    uint maxNeighbors;
};

uniform uint head;
uniform uint particleNum;
uniform float neighborRadius;

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
    uint particleId = head + i;
    if (i >= particleNum) return;

    ivec3 initCellPos = ivec3(floor(particle[particleId].predict / cellSize));
    uint neighborNum = 0;

    uint has[27];
    uint now = 0;

    int x, y, z;
    for (x = -1; x <= 1; ++x)
    {
        for (y = -1; y <= 1; ++y)
        {
            for (z = -1; z <= 1; ++z)
            {
                ivec3 cellPos = initCellPos + ivec3(x, y, z);
                uint cellHash = hash(cellPos);

                uint j;
                bool found = false;
                for (j = 0; j < now; j++)
                {
                    if (has[j] == cellHash)
                    {
                        found = true;
                        break;
                    }
                }
                if (found) continue;

                has[now] = cellHash;
                ++now;

                uint s = order[cellHash][0];
                uint e = order[cellHash][1];
                for (j = s; j < e; ++j)
                {
                    uint pi = table[j][1];
                    if (pi >= head && pi < head + particleNum)
                        continue;

                    if (pi != particleId)
                    {
                        vec3 d = particle[particleId].predict - particle[pi].predict;
                        float dist2 = dot(d, d);
                        if (dist2 <= 1.1 * neighborRadius * neighborRadius)
                        {
                            uint neighborIndex = particleId * maxNeighbors + neighborNum;
                            neighbor[neighborIndex] = pi;
                            ++neighborNum;
                        }
                    } 

                    if (maxNeighbors == neighborNum)
                    {
                        count[particleId] = neighborNum;
                        return;
                    }
                }
            }
        }
    }
    count[particleId] = neighborNum;
}
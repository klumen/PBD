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

layout (std430, binding = 5) buffer edgeBuffer
{
    uvec2 edge[];
};

layout (std430, binding = 6) buffer edgeLenBuffer
{
    float edgeLen[];
};

layout (std430, binding = 8) buffer nBuffer
{
    uint n[];
};

uniform uint head;
uniform uint edgeNum;

void main()
{
    const uint i = gl_GlobalInvocationID.x;
    if (i >= edgeNum) return;

    uint px = edge[i].x + head;
    uint py = edge[i].y + head;

    vec3 e = particle[px].predict - particle[py].predict;
    float e_l = length(e);
    float sigma = e_l / edgeLen[i];
    float sigma_min = 0.99;
    float sigma_max = 1.05;
    sigma = clamp(sigma, sigma_min, sigma_max);

    // float k = 1.0 - pow(1.0 - k_strech, 1.0 / iter);
    if (particle[px].massInv + particle[py].massInv == 0.0)
        return;
        
    vec3 corr1 = particle[px].massInv / (particle[px].massInv + particle[py].massInv) * (1.0 - sigma * edgeLen[i] / e_l) * e;
    vec3 corr2 = particle[py].massInv / (particle[px].massInv + particle[py].massInv) * (1.0 - sigma * edgeLen[i] / e_l) * e;

    n[edge[i].x] += 1;
    n[edge[i].y] += 1;

    atomicAdd(particle[px].corr.x, -corr1.x);
    atomicAdd(particle[px].corr.y, -corr1.y);
    atomicAdd(particle[px].corr.z, -corr1.z);
    atomicAdd(particle[py].corr.x, corr2.x);
    atomicAdd(particle[py].corr.y, corr2.y);
    atomicAdd(particle[py].corr.z, corr2.z);
}
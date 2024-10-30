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

uniform uint head;
uniform uint particleNum;
uniform float particleRadius;
uniform float mu_s;
uniform float mu_k;

float SDF(vec3 p, vec3 p0, vec3 n)
{
    return dot(p - p0, n) - particleRadius;
}

void collision(vec3 p, vec3 n)
{
    uint i = gl_GlobalInvocationID.x;
    uint particleId = i + head;
    float dis = SDF(particle[particleId].predict, p, n);
    if (dis < 0.0)
    {
        dis = -dis;
        particle[particleId].corr += dis * n;
        vec3 deltaPosition = particle[particleId].predict + particle[particleId].corr - particle[particleId].position;
        vec3 deltaPosition_t = deltaPosition - dot(deltaPosition, n) * n;
        float deltaLengh = length(deltaPosition_t);
        if (deltaLengh < mu_s * dis)
        {
            particle[particleId].corr -= deltaPosition_t;
        }
        else
        {
            particle[particleId].corr -= deltaPosition_t * min(1.0, mu_k * dis / deltaLengh);
        }
    }
}

void main()
{
    uint i = gl_GlobalInvocationID.x;
    if (i >= particleNum) return;

    collision(vec3(0.0, -1.7, 0.0), vec3(0.0, 1, 0.0));
    collision(vec3(1.0, 0.0, 0.0), vec3(-1, 0.0, 0.0));
    collision(vec3(-1.0, 0.0, 0.0), vec3(1, 0.0, 0.0));
    collision(vec3(0.0, 0.0, 0.7), vec3(0.0, 0.0, -1));
    collision(vec3(0.0, 0.0, -0.7), vec3(0.0, 0.0, 1));
}
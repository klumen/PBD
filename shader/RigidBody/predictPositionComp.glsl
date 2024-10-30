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
uniform float deltaTime;
uniform float damping;

void main()
{
    uint i = gl_GlobalInvocationID.x;
    uint particleId = i + head;
    if (i >= particleNum) return;

    particle[particleId].velocity += particle[particleId].force / particle[particleId].mass * deltaTime;
    particle[particleId].velocity *= exp(-damping * deltaTime);
    particle[particleId].predict = particle[particleId].position + particle[particleId].velocity * deltaTime;
    particle[particleId].corr = vec3(0.0);
}
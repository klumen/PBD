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

void main()
{
    const uint i = gl_GlobalInvocationID.x;
    if (i >= particleNum) return;
    const uint pi = i + head;

    particle[pi].velocity = (particle[pi].predict - particle[pi].position) / deltaTime;
    particle[pi].position = particle[pi].predict;
}
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

layout (std430, binding = 8) buffer positionBuffer
{
    vec4 position[];
};

uniform uint head;
uniform uint particleNum;

void main()
{
    uint i = gl_GlobalInvocationID.x;
    uint pi = head + i;
    if (i >= particleNum) return;

    particle[pi].position = particle[pi].predict;
    position[i] = vec4(particle[pi].position, 0.0);
}
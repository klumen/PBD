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

uniform uint head;
uniform uint particleNum;
uniform vec3 location;
uniform mat3 rotation;

void main()
{
    uint i = gl_GlobalInvocationID.x;
    uint particleId = i + head;
    if (i >= particleNum) return;

    vec3 g = rotation * r[i].xyz + location;
    particle[particleId].corr = g - particle[particleId].predict;
}
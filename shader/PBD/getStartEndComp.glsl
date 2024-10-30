#version 430 core

layout (local_size_x = 1024, local_size_y = 1, local_size_z = 1) in;

layout (std430, binding = 1) buffer tableBuffer
{
    uvec2 table[];
};

layout (std430, binding = 2) buffer orderBuffer
{
    uvec2 order[];
};

uniform uint particleNum;

void main()
{
    uint i = gl_GlobalInvocationID.x;
    if (i >= particleNum)
        return;
        
    if (i != 0)
    {
        if (table[i][0] != table[i - 1][0])
        {
            order[table[i][0]][0] = i;
            order[table[i - 1][0]][1] = i;
        }
    }

    if (i == particleNum - 1)
        order[table[i][0]][1] = particleNum;
}
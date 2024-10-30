#version 430 core

layout (local_size_x = 1024, local_size_y = 1, local_size_z = 1) in;

layout (std430, binding = 1) buffer tableBuffer
{
    uvec2 table[];
};

uniform uint dataLength;
uniform uint compareLength;

void main()
{
    uint i = gl_GlobalInvocationID.x;
    if (i >= dataLength) return;
    if ((i / compareLength) % 2 != 0) return;

    uint a = table[i][0];
    uint b = table[i + compareLength][0];
    uint aIndex = table[i][1];
    uint bIndex = table[i + compareLength][1];

    if (a < b)
    {
        table[i][0] = a;
        table[i][1] = aIndex;
        table[i + compareLength][0] = b;
        table[i + compareLength][1] = bIndex;
    }
    else
    {
        table[i][0] = b;
        table[i][1] = bIndex;
        table[i + compareLength][0] = a;
        table[i + compareLength][1] = aIndex;
    }
}
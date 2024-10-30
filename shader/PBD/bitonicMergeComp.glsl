#version 430 core

layout (local_size_x = 1024, local_size_y = 1, local_size_z = 1) in;

layout (std430, binding = 1) buffer tableBuffer
{
    uvec2 table[];
};

uniform uint dataLength;
uniform uint signLength;
uniform uint compareLength;

void main()
{
    uint i = gl_GlobalInvocationID.x;
    if (i >= dataLength) return;
    if ((i / compareLength) % 2 != 0) return;

    uint signValue = (i / signLength) % 2;

    uint a = table[i][0];
    uint b = table[i + compareLength][0];
    uint aIndex = table[i][1];
    uint bIndex = table[i + compareLength][1];

    uint maxValue;
    uint maxIndex;
    uint minValue;
    uint minIndex;

    if (a > b)
    {
        maxValue = a;
        maxIndex = aIndex;
        minValue = b;
        minIndex = bIndex;
    }
    else
    {
        maxValue = b;
        maxIndex = bIndex;
        minValue = a;
        minIndex = aIndex;
    }

    if (signValue == 1)
    {
        table[i][0] = maxValue;
        table[i][1] = maxIndex;
        table[i + compareLength][0] = minValue;
        table[i + compareLength][1] = minIndex;
    }
    else
    {
        table[i][0] = minValue;
        table[i][1] = minIndex;
        table[i + compareLength][0] = maxValue;
        table[i + compareLength][1] = maxIndex;
    }
}
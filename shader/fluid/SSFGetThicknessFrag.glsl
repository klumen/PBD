#version 430 core

in vec3 viewSpacePos;

uniform float pointSize;
uniform mat4 projection;

layout (location = 0) out float thickness;

void main()
{
    vec3 normal;
    normal.xy = gl_PointCoord.xy * vec2(2.0, -2.0) + vec2(-1.0, 1.0);
    float normalMag = dot(normal.xy, normal.xy);
    if (normalMag > 1.0) discard;
    normal.z = sqrt(1.0 - normalMag);

    thickness = normal.z * 2.0 * pointSize;
}
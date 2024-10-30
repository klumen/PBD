#version 430 core

in vec3 viewSpacePos;

uniform float pointSize;
uniform mat4 projection;

void main()
{
    vec3 normal;
    normal.xy = gl_PointCoord.xy * vec2(2.0, -2.0) + vec2(-1.0, 1.0);
    float normalMag = dot(normal.xy, normal.xy);
    if (normalMag > 1.0) discard;
    normal.z = sqrt(1.0 - normalMag);

    vec4 pixelViewPos = vec4(viewSpacePos + normal * pointSize, 1.0);
    vec4 pixelClipPos = projection * pixelViewPos;
    float NDCz = pixelClipPos.z / pixelClipPos.w;
    gl_FragDepth = NDCz;
}
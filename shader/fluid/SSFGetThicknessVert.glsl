#version 430 core

layout (location = 0) in vec3 aPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection; 
uniform float pointScale;
uniform float pointSize;
// uniform float minDensity

out vec3 viewSpacePos;

void main()
{
    viewSpacePos = (view * model * vec4(aPos, 1.0)).xyz;
    gl_PointSize = -pointScale * pointSize / viewSpacePos.z;

    // if (aPos.w < minDensity)
    //     gl_PointSize = 0.0;

    gl_Position = projection * vec4(viewSpacePos, 1.0);
}
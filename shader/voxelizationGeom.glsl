#version 430 core

layout (triangles) in;
layout (triangle_strip, max_vertices = 3) out;

out vec3 fragPos;
out vec3 projectPos;
out vec4 box;

uniform vec2 halfPixel[3];
uniform mat4 viewProject[3];
uniform mat4 viewProject_inv[3];

uint select_viewProject()
{
    vec3 p1 = gl_in[1].gl_Position.xyz - gl_in[0].gl_Position.xyz;
    vec3 p2 = gl_in[2].gl_Position.xyz - gl_in[0].gl_Position.xyz;
    vec3 normal = cross(p1, p2);
    float nx = abs(normal.x);
    float ny = abs(normal.y);
    float nz = abs(normal.z);

    if (nx > ny && nx > nz)
    {
        return 0;
    }
    else if (ny > nx && ny > nz)
    {
        return 1;
    }
    else if (nz > nx && nz > ny)
    {
        return 2;
    }
}

vec4 calcAABB(vec4 pos[3], vec2 pixelDiagonal)
{
    vec4 aabb;
    aabb.xy = min(pos[2].xy, min(pos[1].xy, pos[0].xy));
    aabb.zw = max(pos[2].xy, max(pos[1].xy, pos[0].xy));

    aabb.xy -= pixelDiagonal;
    aabb.zw += pixelDiagonal;
    return aabb;
}

void main()
{
    uint vpId = select_viewProject();
    vec4 pos[3] = vec4[3]
    (
        viewProject[vpId] * gl_in[0].gl_Position,
        viewProject[vpId] * gl_in[1].gl_Position,
        viewProject[vpId] * gl_in[2].gl_Position
    );

    vec4 triPlane;
    triPlane.xyz = normalize(cross(pos[1].xyz - pos[0].xyz, pos[2].xyz - pos[0].xyz));
    triPlane.w = -dot(pos[0].xyz, triPlane.xyz);

    if (dot(triPlane.xyz, vec3(0.0, 0.0, 1.0)) < 0.0)
    {
        vec4 vertexTemp = pos[2];
        pos[2] = pos[1];
        pos[1] = vertexTemp;
    }
    if (triPlane.z == 0.0) return;

    box = calcAABB(pos, halfPixel[vpId]);

    vec3 edgePlanes[3];
    edgePlanes[0] = cross(pos[0].xyw - pos[2].xyw, pos[2].xyw);
    edgePlanes[1] = cross(pos[1].xyw - pos[0].xyw, pos[0].xyw);
    edgePlanes[2] = cross(pos[2].xyw - pos[1].xyw, pos[1].xyw);
    edgePlanes[0].z -= dot(halfPixel[vpId], abs(edgePlanes[0].xy));
    edgePlanes[1].z -= dot(halfPixel[vpId], abs(edgePlanes[1].xy));
    edgePlanes[2].z -= dot(halfPixel[vpId], abs(edgePlanes[2].xy));

    vec3 intersection[3];
	intersection[0] = cross(edgePlanes[0], edgePlanes[1]);
	intersection[1] = cross(edgePlanes[1], edgePlanes[2]);
	intersection[2] = cross(edgePlanes[2], edgePlanes[0]);
	intersection[0] /= intersection[0].z;
	intersection[1] /= intersection[1].z;
	intersection[2] /= intersection[2].z;

    float z[3];
	z[0] = -(intersection[0].x * triPlane.x + intersection[0].y * triPlane.y + triPlane.w) / triPlane.z;
	z[1] = -(intersection[1].x * triPlane.x + intersection[1].y * triPlane.y + triPlane.w) / triPlane.z;
	z[2] = -(intersection[2].x * triPlane.x + intersection[2].y * triPlane.y + triPlane.w) / triPlane.z;
	pos[0].xyz = vec3(intersection[0].xy, z[0]);
	pos[1].xyz = vec3(intersection[1].xy, z[1]);
	pos[2].xyz = vec3(intersection[2].xy, z[2]);

    vec4 voxelPos;

    projectPos = pos[0].xyz;
	gl_Position = pos[0];
	voxelPos = viewProject_inv[vpId] * gl_Position;
	fragPos = voxelPos.xyz;
    EmitVertex();

	projectPos = pos[1].xyz;
	gl_Position = pos[1];
	voxelPos = viewProject_inv[vpId] * gl_Position;
	fragPos = voxelPos.xyz;
    EmitVertex();

	projectPos = pos[2].xyz;
	gl_Position = pos[2];
	voxelPos = viewProject_inv[vpId] * gl_Position;
	fragPos = voxelPos.xyz;
    EmitVertex();

    EndPrimitive();
}
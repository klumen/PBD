#ifndef MESH_H
#define MESH_H

#include <string>
#include <vector>
#include <limits>
#include <map>
#include <unordered_map>
#include <algorithm>

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Shader.h"

//constexpr auto MAX_BONE_INFLUENCE = 4;

struct Vertex {
	glm::vec3 position;
	glm::vec3 normal;
	glm::vec2 texCoord;
	/*glm::vec3 tangent;
	glm::vec3 bitangent;
	int boneIDs[MAX_BONE_INFLUENCE];
	float weights[MAX_BONE_INFLUENCE];*/
};

struct Texture
{
	unsigned int id = 0;
	std::string type;
	std::string name;
};

/*struct HEedge;
struct HEvert;
struct HEface;

struct HEedge
{
	HEvert* vert = nullptr;
	HEedge* pair = nullptr;
	HEface* face = nullptr;
	HEedge* next = nullptr;
};

struct HEvert
{
	unsigned int id;

	HEedge* edge = nullptr;
};

struct HEface
{
	HEedge* edge = nullptr;
};*/

struct Ray {
	float tMin = 1e-4f, tMax = 1e4f;
	glm::vec3 origin;
	glm::vec3 direction;
};

struct AABB
{
	glm::vec3 max{ 0.f }, min{ 0.f };

	bool intersect(const Ray& ray) const;
	AABB combine(const AABB& box) const;
};

class Mesh
{
public:
	Mesh(const std::string& name = "default", const std::vector<Vertex>& vertices = std::vector<Vertex>(), const std::vector<unsigned int>& indices = std::vector<unsigned int>(), const std::vector<Texture>& textures = std::vector<Texture>(), const AABB& box = AABB());
	~Mesh();

	// void generate_helf_edge();

	void draw(Shader& shader) const;
	/*void set_vertices(const std::vector<glm::vec3>& vec);
	void set_normals(const std::vector<glm::vec3>& vec);
	void set_texCoords(const std::vector<glm::vec2>& vec);
	void set_indices(const std::vector<unsigned int>& vec);

	void recalculate_normals();*/
	void recalculate_AABB();

	void surface_voxelize(const float step, std::vector<glm::vec3>& p);
	void voxelize(const float step, std::vector<glm::vec3>& p);

	std::string name;

	std::vector<Vertex> vertices;
	std::vector<unsigned int> indices;
	std::vector<Texture> textures;

	AABB box;

	// std::vector<HEedge*> HEedges;
	// std::vector<HEvert*> HEverts;
	// std::vector<HEface*> HEfaces;

private:
	void setup();

	unsigned int VAO = 0, VBO = 0, EBO = 0;

};

struct Triangle
{
	glm::vec3 v0, v1, v2;

	glm::vec3 get_center() const;
	bool ray_trace(Ray& ray, Triangle*& hitTri) const;
};

class AABBTreeNode
{
public:
	AABBTreeNode() = default;
	~AABBTreeNode();

	AABB box;
	AABBTreeNode* left = nullptr, * right = nullptr;
	Triangle* tri = nullptr;

	bool ray_trace(Ray& ray, Triangle*& hitTri);

private:

};

class AABBTree
{
public:
	AABBTree(Mesh* mesh);
	~AABBTree();

	bool ray_trace(Ray& ray, Triangle*& hitTri);
	void debug_draw();

	AABBTreeNode* root = nullptr;

private:
	AABBTreeNode* build(unsigned left, unsigned right, unsigned axis);

	std::vector<Triangle> tris;

};

#endif // !MESH_H
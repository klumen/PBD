#include "Mesh.h"

bool AABB::intersect(const Ray& ray) const
{
	float t_x0, t_x1, t_y0, t_y1, t_z0, t_z1;
	float x_inv = 1.f / ray.direction.x, y_inv = 1.f / ray.direction.y, z_inv = 1.f / ray.direction.z;

	if (x_inv >= 0.f)
	{
		t_x0 = (min.x - ray.origin.x) * x_inv;
		t_x1 = (max.x - ray.origin.x) * x_inv;
	}
	else
	{
		t_x1 = (min.x - ray.origin.x) * x_inv;
		t_x0 = (max.x - ray.origin.x) * x_inv;
	}

	if (y_inv >= 0.f)
	{
		t_y0 = (min.y - ray.origin.y) * y_inv;
		t_y1 = (max.y - ray.origin.y) * y_inv;
	}
	else
	{
		t_y1 = (min.y - ray.origin.y) * y_inv;
		t_y0 = (max.y - ray.origin.y) * y_inv;
	}

	if (z_inv >= 0.f)
	{
		t_z0 = (min.z - ray.origin.z) * z_inv;
		t_z1 = (max.z - ray.origin.z) * z_inv;
	}
	else
	{
		t_z1 = (min.z - ray.origin.z) * z_inv;
		t_z0 = (max.z - ray.origin.z) * z_inv;
	}

	float t_enter = std::max({ t_x0, t_y0, t_z0 });
	float t_exit = std::min({ t_x1, t_y1, t_z1 });

	if (t_enter > t_exit || ray.tMin > t_exit || ray.tMax < t_enter)
		return false;

	return true;
}

AABB AABB::combine(const AABB& box) const
{
	float x1 = std::min(min.x, box.min.x);
	float y1 = std::min(min.y, box.min.y);
	float z1 = std::min(min.z, box.min.z);
	float x2 = std::max(max.x, box.max.x);
	float y2 = std::max(max.y, box.max.y);
	float z2 = std::max(max.z, box.max.z);

	AABB aabb;
	aabb.min = glm::vec3(x1, y1, z1);
	aabb.max = glm::vec3(x2, y2, z2);

	return aabb;
}


Mesh::Mesh(const std::string& name /*= "default"*/, const std::vector<Vertex>& vertices /*= std::vector<Vertex>()*/, const std::vector<unsigned int>& indices /*= std::vector<unsigned int>()*/, const std::vector<Texture>& textures /*= std::vector<Texture>()*/, const AABB& box /*= AABB()*/) : name(name), vertices(vertices), indices(indices), textures(textures), box(box)
{
	//generate_helf_edge();
	setup();
}

Mesh::~Mesh() {}

/*void Mesh::generate_helf_edge()
{
	if (indices.empty())
		return;

	HEverts.clear();
	HEverts.resize(vertices.size());
	HEedges.clear();
	HEedges.reserve(indices.size());
	HEfaces.clear();
	HEfaces.reserve(indices.size() / 3);

	for (unsigned int i = 0; i < vertices.size(); i++)
		HEverts[i].id = i;

	std::map<std::pair<unsigned int, unsigned int>, HEedge*> edgeMap;

	for (unsigned int i = 0; i < indices.size(); i += 3)
	{
		HEfaces.emplace_back();
		HEface& face = HEfaces.back();
		HEedge* preEdge = nullptr;
		HEedge* firstEdge = nullptr;

		for (unsigned int j = 0; j < 3; j++)
		{
			unsigned int v1 = indices[i + j];
			unsigned int v2 = indices[i + (j + 1) % 3];

			HEedges.emplace_back();
			HEedge& edge = HEedges.back();
			edge.vert = &HEverts[v1];
			edge.face = &face;

			if (j == 0)
				firstEdge = &edge;
			else
				preEdge->next = &edge;
			preEdge = &edge;

			auto edgeKey = std::make_pair(v1, v2);
			edgeMap[edgeKey] = &edge;
		}

		preEdge->next = face.edge = firstEdge;
	}

	for (auto& edge : HEedges)
	{
		if (!edge.vert->edge)
			edge.vert->edge = &edge;

		unsigned int v1 = edge.vert->id;
		unsigned int v2 = edge.next->vert->id;

		auto edgeKey = std::make_pair(v2, v1);
		if (edgeMap.find(edgeKey) != edgeMap.end()) 
 		{
			edge.pair = edgeMap[edgeKey];
			edgeMap[edgeKey]->pair = &edge;
		}
	}
}*/

void Mesh::setup()
{
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned), indices.data(), GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glBindVertexArray(VAO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoord));
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);

	glBindVertexArray(0); // Attantion VAO and EBO unbind order.
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

/*void Mesh::set_vertices(const std::vector<glm::vec3>& vec)
{
	if (vec.empty())
	{
		std::cerr << "MESH::SET_VERTICES::INPUTE_ERROR" << std::endl;
		exit(-1);
	}

	vertices = vec;

	if (VBOs[0] == 0)
		glGenBuffers(1, &VBOs[0]);

	glBindBuffer(GL_ARRAY_BUFFER, VBOs[0]);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), &vertices[0], GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void Mesh::set_normals(const std::vector<glm::vec3>& vec)
{
	if (vec.empty())
	{
		std::cerr << "MESH::SET_NORMALS::INPUTE_ERROR" << std::endl;
		exit(-1);
	}

	normals = vec;

	if (VBOs[1] == 0)
		glGenBuffers(1, &VBOs[1]);

	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBOs[1]);

	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), &vertices[0], GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
	glEnableVertexAttribArray(0);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

void Mesh::set_texCoords(const std::vector<glm::vec2>& vec)
{
	if (vec.empty())
	{
		std::cerr << "MESH::SET_TEXCOORDS::INPUTE_ERROR" << std::endl;
		exit(-1);
	}

	texCoords = vec;

	if (VBOs[2] == 0)
		glGenBuffers(1, &VBOs[2]);

	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBOs[2]);

	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), &vertices[0], GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
	glEnableVertexAttribArray(0);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

void Mesh::set_indices(const std::vector<unsigned int>& vec)
{
	if (vec.empty())
	{
		std::cerr << "MESH::SET_INDICES::INPUTE_ERROR" << std::endl;
		exit(-1);
	}

	indices = vec;

	if (EBO == 0)
		glGenBuffers(1, &EBO);

	glBindVertexArray(VAO);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

	glBindVertexArray(0);
}

void Mesh::recalculate_normals()
{
	if (vertices.empty())
	{
		std::cerr << "MESH::RECALCULATE_NORMALS::VERTICES_EMPTY" << std::endl;
		exit(-1);
	}

	std::vector<float> sum_area(vertices.size(), 0.f);
	for (unsigned int i = 0; i < indices.size(); i += 3)
	{
		const glm::vec3& v0 = vertices[indices[i]].position;
		const glm::vec3& v1 = vertices[indices[i + 1]].position;
		const glm::vec3& v2 = vertices[indices[i + 2]].position;

		glm::vec3 e1 = v1 - v0;
		glm::vec3 e2 = v2 - v0;

		glm::vec3 n = glm::cross(e1, e2);
		float area = sqrtf(glm::dot(n, n));
		n /= area;

		sum_area[indices[i]] += area;
		sum_area[indices[i + 1]] += area;
		sum_area[indices[i + 2]] += area;

		normals[indices[i]] += area * n;
		normals[indices[i + 1]] += area * n;
		normals[indices[i + 2]] += area * n;
	}

	for (unsigned int i = 0; i < vertices.size(); i++)
		normals[i] = glm::normalize(normals[i] / sum_area[i]);

	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBOs[1]);

	glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(glm::vec3), &normals[0], GL_STATIC_DRAW);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
	glEnableVertexAttribArray(1);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}*/

void Mesh::recalculate_AABB()
{
	if (vertices.empty())
	{
		std::cerr << "MESH::RECALCULATE_AABB::VERTICES_EMPTY" << std::endl;
		exit(-1);
	}

	glm::vec3 min(std::numeric_limits<float>::max());
	glm::vec3 max(std::numeric_limits<float>::lowest());

	for (unsigned int i = 0; i < vertices.size(); i++)
		for (unsigned int j = 0; j < 3; j++)
		{
			min[j] = std::min(min[j], vertices[i].position[j]);
			max[j] = std::max(max[j], vertices[i].position[j]);
		}

	box.max = glm::vec3(std::move(max));
	box.min = glm::vec3(std::move(min));
}

void Mesh::draw(Shader& shader) const
{
	unsigned int diffuseNr = 0;
	unsigned int specularNr = 0;
	for (unsigned int i = 0; i < textures.size(); i++)
	{
		std::string number;
		std::string name = textures[i].type;
		if (name == "diffuse")
			number = std::to_string(diffuseNr++);
		else if (name == "specular")
			number = std::to_string(specularNr++);

		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, textures[i].id);
		shader.set_int("material." + name + number, i);
	}
	glActiveTexture(GL_TEXTURE0);
	if (textures.empty())
		shader.set_bool("hasTexture", false);
	else 
		shader.set_bool("hasTexture", true);

	glBindVertexArray(VAO);
	glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);
}

void Mesh::surface_voxelize(const float step, std::vector<glm::vec3>& p)
{
	if (box.max == box.min)
		recalculate_AABB();

	const glm::vec3& min = box.min;
	const glm::vec3& max = box.max;
	glm::vec3 range(max.x - min.x, max.y - min.y, max.z - min.z);
	glm::u32vec3 resolution(0);
	resolution.x = range.x / step + 1;
	resolution.y = range.y / step + 1;
	resolution.z = range.z / step + 1;
	unsigned int size = resolution.x * resolution.y * resolution.z;

	float offset = 0.2f;

	glm::vec3 cameraPosZ((max.x + min.x) * 0.5f, (max.y + min.y) * 0.5f, max.z + offset);
	glm::mat4 viewZ = glm::lookAt(cameraPosZ, cameraPosZ + glm::vec3(0.f, 0.f, -1.f), glm::vec3(0.f, 1.f, 0.f));
	glm::mat4 projectZ = glm::ortho(-range.x * 0.51f, range.x * 0.51f, -range.y * 0.51f, range.y * 0.51f, 0.1f, range.z + offset);

	glm::vec3 cameraPosX(max.x + offset, (max.y + min.y) * 0.5f, (max.z + min.z) * 0.5f);
	glm::mat4 viewX = glm::lookAt(cameraPosX, cameraPosX + glm::vec3(-1.f, 0.f, 0.f), glm::vec3(0.f, 1.f, 0.f));
	glm::mat4 projectX = glm::ortho(-range.z * 0.51f, range.z * 0.51f, -range.y * 0.51f, range.y * 0.5f, 0.1f, range.x + offset);

	glm::vec3 cameraPosY((max.x + min.x) * 0.5f, max.y + offset, (max.z + min.z) * 0.5f);
	glm::mat4 viewY = glm::lookAt(cameraPosY, cameraPosY + glm::vec3(0.f, -1.f, 0.f), glm::vec3(0.f, 1.f, 0.01f));
	glm::mat4 projectY = glm::ortho(-range.x * 0.51f, range.x * 0.51f, -range.z * 0.51f, range.z * 0.5f, 0.1f, range.y + offset);

	PipelineShader shader("shader/voxelizationVert.glsl", "shader/voxelizationFrag.glsl", "shader/voxelizationGeom.glsl");

	// glViewport(0, 0, resolution.x * 2, resolution.y * 2);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
	glClearColor(1.f, 0.f, 0.f, 1.f);
	glClear(GL_COLOR_BUFFER_BIT);

	unsigned int cntBuffer;
	glGenBuffers(1, &cntBuffer);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, cntBuffer);
	glBufferData(GL_SHADER_STORAGE_BUFFER, size * sizeof(unsigned int), nullptr, GL_STATIC_DRAW);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, cntBuffer);

	shader.use();

	shader.set_vec3("boxMin", min);
	shader.set_float("step", step);
	shader.set_vec3("resolution", resolution);
	shader.set_vec2("halfPixel[0]", glm::vec2(1.0f / resolution.z, 1.0f / resolution.y));
	shader.set_vec2("halfPixel[1]", glm::vec2(1.0f / resolution.x, 1.0f / resolution.z));
	shader.set_vec2("halfPixel[2]", glm::vec2(1.0f / resolution.x, 1.0f / resolution.y));
	shader.set_mat4("viewProject[0]", projectX * viewX);
	shader.set_mat4("viewProject[1]", projectY * viewY);
	shader.set_mat4("viewProject[2]", projectZ * viewZ);
	shader.set_mat4("viewProject_inv[0]", glm::inverse(projectX * viewX));
	shader.set_mat4("viewProject_inv[1]", glm::inverse(projectY * viewY));
	shader.set_mat4("viewProject_inv[2]", glm::inverse(projectZ * viewZ));

	unsigned int* writePtr = reinterpret_cast<unsigned int*>(glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_WRITE_ONLY));
	for (unsigned int i = 0; i < size; i++)
		writePtr[i] = 0;
	if (!glUnmapBuffer(GL_SHADER_STORAGE_BUFFER))
		std::cerr << "unMap error" << std::endl;
	
	draw(shader);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, cntBuffer);
	unsigned int* readPtr = reinterpret_cast<unsigned int*>(glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY));

	std::vector<glm::vec3> pos;
	if (readPtr != nullptr)
	{
		for (unsigned int i = 0; i < size; i++)
		{
			if (readPtr[i] != 0)
			{
				unsigned int y = i / (resolution.x * resolution.z);
				unsigned int z = (i - y * resolution.x * resolution.z) / resolution.x;
				unsigned int x = i - y * resolution.x * resolution.z - z * resolution.x;
				pos.emplace_back(min + glm::vec3(x, y, z) * step);
			}
		}
	}
	else
	{
		std::cout << "read error!" << std::endl;
	}

	glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
	glDeleteBuffers(1, &cntBuffer);

	p = std::move(pos);
}

void Mesh::voxelize(const float step, std::vector<glm::vec3>& p)
{
	if (box.max == box.min)
		recalculate_AABB();

	const glm::vec3 max = box.max;
	const glm::vec3 min = box.min;
	const glm::vec3 range = max - min;
	const glm::u32vec3 resolution = glm::u32vec3(range / step) + glm::u32vec3(1u);

	const glm::vec3 offset(0.5f * step);
	const float eps = 0.001f * step;

	AABBTree tree(this);

	for (unsigned x = 0; x < resolution.x; x++)
		for (unsigned y = 0; y < resolution.y; y++)
		{
			Ray ray;
			ray.direction = glm::vec3(0.f, 0.f, 1.f);
			ray.origin = min + glm::vec3(x * step + offset.x, y * step + offset.y, 0.f);
			ray.tMin = 0.f;

			bool inside = false;
			Triangle* lastTri = nullptr;
			Triangle* tri = nullptr;

			while (tree.ray_trace(ray, tri))
			{
				const float zpos = ray.origin.z + ray.tMax * ray.direction.z;

				unsigned zstart = std::floorf((ray.origin.z - min.z) / step + 0.5f);
				unsigned zend = std::min(std::floorf((zpos - min.z) / step + 0.5f), resolution.z * 1.f);

				if (inside)
				{
					for (unsigned z = zstart; z < zend; ++z)
					{
						glm::vec3 pos(min);
						pos += glm::vec3(x * step + offset.x, y * step + offset.y, z * step + offset.z);
						p.emplace_back(std::move(pos));
					}
				}

				inside = !inside;

				if (lastTri == tri)
				{
					std::cerr << "MESH::VOXELIZE self-intersection!" << std::endl;
					exit(-1);
				}
				lastTri = tri;

				ray.origin += ray.direction * (ray.tMax + eps);
				ray.tMax = 1e4f;
			}
		}
}


glm::vec3 Triangle::get_center() const
{
	return (v0 + v1 + v2) / 3.f;
}

bool Triangle::ray_trace(Ray& ray, Triangle*& hitTri) const
{
	glm::vec3 AC = v2 - v0;
	glm::vec3 BC = v2 - v1;
	glm::vec3 OC = v2 - ray.origin;
	const glm::vec3& d = ray.direction;
	glm::mat3 tempM({
		{ d.x, AC.x, BC.x },
		{ d.y, AC.y, BC.y },
		{ d.z, AC.z, BC.z } });
	float M_inv = 1.f / glm::determinant(tempM);

	tempM = glm::mat3({
		{ OC.x, AC.x, BC.x },
		{ OC.y, AC.y, BC.y },
		{ OC.z, AC.z, BC.z } });
	float t_new = glm::determinant(tempM) * M_inv;

	if (t_new > ray.tMax || t_new < ray.tMin)
		return false;

	tempM = glm::mat3({
		{ d.x, OC.x, BC.x },
		{ d.y, OC.y, BC.y },
		{ d.z, OC.z, BC.z } });
	float alpha = glm::determinant(tempM) * M_inv;

	if (alpha < 0.f || alpha > 1.f)
		return false;

	tempM = glm::mat3({
		{ d.x, AC.x, OC.x },
		{ d.y, AC.y, OC.y },
		{ d.z, AC.z, OC.z } });
	float beta = glm::determinant(tempM) * M_inv;

	if (beta < 0.f || beta + alpha > 1.f)
		return false;

	ray.tMax = t_new;
	hitTri = const_cast<Triangle*>(this);

	return true;
}


AABBTreeNode::~AABBTreeNode()
{
	tri = nullptr;

	if (left != nullptr)
	{
		delete left;
		left = nullptr;
	}

	if (right != nullptr)
	{
		delete right;
		right = nullptr;
	}
}

bool AABBTreeNode::ray_trace(Ray& ray, Triangle*& hitTri)
{
	bool hit = false;

	if (box.intersect(ray))
	{
		if (!left && !right)
		{
			return tri->ray_trace(ray, hitTri);
		}
		else
		{
			if (left) hit |= left->ray_trace(ray, hitTri);
			if (right) hit |= right->ray_trace(ray, hitTri);
		}
	}

	return hit;
}


AABBTree::AABBTree(Mesh* mesh)
{
	if (!mesh)
		return;

	tris.resize(mesh->indices.size() / 3);
	for (unsigned i = 0; i < tris.size(); i++)
	{
		const glm::vec3& v0 = mesh->vertices[mesh->indices[i * 3]].position;
		const glm::vec3& v1 = mesh->vertices[mesh->indices[i * 3 + 1]].position;
		const glm::vec3& v2 = mesh->vertices[mesh->indices[i * 3 + 2]].position;
		Triangle tri{ v0, v1, v2 };
		tris[i] = std::move(tri);
	}

	root = build(0, tris.size(), 0);
}

AABBTree::~AABBTree()
{
	if (!root)
	{
		delete root;
		root = nullptr;
	}
}

AABBTreeNode* AABBTree::build(unsigned left, unsigned right, unsigned axis)
{
	AABBTreeNode* node = new AABBTreeNode();

	if (right - left == 1)
	{
		const Triangle& tri = tris[left];
		AABB box;
		box.min.x = std::min({ tri.v0.x, tri.v1.x, tri.v2.x });
		box.min.y = std::min({ tri.v0.y, tri.v1.y, tri.v2.y });
		box.min.z = std::min({ tri.v0.z, tri.v1.z, tri.v2.z });
		box.max.x = std::max({ tri.v0.x, tri.v1.x, tri.v2.x });
		box.max.y = std::max({ tri.v0.y, tri.v1.y, tri.v2.y });
		box.max.z = std::max({ tri.v0.z, tri.v1.z, tri.v2.z });

		node->box = box;
		node->tri = &tris[left];

		return node;
	}

	std::sort(tris.begin() + left, tris.begin() + right, [axis](const Triangle& t0, const Triangle& t1){
			return t0.get_center()[axis] < t1.get_center()[axis];
		});

	unsigned mid = (left + right) / 2;
	node->left = build(left, mid, (axis + 1) % 3);
	node->right = build(mid, right, (axis + 1) % 3);
	node->box = node->left->box.combine(node->right->box);

	return node;
}

bool AABBTree::ray_trace(Ray& ray, Triangle*& hitTri)
{
	if (!root)
		return false;

	return root->ray_trace(ray, hitTri);
}


Sphere::Sphere(const glm::vec3& position /*= glm::vec3(0.f)*/, float radius /*= 1.f*/) : position(position), radius(radius)
{

}

Sphere::~Sphere()
{
	if (mesh != nullptr)
	{
		delete mesh;
		mesh = nullptr;
	}
}

void Sphere::generate_mesh(unsigned int ringNum /*= 16*/, unsigned int segmentNum /*= 32*/)
{
	if (radius == 0.f || ringNum < 3 || segmentNum < 3)
		return;

	const float PI = acosf(-1.f);

	std::vector<Vertex> vertices;
	for (unsigned int i = 0; i <= ringNum; i++)
	{
		float ring = (float)i / (float)ringNum;
		for (unsigned int j = 0; j <= segmentNum; j++)
		{
			float segment = (float)j / (float)segmentNum;
			float xPos = cosf(segment * 2.f * PI) * sinf(ring * PI);
			float yPos = cosf(ring * PI);
			float zPos = sinf(segment * 2.f * PI) * sinf(ring * PI);

			Vertex vertex;
			vertex.position = glm::vec3(xPos, yPos, zPos);
			vertex.normal = glm::vec3(glm::normalize(glm::vec3(xPos, yPos, zPos)));
			vertex.texCoord = glm::vec2(ring, segment);
			vertices.emplace_back(std::move(vertex));
		}
	}

	std::vector<unsigned int> indices;
	for (unsigned int i = 0; i < ringNum; i++)
		for (unsigned int j = 0; j < segmentNum; j++)
		{
			indices.emplace_back(i * (segmentNum + 1) + j);
			indices.emplace_back(i * (segmentNum + 1) + j + 1);
			indices.emplace_back((i + 1) * (segmentNum + 1) + j);

			indices.emplace_back(i * (segmentNum + 1) + j + 1);
			indices.emplace_back((i + 1) * (segmentNum + 1) + j + 1);
			indices.emplace_back((i + 1) * (segmentNum + 1) + j);
		}


	mesh = new Mesh("Sphere", vertices, indices);
}
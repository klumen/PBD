#ifndef SDF_H
#define SDF_H

#include "Mesh.h"

class SDF
{
public:
	SDF(Mesh* mesh);
	~SDF();

	void generate_SDF(const std::vector<glm::vec3>& particles, std::vector<float>& data, std::vector<glm::vec3>& gradient);

private:
	void init_angle_weighted_pseudo_normal();
	glm::vec3 get_closest_point_on_triangle(glm::vec3 p, glm::vec3 n, unsigned v[3], bool& inside) const;

	Mesh* mesh;
	std::vector<glm::vec3> faceNormal;
	std::map<std::pair<unsigned int, unsigned int>, glm::vec3> edgeNormal;
	std::vector<glm::vec3> vertNormal;

};

#endif // !SDF_H
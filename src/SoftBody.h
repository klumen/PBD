#ifndef SOFTBODY_H
#define SOFTBODY_H

#include "Math.h"
#include "Mesh.h"
#include "Particle.h"
#include "SDF.h"

class SoftBody
{
public:
	SoftBody(Mesh* mesh);
	~SoftBody();

	void damping_v();

	static void shape_matching_constraint(SoftBody& sb);
	static void mesh_contact_constrain(SoftBody& sb);
	static void particle_contact_constrain(SoftBody& sb);

	unsigned int particleNr = 0;
	std::vector<Particle> particles;

	Mesh* mesh = nullptr;
	glm::vec3 gravity{ 0.f, -9.8f, 0.f };
	float mass = 30.f;

	float damping = 0.2f;
	float stiffness = 1.f;

	std::vector<glm::vec3> r0;
	glm::vec3 cm0{ 0.f };
	glm::mat3 R0{ 1.f };

	std::vector<glm::vec3> gradientSDF;
	std::vector<float> SDFdata;

private:
	std::vector<glm::vec3> r;
	glm::mat3 rr_inv{ 0.f };

};

#endif // !SOFTBODY_H
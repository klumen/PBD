#ifndef PARTICLE_H
#define PARTICLE_H

#include "Mesh.h"

struct Particle
{
	static float radius;
	static std::vector<Particle*> globalParticles;
	static unsigned totPhase;

	glm::vec3 f{ 0.f };
	float m = 0.f;
	glm::vec3 v{ 0.f };
	float w = 0.f;
	glm::vec3 x{ 0.f };
	unsigned phase = 0;
	glm::vec3 pred{ 0.f };
	float contactMass = 0.f;
	glm::vec3 corr{ 0.f };
	float height = 0.f;
};

#endif // PARTICLE_H
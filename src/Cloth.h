#ifndef CLOTH_H
#define CLOTH_H

#include <algorithm>
#include <set>

#include "Mesh.h"
#include "Particle.h"
#include "Shader.h"

class Cloth
{
public:
	Cloth(Mesh* mesh);
	~Cloth();

	static void create_shaders();

	void bind_buffers();
	void predict_position();
	void neighborhood_search();
	void solve_strech_constraint();
	void solve_environment_collision_constraint();
	void solve_particle_collision_constraint();
	void solve_attach_constraint();
	void update_predict_position();
	void update_position();
	//void smooth_velocity();

	Mesh* mesh = nullptr;
	glm::vec3 gravity{ 0.f, -9.8f, 0.f };

	unsigned head = 0;
	unsigned particleNr = 0;
	std::vector<Particle> particles;
	unsigned workGroupSize = 0;
	unsigned workGroupNum = 0;

	float deltaTime = 0.015f;
	float rho = 0.2f;
	float damping = 0.5f;
	float mu_s = 0.6f;
	float mu_k = 0.5f;

private:
	void generate_mass();
	void init_buffers();

	static ComputeShader* predictPositionCompShader;
	static ComputeShader* neighborhoodSearchCompShader;
	static ComputeShader* solveStrechConstraintCompShader;
	static ComputeShader* solveAttachConstraintCompShader;
	static ComputeShader* solveEnvironmentCollisionConstraintCompShader;
	static ComputeShader* solveParticleCollisionConstraintCompShader;
	static ComputeShader* updatePredictPositionCompShader;
	static ComputeShader* updatePositionCompShader;

	unsigned edgeSSBO = 0;
	unsigned edgeLenSSBO = 0;
	unsigned attachSSBO = 0;
	unsigned nSSBO = 0;
	unsigned rSSBO = 0;

	float k_strech = 1.f;
	std::vector<glm::uvec2> edge;
	std::vector<float> length;

	float k_bending = 0.5f;
	std::vector<glm::uvec4> nbrInd;

	std::vector<unsigned int> attaId;
	std::vector<glm::vec3> atta;
	unsigned maxAttaments = 4;
	std::vector<float> r;

};

#endif // !CLOTH_H
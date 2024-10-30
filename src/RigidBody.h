#ifndef RIGIDBODY_H
#define RIGIDBODY_H

#include "Shader.h"
#include "Math.h"
#include "Mesh.h"
#include "Particle.h"
#include "SDF.h"

class RigidBody
{
public:
	RigidBody(Mesh* mesh);
	~RigidBody();

	void bind_buffers();
	void predict_position();
	void neighborhood_search();
	void solve_environment_collision_constraint();
	void solve_particle_collision_constraint();
	void solve_shape_matching_constraint();
	void update_predict_position();
	void update_position();

	static void create_shaders();

	static ComputeShader* predictPositionCompShader;
	static ComputeShader* neighborhoodSearchCompShader;
	static ComputeShader* solveEnvironmentCollisionConstraintCompShader;
	static ComputeShader* solveParticleCollisionConstraintCompShader;
	static ComputeShader* calculateLocationCompShader;
	static ComputeShader* calculateRotationCompShader;
	static ComputeShader* solveShapeMatchingConstraintCompShader;
	static ComputeShader* updatePredictPositionCompShader;
	static ComputeShader* updatePositionCompShader;

	unsigned head = 0;
	unsigned int particleNr = 0;
	std::vector<Particle> particles;

	Mesh* mesh = nullptr;
	glm::vec3 gravity{ 0.f, -9.8f, 0.f };
	float mass = 1.f;

	float deltaTime = 0.015f;

	float damping = 0.5f;
	float mu_s = 0.5f;
	float mu_k = 0.4f;

	glm::vec3 c0{ 0.f };
	glm::vec3 cm{ 0.f };
	glm::mat3 R{ 1.f };
	glm::mat4 model{ 1.f };

private:
	void init_buffers();

	unsigned workGroupSize = 0;
	unsigned workGroupNum = 0;

	std::vector<glm::vec4> r;
	glm::mat3 rr_inv{ 0.f };

	std::vector<float> SDFdata;
	std::vector<glm::vec3> SDFgrad;

	unsigned sdfBuffer = 0;
	unsigned locationSumBuffer = 0;
	unsigned rBuffer = 0;
	unsigned rotationSumBuffer = 0;

	unsigned texture = 0;

	unsigned FBO = 0;

};

#endif // !RIGIDBODY_H
#ifndef FLUID_H
#define FLUID_H

#include "Mesh.h"
#include "Particle.h"
#include "Light.h"
#include "Global.h"

class Fluid
{
public:
	Fluid(Mesh* mesh);
	~Fluid();

	static void create_shaders();
	static void delete_shaders();

	void bind_buffers();
	void predict_position();
	void neighborhood_search();
	void calculate_lambda();
	void solve_incompressibility_constraint();
	void solve_environment_collision_constraint();
	void solve_particle_collision_constraint();
	void update_predict_position();
	void update_velocity();
	void apply_vorticity_confinement();
	void apply_XSPHV_viscosity();
	void update_position();

	void draw();
	void get_depth();
	void get_thickness();
	void blur();
	void shading();

	unsigned head = 0;
	unsigned particleNr = 0;
	std::vector<Particle> particles;

	float rho0 = 1000.f;
	float solidPressure = 1.f;
	float vorticity = 0.1f;
	float viscosity = 0.01f;
	float mu_s = 0.015f;
	float mu_k = 0.01f;

	unsigned blurIteration = 3;
	unsigned filterRadius = 8;
	float spatialScale = 0.1f;
	float rangeScale = 100.f;
	glm::mat4 model{ 1.f };

private:
	void init_buffers();

	static ComputeShader* predictPositionCompShader;
	static ComputeShader* neighborhoodSearchCompShader;
	static ComputeShader* calculateLambdaCompShader;
	static ComputeShader* solveIncompressibilityConstraintCompShader;
	static ComputeShader* solveEnvironmentCollisionConstraintCompShader;
	static ComputeShader* solveParticleCollisionConstraintShader;
	static ComputeShader* updatePredictPositionCompShader;
	static ComputeShader* updateVelocityCompShader;
	static ComputeShader* calculateOmegaCompShader;
	static ComputeShader* applyVorticityConfinementCompShader;
	static ComputeShader* applyXSPHVViscosityCompShader;
	static ComputeShader* updatePositionCompShader;

	static PipelineShader* SSFGetDepthShader;
	static PipelineShader* SSFGetThicknessShader;
	static PipelineShader* SSFBlurShader;
	static PipelineShader* SSFRenderShader;

	Mesh* mesh = nullptr;	
	glm::vec3 gravity{ 0.f, -9.8f, 0.f };

	float deltaTime = 0.015f;
	float h = 4.f * Particle::radius;

	unsigned workGroupSize = 0;
	unsigned workGroupNum = 0;
	
	unsigned rhoBuffer = 0;
	unsigned lambdaBuffer = 0;
	unsigned omegaBuffer = 0;
	unsigned positionBuffer = 0;

	unsigned VAO = 0;

	unsigned depthTexture = 0;
	unsigned thicknessTexture = 0;
	unsigned blurDepthTex[2]{};
	unsigned blurthicknessTex[2]{};

	unsigned depthFBO = 0;
	unsigned thicknessFBO = 0;
	unsigned blurFBO[2]{};

};

#endif // !FLUID_H
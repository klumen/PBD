#ifndef PBD_H
#define PBD_H

#include "Shader.h"
#include "Mesh.h"
#include "Particle.h"
#include "RigidBody.h"
#include "SoftBody.h"
#include "Cloth.h"
#include "Fluid.h"

class PBD
{
	friend Fluid;

public:
	PBD();
	~PBD();

	void add_rigid_body(RigidBody* rigidBody);
	void add_soft_body(SoftBody* softBody);
	void add_cloth(Cloth* cloth);
	void add_fluid(Fluid* fluid);

	void init();
	void update(float dt);
	void draw(Shader& shader);

	float omega = 1.f; // 1~2
	float epsilon = 0.01f; // x

private:
	void create_shaders();
	void init_buffers();
	void bind_buffers();

	void predict_particle();
	void neighborhood_search();
	void solver();
	void update_particle();

	std::vector<RigidBody*> rigidBodies;
	std::vector<SoftBody*> softBodies;
	std::vector<Cloth*> cloths;
	std::vector<Fluid*> fluids;
	std::vector<Particle> particles;

	unsigned particleNum = 0;
	float deltaTime = 0.f;
	unsigned sortSize = 0;
	unsigned workGroupSize = 0;
	unsigned workGroupNum = 0;
	float cellSize = 4 * Particle::radius;
	unsigned tableSize = 0;
	unsigned maxNeighbors = 96;

	ComputeShader* spitalHashCompShader = nullptr;
	ComputeShader* bitonicMergeCompShader = nullptr;
	ComputeShader* bitonicSortCompShader = nullptr;
	ComputeShader* getStartEndCompShader = nullptr;

	unsigned particleSSBO = 0;
	unsigned tableSSBO = 0;
	unsigned orderSSBO = 0;
	unsigned neighborSSBO = 0;
	unsigned countSSBO = 0;

	unsigned neighborUBO = 0;

	unsigned backgroundTex = 0;
	unsigned backgroundDepthTex = 0;

	unsigned backgroundFBO = 0;

};

#endif // !PBD_H
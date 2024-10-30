#include "PBD.h"

PBD::PBD() {}

PBD::~PBD() {}

void PBD::add_rigid_body(RigidBody* rigidBody)
{ 
	if (rigidBody)
	{
		rigidBodies.emplace_back(rigidBody);
		for (auto& particle : rigidBody->particles)
			particles.emplace_back(particle);

		rigidBody->head = particleNum;
		particleNum += rigidBody->particleNr;
	}
}
void PBD::add_soft_body(SoftBody* softBody)
{
	/*if (softBody)
	{
		softBodies.emplace_back(softBody);
		for (auto& particle : softBody->particles)
			particles.emplace_back(particle);
	}*/
}
void PBD::add_cloth(Cloth* cloth)
{
	if (cloth)
	{
		cloths.emplace_back(cloth);
		for (auto& particle : cloth->particles)
			particles.emplace_back(particle);

		cloth->head = particleNum;
		particleNum += cloth->particleNr;
	}
}
void PBD::add_fluid(Fluid* fluid)
{
	if (fluid)
	{
		fluids.emplace_back(fluid);
		for (auto& particle : fluid->particles)
			particles.emplace_back(particle);

		fluid->head = particleNum;
		particleNum += fluid->particleNr;
	}
}

void PBD::init()
{
	RigidBody::create_shaders();
	Cloth::create_shaders();
	Fluid::create_shaders();

	if (particleNum & (particleNum - 1))
		sortSize = static_cast<unsigned>(pow(2, std::ceil(log2(particleNum))));
	if (sortSize < 1024)
		sortSize = 1024;
	tableSize = 2 * particleNum;

	workGroupSize = 1024;
	workGroupNum = static_cast<unsigned>(ceil(particleNum * 1.f / workGroupSize));

	create_shaders();
	init_buffers();
}

void PBD::create_shaders()
{
	spitalHashCompShader = new ComputeShader("shader/PBD/spitalHashComp.glsl");
	bitonicMergeCompShader = new ComputeShader("shader/PBD/bitonicMergeComp.glsl");
	bitonicSortCompShader = new ComputeShader("shader/PBD/bitonicSortComp.glsl");
	getStartEndCompShader = new ComputeShader("shader/PBD/getStartEndComp.glsl");
}

void PBD::init_buffers()
{
	glGenBuffers(1, &particleSSBO);
	glGenBuffers(1, &tableSSBO);
	glGenBuffers(1, &orderSSBO);
	glGenBuffers(1, &neighborSSBO);
	glGenBuffers(1, &countSSBO);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, particleSSBO);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(Particle) * particleNum, particles.data(), GL_DYNAMIC_DRAW);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, tableSSBO);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(glm::uvec2) * sortSize, nullptr, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, orderSSBO);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(glm::uvec2) * 2 * particleNum, nullptr, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, neighborSSBO);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(unsigned) * maxNeighbors * particleNum, nullptr, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, countSSBO);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(unsigned) * particleNum, nullptr, GL_DYNAMIC_DRAW);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	glGenBuffers(1, &neighborUBO);

	glBindBuffer(GL_UNIFORM_BUFFER, neighborUBO);
	glBufferData(GL_UNIFORM_BUFFER, 12, NULL, GL_DYNAMIC_DRAW);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, 4, &cellSize);
	glBufferSubData(GL_UNIFORM_BUFFER, 4, 4, &tableSize);
	glBufferSubData(GL_UNIFORM_BUFFER, 8, 4, &maxNeighbors);

	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	glGenTextures(1, &backgroundTex);
	glGenTextures(1, &backgroundDepthTex);

	glBindTexture(GL_TEXTURE_2D, backgroundTex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1600, 1200, 0, GL_RGBA, GL_FLOAT, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glBindTexture(GL_TEXTURE_2D, backgroundDepthTex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, 1600, 1200, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glBindTexture(GL_TEXTURE_2D, 0);

	glGenFramebuffers(1, &backgroundFBO);

	glBindFramebuffer(GL_FRAMEBUFFER, backgroundFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, backgroundDepthTex, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, backgroundTex, 0);
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		std::cerr << "Framebuffer error!" << std::endl;
		exit(-1);
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void PBD::update(float dt)
{
	//std::cout << glGetError() << " ";
	deltaTime = 0.015f;

	bind_buffers();

	predict_particle();

	neighborhood_search();

	solver();

	update_particle();
}

void PBD::draw(Shader& shader)
{
	glm::mat4 model(1.f);
	shader.use();
	shader.set_mat4("view", Global::mainCamera->view);
	shader.set_mat4("projection", Global::mainCamera->projection);

	glBindFramebuffer(GL_FRAMEBUFFER, backgroundFBO);
	glEnable(GL_CULL_FACE);
	glClearColor(0.45f, 0.55f, 0.60f, 1.00f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	for (auto& rb : rigidBodies)
	{
		model = rb->model;
		shader.set_mat4("model", model);
		rb->mesh->draw(shader);
	}

	/*for (auto& cloth : cloths)
	{
		model = glm::mat4(1.f);
		shader.set_mat4("model", model);
		cloth->mesh->draw(shader);
	}*/

	glDisable(GL_CULL_FACE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glActiveTexture(GL_TEXTURE6);
	glBindTexture(GL_TEXTURE_2D, backgroundDepthTex);
	glActiveTexture(GL_TEXTURE7);
	glBindTexture(GL_TEXTURE_2D, backgroundTex);

	model = glm::mat4(1.f);
	for (auto& fluid : fluids)
	{
		fluid->draw();
	}
}

void PBD::bind_buffers()
{
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, particleSSBO);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, tableSSBO);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, orderSSBO);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, neighborSSBO);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, countSSBO);

	glBindBufferBase(GL_UNIFORM_BUFFER, 0, neighborUBO);
}

void PBD::predict_particle()
{
	for (auto& rb : rigidBodies)
		rb->predict_position();

	for (auto& cloth : cloths)
		cloth->predict_position();

	for (auto& fluid : fluids)
		fluid->predict_position();
}

void PBD::neighborhood_search()
{
	spitalHashCompShader->use();
	spitalHashCompShader->set_uint("particleNum", particleNum);

	glDispatchCompute(sortSize / 1024, 1, 1);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
	
	bitonicMergeCompShader->use();
	bitonicMergeCompShader->set_uint("dataLength", sortSize);
	unsigned signLength = 2;
	while (signLength < sortSize)
	{
		unsigned compareLength = signLength / 2;
		bitonicMergeCompShader->set_uint("signLength", signLength);

		while (compareLength)
		{
			bitonicMergeCompShader->set_uint("compareLength", compareLength);
			glDispatchCompute(sortSize / 1024, 1, 1);
			glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
			compareLength /= 2;
		}

		signLength *= 2;
	}

	bitonicSortCompShader->use();
	bitonicSortCompShader->set_uint("dataLength", sortSize);
	unsigned compareN = sortSize / 2;
	while (compareN)
	{
		bitonicSortCompShader->set_uint("compareLength", compareN);
		glDispatchCompute(sortSize / 1024, 1, 1);
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

		compareN /= 2;
	}

	getStartEndCompShader->use();
	getStartEndCompShader->set_uint("particleNum", particleNum);

	glDispatchCompute(workGroupNum, 1, 1);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

	for (auto& rb : rigidBodies)
		rb->neighborhood_search();

	for (auto& cloth : cloths)
		cloth->neighborhood_search();

	for (auto& fluid : fluids)
		fluid->neighborhood_search();
}

void PBD::solver()
{
	for (auto& rb : rigidBodies)
	{
		rb->bind_buffers();
		rb->update_predict_position();

		for (unsigned i = 0; i < 2; i++)
		{
			rb->solve_environment_collision_constraint();
			rb->solve_particle_collision_constraint();
			rb->solve_shape_matching_constraint();
			rb->update_predict_position();
		}
	}

	for (auto& cloth : cloths)
	{
		cloth->bind_buffers();
		cloth->update_predict_position();

		for (unsigned i = 0; i < 5; i++)
		{
			cloth->solve_strech_constraint();
			cloth->update_predict_position();

			cloth->solve_attach_constraint();
			cloth->update_predict_position();

			cloth->solve_environment_collision_constraint();
			cloth->solve_particle_collision_constraint();
			cloth->update_predict_position();
		}
	}

	for (auto& fluid : fluids)
	{
		fluid->bind_buffers();
		fluid->update_predict_position();

		for (unsigned i = 0; i < 3; i++)
		{
			fluid->calculate_lambda();
			fluid->solve_incompressibility_constraint();
			fluid->solve_environment_collision_constraint();
			fluid->solve_particle_collision_constraint();
			fluid->update_predict_position();
		}
	}
}

void PBD::update_particle()
{
	for (auto& rb : rigidBodies)
	{
		rb->bind_buffers();

		rb->update_position();
	}

	for (auto& cloth : cloths)
	{
		cloth->bind_buffers();
		
		cloth->update_position();
	}

	for (auto& fluid : fluids)
	{
		fluid->bind_buffers();

		fluid->update_velocity();
		fluid->apply_vorticity_confinement();
		fluid->apply_XSPHV_viscosity();
		fluid->update_position();
	}
}
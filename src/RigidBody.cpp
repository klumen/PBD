#include "RigidBody.h"

ComputeShader* RigidBody::predictPositionCompShader = nullptr;
ComputeShader* RigidBody::neighborhoodSearchCompShader = nullptr;
ComputeShader* RigidBody::solveEnvironmentCollisionConstraintCompShader = nullptr;
ComputeShader* RigidBody::solveParticleCollisionConstraintCompShader = nullptr;
ComputeShader* RigidBody::calculateLocationCompShader = nullptr;
ComputeShader* RigidBody::calculateRotationCompShader = nullptr;
ComputeShader* RigidBody::solveShapeMatchingConstraintCompShader = nullptr;
ComputeShader* RigidBody::updatePredictPositionCompShader = nullptr;
ComputeShader* RigidBody::updatePositionCompShader = nullptr;

void RigidBody::create_shaders()
{
	predictPositionCompShader = new ComputeShader("shader/RigidBody/predictPositionComp.glsl");
	neighborhoodSearchCompShader = new ComputeShader("shader/RigidBody/neighborhoodSearchComp.glsl");
	solveEnvironmentCollisionConstraintCompShader = new ComputeShader("shader/RigidBody/solveEnvironmentCollisionConstraintComp.glsl");
	solveParticleCollisionConstraintCompShader = new ComputeShader("shader/RigidBody/solveParticleCollisionConstraintComp.glsl");
	calculateLocationCompShader = new ComputeShader("shader/RigidBody/calculateLocationComp.glsl");
	calculateRotationCompShader = new ComputeShader("shader/RigidBody/calculateRotationComp.glsl");
	solveShapeMatchingConstraintCompShader = new ComputeShader("shader/RigidBody/solveShapeMatchingConstraintComp.glsl");
	updatePredictPositionCompShader = new ComputeShader("shader/RigidBody/updatePredictPositionComp.glsl");
	updatePositionCompShader = new ComputeShader("shader/RigidBody/updatePositionComp.glsl");
}

RigidBody::RigidBody(Mesh* mesh) : mesh(mesh)
{
	if (!mesh)
		return;

	std::vector<glm::vec3> p;
	mesh->voxelize(Particle::radius * 2.f, p);
	SDF sdf(mesh);
	sdf.generate_SDF(p, SDFdata, SDFgrad);

	particleNr = static_cast<unsigned>(p.size());
	workGroupSize = 1024;
	workGroupNum = static_cast<unsigned>(ceil(particleNr * 1.f / workGroupSize));
	particles.resize(particleNr);
	float m = mass / particleNr;
	for (unsigned int i = 0; i < particleNr; i++)
	{
		particles[i].m = m;
		particles[i].f = particles[i].m * gravity;
		particles[i].w = 1.f / particles[i].m;
		particles[i].x = p[i];
		particles[i].v = glm::vec3(5.f, 2.f, 0.f);
		particles[i].corr = glm::vec3(0.f);
		particles[i].phase = 0;
		c0 += particles[i].m * particles[i].x;

		Particle::globalParticles.emplace_back(&particles[i]);
	}
	c0 /= mass;

	Particle::totPhase++;

	r.resize(particleNr);
	for (unsigned int i = 0; i < particleNr; i++)
	{
		r[i] = glm::vec4(particles[i].x - c0, 0.f);

		rr_inv[0][0] += particles[i].m * r[i].x * r[i].x;
		rr_inv[0][1] += particles[i].m * r[i].x * r[i].y;
		rr_inv[0][2] += particles[i].m * r[i].x * r[i].z;
		rr_inv[1][0] += particles[i].m * r[i].y * r[i].x;
		rr_inv[1][1] += particles[i].m * r[i].y * r[i].y;
		rr_inv[1][2] += particles[i].m * r[i].y * r[i].z;
		rr_inv[2][0] += particles[i].m * r[i].z * r[i].x;
		rr_inv[2][1] += particles[i].m * r[i].z * r[i].y;
		rr_inv[2][2] += particles[i].m * r[i].z * r[i].z;
	}
	rr_inv = glm::inverse(rr_inv);

	init_buffers();
}

RigidBody::~RigidBody()
{
	mesh = nullptr;

	glDeleteBuffers(1, &sdfBuffer);
	glDeleteBuffers(1, &locationSumBuffer);
	glDeleteBuffers(1, &rBuffer);
	glDeleteBuffers(1, &rotationSumBuffer);
}

void RigidBody::init_buffers()
{
	glGenBuffers(1, &sdfBuffer);
	glGenBuffers(1, &locationSumBuffer);
	glGenBuffers(1, &rBuffer);
	glGenBuffers(1, &rotationSumBuffer);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, sdfBuffer);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(glm::vec4) * particleNr, nullptr, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, locationSumBuffer);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(glm::vec4) * workGroupNum, nullptr, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, rBuffer);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(glm::vec4) * particleNr, r.data(), GL_DYNAMIC_DRAW);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, rotationSumBuffer);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(glm::mat4) * workGroupNum, nullptr, GL_DYNAMIC_DRAW);
	
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

}

void RigidBody::bind_buffers()
{
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, sdfBuffer);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, locationSumBuffer);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, rBuffer);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 8, rotationSumBuffer);
}

void RigidBody::predict_position()
{
	predictPositionCompShader->use();
	predictPositionCompShader->set_uint("head", head);
	predictPositionCompShader->set_uint("particleNum", particleNr);
	predictPositionCompShader->set_float("deltaTime", deltaTime);
	predictPositionCompShader->set_float("damping", damping);

	glDispatchCompute(workGroupNum, 1, 1);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
	predictPositionCompShader->close();
}

void RigidBody::neighborhood_search()
{
	neighborhoodSearchCompShader->use();
	neighborhoodSearchCompShader->set_uint("head", head);
	neighborhoodSearchCompShader->set_uint("particleNum", particleNr);
	neighborhoodSearchCompShader->set_float("neighborRadius", 2.f * Particle::radius);

	glDispatchCompute(workGroupNum, 1, 1);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
	neighborhoodSearchCompShader->close();
}

void RigidBody::solve_environment_collision_constraint()
{
	solveEnvironmentCollisionConstraintCompShader->use();
	solveEnvironmentCollisionConstraintCompShader->set_uint("head", head);
	solveEnvironmentCollisionConstraintCompShader->set_uint("particleNum", particleNr);
	solveEnvironmentCollisionConstraintCompShader->set_float("particleRadius", Particle::radius);
	solveEnvironmentCollisionConstraintCompShader->set_float("mu_s", mu_s);
	solveEnvironmentCollisionConstraintCompShader->set_float("mu_s", mu_k);

	glDispatchCompute(workGroupNum, 1, 1);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
	solveEnvironmentCollisionConstraintCompShader->close();
}

void RigidBody::solve_particle_collision_constraint()
{
	solveParticleCollisionConstraintCompShader->use();
	solveParticleCollisionConstraintCompShader->set_uint("head", head);
	solveParticleCollisionConstraintCompShader->set_uint("particleNum", particleNr);
	solveParticleCollisionConstraintCompShader->set_float("radius", 2.f * Particle::radius);
	solveParticleCollisionConstraintCompShader->set_float("mu_s", mu_s);
	solveParticleCollisionConstraintCompShader->set_float("mu_s", mu_k);

	glDispatchCompute(workGroupNum, 1, 1);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
	solveParticleCollisionConstraintCompShader->close();
}

void RigidBody::solve_shape_matching_constraint()
{
	calculateLocationCompShader->use();
	calculateLocationCompShader->set_uint("head", head);
	calculateLocationCompShader->set_uint("particleNum", particleNr);

	glDispatchCompute(workGroupNum, 1, 1);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
	calculateLocationCompShader->close();

	cm = glm::vec3(0.f);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, locationSumBuffer);
	glm::vec4* readc = reinterpret_cast<glm::vec4*>(glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY));
	if (readc == nullptr)
	{
		std::cerr << "read error!" << std::endl;
	}
	else
	{
		for (unsigned i = 0; i < workGroupNum; i++)
		{
			cm += glm::vec3(readc[i]);
		}
	}
	if (!glUnmapBuffer(GL_SHADER_STORAGE_BUFFER))
		std::cerr << "unMap error" << std::endl;
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
	cm /= mass;

	calculateRotationCompShader->use();
	calculateRotationCompShader->set_uint("head", head);
	calculateRotationCompShader->set_uint("particleNum", particleNr);
	calculateRotationCompShader->set_vec3("location", cm);

	glDispatchCompute(workGroupNum, 1, 1);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
	calculateRotationCompShader->close();

	glm::mat3 A(0.f);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, rotationSumBuffer);
	glm::mat4* readR = reinterpret_cast<glm::mat4*>(glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY));
	if (readR == nullptr)
	{
		std::cerr << "read error!" << std::endl;
	}
	else
	{
		for (unsigned i = 0; i < workGroupNum; i++)
		{
			A += glm::mat3(readR[i]);
		}
	}
	if (!glUnmapBuffer(GL_SHADER_STORAGE_BUFFER))
		std::cerr << "unMap error" << std::endl;
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
	R = glm::transpose(Math::polar_decomposition(rr_inv * A));

	solveShapeMatchingConstraintCompShader->use();
	solveShapeMatchingConstraintCompShader->set_uint("head", head);
	solveShapeMatchingConstraintCompShader->set_uint("particleNum", particleNr);
	solveShapeMatchingConstraintCompShader->set_vec3("location", cm);
	solveShapeMatchingConstraintCompShader->set_mat3("rotation", R);

	glDispatchCompute(workGroupNum, 1, 1);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
	solveShapeMatchingConstraintCompShader->close();
}

void RigidBody::update_predict_position()
{
	updatePredictPositionCompShader->use();
	updatePredictPositionCompShader->set_uint("head", head);
	updatePredictPositionCompShader->set_uint("particleNum", particleNr);

	glDispatchCompute(workGroupNum, 1, 1);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
	updatePredictPositionCompShader->close();
}

void RigidBody::update_position()
{
	updatePositionCompShader->use();
	updatePositionCompShader->set_uint("head", head);
	updatePositionCompShader->set_uint("particleNum", particleNr);
	updatePositionCompShader->set_float("deltaTime", deltaTime);

	glDispatchCompute(workGroupNum, 1, 1);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
	updatePositionCompShader->close();

	model = glm::mat4(1.f);
	model = glm::translate(model, cm);
	model = model * glm::mat4(R);
	model = glm::translate(model, -c0);
}
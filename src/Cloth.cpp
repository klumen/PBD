#include "Cloth.h"

ComputeShader* Cloth::predictPositionCompShader = nullptr;
ComputeShader* Cloth::neighborhoodSearchCompShader = nullptr;
ComputeShader* Cloth::solveStrechConstraintCompShader = nullptr;
ComputeShader* Cloth::solveEnvironmentCollisionConstraintCompShader = nullptr;
ComputeShader* Cloth::solveParticleCollisionConstraintCompShader = nullptr;
ComputeShader* Cloth::solveAttachConstraintCompShader = nullptr;
ComputeShader* Cloth::updatePredictPositionCompShader = nullptr;
ComputeShader* Cloth::updatePositionCompShader = nullptr;

void Cloth::create_shaders()
{
	predictPositionCompShader = new ComputeShader("shader/Cloth/predictPositionComp.glsl");
	neighborhoodSearchCompShader = new ComputeShader("shader/Cloth/neighborhoodSearchComp.glsl");
	solveStrechConstraintCompShader = new ComputeShader("shader/Cloth/solveStrechConstraintComp.glsl");
	solveEnvironmentCollisionConstraintCompShader = new ComputeShader("shader/Cloth/solveEnvironmentCollisionConstraintComp.glsl");
	solveParticleCollisionConstraintCompShader = new ComputeShader("shader/Cloth/solveParticleCollisionConstraintComp.glsl");
	solveAttachConstraintCompShader = new ComputeShader("shader/Cloth/solveAttachConstraintComp.glsl");
	updatePredictPositionCompShader = new ComputeShader("shader/Cloth/updatePredictPositionComp.glsl");
	updatePositionCompShader = new ComputeShader("shader/Cloth/updatePositionComp.glsl");
}

Cloth::Cloth(Mesh* mesh) : mesh(mesh)
{
	if (!mesh)
		return;

	particleNr = static_cast<unsigned>(mesh->vertices.size());
	workGroupSize = 1024;
	workGroupNum = static_cast<unsigned>(ceil(particleNr * 1.f / workGroupSize));
	particles.resize(particleNr);
	generate_mass();

	for (unsigned int i = 0; i < particleNr; i++)
	{
		particles[i].f = particles[i].m * gravity;
		particles[i].w = 1.f / particles[i].m;
		particles[i].x = mesh->vertices[i].position;
		particles[i].v = glm::vec3(0.f);
		particles[i].pred = glm::vec3(0.f);
		particles[i].phase = 2;
		
		if (mesh->vertices[i].position.z == -1.f)
		{
			if (mesh->vertices[i].position.x == 1.f)
			{
				particles[i].w = 0.f;
				attaId.emplace_back(i);
				atta.emplace_back(mesh->vertices[i].position);
			}

			if (mesh->vertices[i].position.x == -1.f)
			{
				particles[i].w = 0.f;
				attaId.emplace_back(i);
				atta.emplace_back(mesh->vertices[i].position);
			}
		}
	}

	std::vector<glm::uvec3> triple(mesh->indices.size());
	for (unsigned int i = 0; i < triple.size() / 3; i++)
	{
		triple[3 * i + 0] = glm::uvec3(mesh->indices[3 * i + 0], mesh->indices[3 * i + 1], i);
		triple[3 * i + 1] = glm::uvec3(mesh->indices[3 * i + 1], mesh->indices[3 * i + 2], i);
		triple[3 * i + 2] = glm::uvec3(mesh->indices[3 * i + 0], mesh->indices[3 * i + 2], i);
	}
	for (auto& t : triple)
		if (t[0] > t[1])
			std::swap(t[0], t[1]);

	std::sort(triple.begin(), triple.end(), [](const glm::uvec3& a, const glm::uvec3& b) {
		for (unsigned int i = 0; i < 3; i++)
		{
			if (a[i] < b[i])
				return true;
			else if (a[i] > b[i])
				return false;
		}

		return false;
		});

	edge.emplace_back(glm::uvec2(triple[0][0], triple[0][1]));
	std::vector<glm::uvec2> nbrTri;
	for (unsigned int i = 1; i < triple.size(); i++)
	{
		if (triple[i - 1][0] == triple[i][0] && triple[i - 1][1] == triple[i][1])
		{
			nbrTri.emplace_back(glm::uvec2(triple[i - 1][2], triple[i][2]));
			continue;
		}
		edge.emplace_back(glm::uvec2(triple[i][0], triple[i][1]));
	}

	for (auto& nbr : nbrTri)
	{
		unsigned int s0 = 3 * nbr[0];
		unsigned int s1 = 3 * nbr[1];

		std::set<unsigned int> id0 = { mesh->indices[s0], mesh->indices[s0 + 1], mesh->indices[s0 + 2] };
		std::set<unsigned int> id1 = { mesh->indices[s1], mesh->indices[s1 + 1], mesh->indices[s1 + 2] };
		std::set<unsigned int> unionSet;
		std::set<unsigned int> intersectionSet;
		std::set<unsigned int> res;
		std::set_union(id0.begin(), id0.end(), id1.begin(), id1.end(), std::inserter(unionSet, unionSet.begin()));
		std::set_intersection(id0.begin(), id0.end(), id1.begin(), id1.end(), std::inserter(intersectionSet, intersectionSet.begin()));
		std::set_difference(unionSet.begin(), unionSet.end(), intersectionSet.begin(), intersectionSet.end(), std::inserter(res, res.begin()));
		if (res.size() != 2)
			std::cerr << "CLOTH::CLOTH::ERROR" << std::endl;

		auto it0 = res.begin();
		unsigned int u = *it0;
		unsigned int v = *(++it0);
		edge.emplace_back(u, v);

		auto it1 = intersectionSet.begin();
		unsigned int u1 = *it1;
		unsigned int v1 = *(++it1);
		nbrInd.emplace_back(u1, v1, u, v);
	}
	
	length.resize(edge.size());
	for (unsigned int i = 0; i < length.size(); i++)
	{
		glm::vec3 l = mesh->vertices[edge[i][0]].position - mesh->vertices[edge[i][1]].position;
		length[i] = sqrtf(glm::dot(l, l));
	}

	maxAttaments = 2;
	r.resize(maxAttaments * particleNr);
	for (unsigned i = 0; i < atta.size(); i++)
		for (unsigned j = 0; j < particleNr; j++)
			r[i * particleNr + j] = glm::distance(atta[i], particles[j].x);

	init_buffers();
}

Cloth::~Cloth()
{
	mesh = nullptr;

	glDeleteBuffers(1, &edgeSSBO);
	glDeleteBuffers(1, &edgeLenSSBO);
	glDeleteBuffers(1, &attachSSBO);
	glDeleteBuffers(1, &nSSBO);
	glDeleteBuffers(1, &rSSBO);
}

void Cloth::generate_mass()
{
	for (unsigned int i = 0; i < mesh->indices.size(); i += 3)
	{
		unsigned int j[3] = { mesh->indices[i] , mesh->indices[i + 1], mesh->indices[i + 2] };

		const glm::vec3& v0 = mesh->vertices[j[0]].position;
		const glm::vec3& v1 = mesh->vertices[j[1]].position;
		const glm::vec3& v2 = mesh->vertices[j[2]].position;

		glm::vec3 n = glm::cross(v1 - v0, v2 - v0);
		float area = sqrtf(glm::dot(n, n)) / 2.f;
		float mass = area * rho / 3.f;
		
		particles[j[0]].m += mass;
		particles[j[1]].m += mass;
		particles[j[2]].m += mass;
	}
}

void Cloth::init_buffers()
{
	struct Attach
	{
		glm::vec3 position;
		unsigned id;
	};
	std::vector<Attach> attach(attaId.size());
	for (unsigned i = 0; i < attach.size(); i++)
	{
		attach[i].position = atta[i];
		attach[i].id = attaId[i];
	}

	glGenBuffers(1, &edgeSSBO);
	glGenBuffers(1, &edgeLenSSBO);
	glGenBuffers(1, &attachSSBO);
	glGenBuffers(1, &nSSBO);
	glGenBuffers(1, &rSSBO);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, edgeSSBO);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(glm::uvec2) * edge.size(), edge.data(), GL_STATIC_DRAW);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, edgeLenSSBO);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(float) * length.size(), length.data(), GL_STATIC_DRAW);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, attachSSBO);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(Attach) * attach.size(), attach.data(), GL_STATIC_DRAW);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, nSSBO);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(unsigned) * particleNr, nullptr, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, rSSBO);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(float) * maxAttaments * particleNr, r.data(), GL_STATIC_DRAW);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void Cloth::bind_buffers()
{
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, edgeSSBO);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, edgeLenSSBO);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, attachSSBO);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 8, nSSBO);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 9, rSSBO);
}

void Cloth::predict_position()
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

void Cloth::neighborhood_search()
{
	neighborhoodSearchCompShader->use();
	neighborhoodSearchCompShader->set_uint("head", head);
	neighborhoodSearchCompShader->set_uint("particleNum", particleNr);
	neighborhoodSearchCompShader->set_float("neighborRadius", 2.f * Particle::radius);

	glDispatchCompute(workGroupNum, 1, 1);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
	neighborhoodSearchCompShader->close();
}

void Cloth::solve_strech_constraint()
{
	unsigned works = static_cast<unsigned>(ceil(edge.size() * 1.f / workGroupSize));

	solveStrechConstraintCompShader->use();
	solveStrechConstraintCompShader->set_uint("head", head);
	solveStrechConstraintCompShader->set_uint("edgeNum", static_cast<unsigned>(edge.size()));

	glDispatchCompute(works, 1, 1);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
	solveStrechConstraintCompShader->close();
}

void Cloth::solve_attach_constraint()
{
	solveAttachConstraintCompShader->use();
	solveAttachConstraintCompShader->set_uint("head", head);
	solveAttachConstraintCompShader->set_uint("particleNum", particleNr);
	solveAttachConstraintCompShader->set_uint("attachNum", static_cast<unsigned>(atta.size()));

	glDispatchCompute(workGroupNum, 1, 1);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
	updatePredictPositionCompShader->close();
}

void Cloth::solve_environment_collision_constraint()
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

void Cloth::solve_particle_collision_constraint()
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

void Cloth::update_predict_position()
{
	updatePredictPositionCompShader->use();
	updatePredictPositionCompShader->set_uint("head", head);
	updatePredictPositionCompShader->set_uint("particleNum", particleNr);

	glDispatchCompute(workGroupNum, 1, 1);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
	updatePredictPositionCompShader->close();
}

void Cloth::update_position()
{
	updatePositionCompShader->use();
	updatePositionCompShader->set_uint("head", head);
	updatePositionCompShader->set_uint("particleNum", particleNr);
	updatePositionCompShader->set_float("deltaTime", deltaTime);

	glDispatchCompute(workGroupNum, 1, 1);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
	updatePositionCompShader->close();
}

/*void Cloth::bend_constraints(std::vector<glm::vec3>& p, unsigned int time)
{
	for (unsigned int i = 0; i < nbrInd.size(); i++)
	{
		unsigned int v[4]{};
		v[0] = nbrInd[i][0];
		v[1] = nbrInd[i][1];
		v[2] = nbrInd[i][2];
		v[3] = nbrInd[i][3];

		glm::vec3 p1 = p[v[0]];
		glm::vec3 p2 = p[v[1]] - p1;
		glm::vec3 p3 = p[v[2]] - p1;
		glm::vec3 p4 = p[v[3]] - p1;

		glm::vec3 n1 = glm::cross(p2, p3), n2 = glm::cross(p2, p4);
		float l1 = sqrtf(glm::dot(n1, n1)), l2 = sqrtf(glm::dot(n2, n2));
		n1 /= l1, n2 /= l2;
		float d = glm::dot(n1, n2);

		glm::vec3 q[4]{};
		q[2] = (glm::cross(p2, n2) + glm::cross(n1, p2) * d) / l1;
		q[3] = (glm::cross(p2, n1) + glm::cross(n2, p2) * d) / l2;
		q[1] = -(glm::cross(p3, n2) + glm::cross(n1, p3) * d) / l1 - (glm::cross(p4, n1) + glm::cross(n2, p4) * d) / l2;
		q[0] = -q[1] - q[2] - q[3];

		float lambda = 0.f;
		for (unsigned int j = 0; j < 4; j++)
			lambda += particles[v[j]].w * glm::dot(q[j], q[j]);
		lambda = (sqrtf(1.f - d * d) * (acosf(d) - acosf(-1.f))) / lambda;

		if (std::isnan(lambda))
			continue;

		float k = 1.f - powf(1.f - k_bending, 1.f / time);

		for (unsigned int j = 0; j < 4; j++)
			p[v[j]] -= k * (particles[j].w * lambda * q[j]);
	}
}*/

/*void Cloth::smooth_v()
{
	std::vector<glm::vec3> v_sum(particleNr, glm::vec3(0.f));
	std::vector<float> v_num(particleNr, 0);

	for (unsigned int i = 0; i < mesh->indices.size(); i += 3)
	{
		unsigned int v0 = mesh->indices[i];
		unsigned int v1 = mesh->indices[i + 1];
		unsigned int v2 = mesh->indices[i + 2];
		glm::vec3 sum = particles[v0].v + particles[v1].v + particles[v2].v;

		v_sum[v0] += sum;
		v_sum[v1] += sum;
		v_sum[v2] += sum;

		v_num[v0] += 3.f;
		v_num[v1] += 3.f;
		v_num[v2] += 3.f;
	}

	for (unsigned int i = 0; i < particleNr; i += 3)
		particles[i].v = 0.9f * particles[i].v + 0.1f * v_sum[i] / v_num[i];
}*/
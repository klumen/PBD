#include "SoftBody.h"

SoftBody::SoftBody(Mesh* mesh) : mesh(mesh)
{
	if (!mesh)
		return;

	std::vector<glm::vec3> p;
	mesh->voxelize(Particle::radius * 2.f, p);
	//mesh->surface_voxelize(Particle::radius * 2.f, p);
	SDF sdf(mesh);
	sdf.generate_SDF(p, SDFdata, gradientSDF);

	particleNr = p.size();
	particles.resize(particleNr);
	float m = mass / particleNr;
	glm::vec3 c(0.f);
	for (unsigned int i = 0; i < particleNr; i++)
	{
		particles[i].m = m;
		particles[i].w = 1.f / particles[i].m;
		particles[i].x = p[i];
		particles[i].v = glm::vec3(5.f, 2.f, 0.f);
		particles[i].phase = Particle::totPhase;
		c += particles[i].m * particles[i].x;

		Particle::globalParticles.emplace_back(&particles[i]);
	}
	c /= mass;

	r0.resize(mesh->vertices.size());
	for (unsigned i = 0; i < mesh->vertices.size(); i++)
		r0[i] = mesh->vertices[i].position - c;

	Particle::totPhase++;

	r.resize(particleNr);
	for (unsigned int i = 0; i < particleNr; i++)
	{
		r[i] = particles[i].x - c;

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
}

SoftBody::~SoftBody()
{
	mesh = nullptr;
}

void SoftBody::damping_v()
{
	glm::vec3 xc(0.f);
	glm::vec3 vc(0.f);

	for (unsigned int i = 0; i < particleNr; i++)
	{
		xc += particles[i].m * particles[i].x;
		vc += particles[i].m * particles[i].v;
	}
	xc /= mass;
	vc /= mass;

	glm::vec3 L(0.f);
	glm::mat3 I(0.f);

	for (unsigned int i = 0; i < particleNr; i++)
	{
		L += glm::cross(r[i], particles[i].m * particles[i].v);

		glm::mat3 R = Math::get_cross_matrix(r[i]);
		I += glm::transpose(R) * R * particles[i].m;
	}

	glm::vec3 w = L * glm::inverse(I);

	for (unsigned int i = 0; i < particleNr; i++)
	{
		glm::vec3 dv = vc - particles[i].v + glm::cross(w, r[i]);
		particles[i].v += damping * dv;
	}
}

void SoftBody::mesh_contact_constrain(SoftBody& sb)
{
	float mu_s = 0.6f;
	float mu_k = 0.5f;

	glm::vec3 P(0.f, -1.f, 0.f);
	glm::vec3 N(0.f, 1.f, 0.f);

	for (unsigned int i = 0; i < sb.particleNr; i++)
	{
		glm::vec3 X_i = sb.particles[i].pred - P;
		if (glm::dot(X_i, N) < 0.f)
		{
			float d = std::abs(glm::dot(X_i, N));
			sb.particles[i].corr += N * d;
			glm::vec3 dx = sb.particles[i].pred + sb.particles[i].corr - sb.particles[i].x;
			glm::vec3 dx_t = dx - glm::dot(dx, N) * N;
			float L_dx = sqrtf(glm::dot(dx_t, dx_t));

			if (L_dx < mu_s * d)
				sb.particles[i].corr -= dx_t;
			else
				sb.particles[i].corr -= dx_t * std::min(1.f, mu_k * d / L_dx);
		}
	}

	P = glm::vec3(1.f, 0.f, 0.f);
	N = glm::vec3(-1.f, 0.f, 0.f);

	for (unsigned int i = 0; i < sb.particleNr; i++)
	{
		glm::vec3 X_i = sb.particles[i].pred - P;
		if (glm::dot(X_i, N) < 0.f)
		{
			float d = std::abs(glm::dot(X_i, N));
			sb.particles[i].corr += N * d;
			glm::vec3 dx = sb.particles[i].pred + sb.particles[i].corr - sb.particles[i].x;
			glm::vec3 dx_t = dx - glm::dot(dx, N) * N;
			float L_dx = sqrtf(glm::dot(dx_t, dx_t));

			if (L_dx < mu_s * d)
				sb.particles[i].corr -= dx_t;
			else
				sb.particles[i].corr -= dx_t * std::min(1.f, mu_k * d / L_dx);
		}
	}
}

void SoftBody::particle_contact_constrain(SoftBody& sb)
{
	float mu_s = 0.6f;
	float mu_k = 0.5f;

	for (unsigned int i = 0; i < sb.particleNr; i++)
	{
		for (auto& particle : Particle::globalParticles)
		{
			if (particle->phase == sb.particles[i].phase)
				continue;

			glm::vec3 xij = particle->pred - sb.particles[i].pred;
			float d = Particle::radius - sqrtf(glm::dot(xij, xij));

			if (d <= 0.f)
				continue;

			glm::vec3 nij = sb.gradientSDF[i];
			if (glm::dot(xij, nij) < 0.f)
				nij = xij - 2.f * glm::dot(xij, nij) * nij;
			else
				nij = xij;
			nij = glm::normalize(nij);
			
			sb.particles[i].corr += -sb.particles[i].w / (sb.particles[i].w + particle->w) * d * nij;
			particle->corr += sb.particles[i].w / (sb.particles[i].w + particle->w) * d * nij;

			glm::vec3 dx = (sb.particles[i].pred + sb.particles[i].corr - sb.particles[i].x)
				- (particle->pred + particle->corr - particle->x);
			glm::vec3 dx_t = dx - glm::dot(dx, nij) * nij;
			float L_dx = sqrtf(glm::dot(dx_t, dx_t));

			if (L_dx < mu_s * d)
			{
				dx = dx_t;
				sb.particles[i].corr -= sb.particles[i].w / (sb.particles[i].w + particle->w) * dx;
				particle->corr += particle->w / (sb.particles[i].w + particle->w) * dx;
			}
			else
			{
				dx = dx_t * std::min(1.f, mu_k * d / L_dx);
				sb.particles[i].corr -= sb.particles[i].w / (sb.particles[i].w + particle->w) * dx;
				particle->corr += particle->w / (sb.particles[i].w + particle->w) * dx;
			}
		}
	}
}

void SoftBody::shape_matching_constraint(SoftBody& sb)
{
	glm::vec3 c(0.f);
	for (unsigned int i = 0; i < sb.particleNr; i++)
		c += sb.particles[i].m * sb.particles[i].pred;
	c /= sb.mass;

	glm::mat3 A(0.f);
	for (unsigned int i = 0; i < sb.particleNr; i++)
	{
		A[0][0] += sb.particles[i].m * (sb.particles[i].pred - c)[0] * sb.r[i][0];
		A[0][1] += sb.particles[i].m * (sb.particles[i].pred - c)[0] * sb.r[i][1];
		A[0][2] += sb.particles[i].m * (sb.particles[i].pred - c)[0] * sb.r[i][2];
		A[1][0] += sb.particles[i].m * (sb.particles[i].pred - c)[1] * sb.r[i][0];
		A[1][1] += sb.particles[i].m * (sb.particles[i].pred - c)[1] * sb.r[i][1];
		A[1][2] += sb.particles[i].m * (sb.particles[i].pred - c)[1] * sb.r[i][2];
		A[2][0] += sb.particles[i].m * (sb.particles[i].pred - c)[2] * sb.r[i][0];
		A[2][1] += sb.particles[i].m * (sb.particles[i].pred - c)[2] * sb.r[i][1];
		A[2][2] += sb.particles[i].m * (sb.particles[i].pred - c)[2] * sb.r[i][2];
	}
	A = sb.rr_inv * A;
	glm::mat3 R = Math::polar_decomposition(A);

	for (unsigned int i = 0; i < sb.particleNr; i++)
	{
		glm::vec3 g = sb.r[i] * R + c;
		sb.particles[i].corr = sb.stiffness * (g - sb.particles[i].pred);
	}

	sb.cm0 = c;
	sb.R0 = R;
}
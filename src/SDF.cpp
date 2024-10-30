#include "SDF.h"

SDF::SDF(Mesh* mesh) : mesh(mesh)
{
	if (!mesh)
		return;

	init_angle_weighted_pseudo_normal();
}

SDF::~SDF()
{
	mesh = nullptr;
}

void SDF::init_angle_weighted_pseudo_normal()
{
	if (mesh->indices.empty())
		return;

	faceNormal.resize(mesh->indices.size() / 3);
	vertNormal.resize(mesh->vertices.size(), glm::vec3(0.f));

	for (unsigned int i = 0; i < faceNormal.size(); ++i)
	{
		unsigned int v[3];
		v[0] = mesh->indices[i * 3 + 0];
		v[1] = mesh->indices[i * 3 + 1];
		v[2] = mesh->indices[i * 3 + 2];

		glm::vec3 edge10 = mesh->vertices[v[1]].position - mesh->vertices[v[0]].position;
		glm::vec3 edge20 = mesh->vertices[v[2]].position - mesh->vertices[v[0]].position;

		faceNormal[i] = glm::normalize(glm::cross(edge10, edge20));
	}

	for (unsigned int i = 0; i < faceNormal.size(); ++i)
	{
		unsigned int v[3];
		v[0] = mesh->indices[i * 3 + 0];
		v[1] = mesh->indices[i * 3 + 1];
		v[2] = mesh->indices[i * 3 + 2];

		glm::vec3 edge10 = mesh->vertices[v[1]].position - mesh->vertices[v[0]].position;
		glm::vec3 edge20 = mesh->vertices[v[2]].position - mesh->vertices[v[0]].position;
		glm::vec3 edge21 = mesh->vertices[v[2]].position - mesh->vertices[v[1]].position;
		float mag10 = sqrtf(glm::dot(edge10, edge10));
		float mag20 = sqrtf(glm::dot(edge20, edge20));
		float mag21 = sqrtf(glm::dot(edge21, edge21));

		float alpha0 = acosf(glm::dot(edge10, edge20) / (mag10 * mag20));
		float alpha1 = acosf(glm::dot(edge21, -edge10) / (mag10 * mag21));
		float alpha2 = acosf(glm::dot(-edge20, -edge21) / (mag20 * mag21));
		vertNormal[v[0]] += alpha0 * faceNormal[i];
		vertNormal[v[1]] += alpha1 * faceNormal[i];
		vertNormal[v[2]] += alpha2 * faceNormal[i];

		for (unsigned j = 0; j < 3; j++)
		{
			unsigned int v1 = v[j];
			unsigned int v2 = v[(j + 1) % 3];

			auto edge1 = std::make_pair(v1, v2);
			auto edge2 = std::make_pair(v2, v1);

			if (edgeNormal.find(edge1) == edgeNormal.end())
				edgeNormal[edge1] = glm::vec3(0.f);
			if (edgeNormal.find(edge2) == edgeNormal.end())
				edgeNormal[edge2] = glm::vec3(0.f);

			edgeNormal[edge1] += faceNormal[i];
			edgeNormal[edge2] += faceNormal[i];
		}
	}

	for (auto& normal : edgeNormal)
		normal.second = glm::normalize(normal.second);

	for (auto& normal : vertNormal)
		normal = glm::normalize(normal);
}

void SDF::generate_SDF(const std::vector<glm::vec3>& particles, std::vector<float>& data, std::vector<glm::vec3>& gradient)
{
	data.resize(particles.size());
	gradient.resize(particles.size());

	for (unsigned i = 0; i < particles.size(); i++)
	{
		glm::vec3 closestP;
		float mind2 = std::numeric_limits<float>::max();
		bool inside = true;

		for (unsigned j = 0; j < faceNormal.size(); j++)
		{
			unsigned int v[3];
			v[0] = mesh->indices[j * 3 + 0];
			v[1] = mesh->indices[j * 3 + 1];
			v[2] = mesh->indices[j * 3 + 2];

			bool tempInside = true;

			glm::vec3 tempP = get_closest_point_on_triangle(particles[i], faceNormal[j], v, tempInside);

			float dis2 = glm::dot(particles[i] - tempP, particles[i] - tempP);
			if (dis2 < mind2)
			{
				mind2 = dis2;
				closestP = tempP;
				inside = tempInside;
			}
		}

		if (inside)
		{
			data[i] = -sqrtf(mind2);
			gradient[i] = closestP - particles[i];
		}
		else
		{
			data[i] = sqrtf(mind2);
			gradient[i] = particles[i] - closestP;
		}
	}
}

glm::vec3 SDF::get_closest_point_on_triangle(glm::vec3 p, glm::vec3 n, unsigned v[3], bool& inside) const
{
	const glm::vec3 point = p;

	p -= glm::dot(p - mesh->vertices[v[0]].position, n) * n;
	const glm::vec3& a = mesh->vertices[v[0]].position;
	const glm::vec3& b = mesh->vertices[v[1]].position;
	const glm::vec3& c = mesh->vertices[v[2]].position;

	glm::vec3 ab = b - a;
	glm::vec3 ac = c - a;
	glm::vec3 bc = c - b;

	float u0 = glm::dot(p - a, ab), u1 = glm::dot(p - b, -ab);
	float v0 = glm::dot(p - a, ac), v1 = glm::dot(p - c, -ac);

	if (u0 <= 0.f && v0 <= 0.f)
	{
		if (glm::dot(point - a, vertNormal[v[0]]) > 0.f)
			inside = false;
		else
			inside = true;

		return a;
	}

	float w0 = glm::dot(p - b, bc), w1 = glm::dot(p - c, -bc);

	if (u1 <= 0.f && w0 <= 0.f)
	{
		if (glm::dot(point - b, vertNormal[v[0]]) > 0.f)
			inside = false;
		else
			inside = true;

		return b;
	}

	if (v1 <= 0.f && w1 <= 0.f)
	{
		if (glm::dot(point - c, vertNormal[v[0]]) > 0.f)
			inside = false;
		else
			inside = true;

		return c;
	}

	n = glm::cross(ab, ac);

	float vc = glm::dot(n, glm::cross(a - p, b - p));
	if (vc <= 0.f && u0 >= 0.f && u1 >= 0.f)
	{
		glm::vec3 x = a + u0 / (u0 + u1) * ab;
		if (glm::dot(point - x, vertNormal[v[0]]) > 0.f)
			inside = false;
		else
			inside = true;

		return x;
	}
		

	float va = glm::dot(n, glm::cross(b - p, c - p));
	if (va <= 0.f && w0 >= 0.f && w1 >= 0.f)
	{
		glm::vec3 x = a + w0 / (w0 + w1) * bc;
		if (glm::dot(point - x, vertNormal[v[0]]) > 0.f)
			inside = false;
		else
			inside = true;

		return x;
	}

	float vb = glm::dot(n, glm::cross(c - p, a - p));
	if (vb <= 0.f && v0 >= 0.f && v1 >= 0.f)
	{
		glm::vec3 x = a + v0 / (v0 + v1) * ac;
		if (glm::dot(point - x, vertNormal[v[0]]) > 0.f)
			inside = false;
		else
			inside = true;

		return x;
	}

	float alpha = va / (va + vb + vc);
	float beta = vb / (va + vb + vc);
	float gamma = 1.0f - alpha - beta;

	glm::vec3 x = alpha * a + beta * b + gamma * c;
	if (glm::dot(point - x, vertNormal[v[0]]) > 0.f)
		inside = false;
	else
		inside = true;

	return x;
	// optimize
	/*
	// Check if P in vertex region outside A
	Vector ab= b- a;
	Vector ac= c- a;
	Vector ap= p- a;
	float d1 = Dot(ab, ap);
	float d2 = Dot(ac, ap);
	if (d1 <= 0.0f && d2 <= 0.0f) return a; // barycentric coordinates (1,0,0)
	// Check if P in vertex region outside B
	Vector bp= p- b;
	float d3 = Dot(ab, bp);
	float d4 = Dot(ac, bp);
	if (d3 >= 0.0f && d4 <= d3) return b; // barycentric coordinates (0,1,0)
	// Check if P in edge region of AB, if so return projection of P onto AB
	float vc = d1*d4- d3*d2;
	if (vc <= 0.0f && d1 >= 0.0f && d3 <= 0.0f) {
	float v = d1 / (d1-d3);
	return a + v * ab; // barycentric coordinates (1-v,v,0)
	}
	// Check if P in vertex region outside C
	Vector cp = p- c;
	float d5 = Dot(ab, cp);
	float d6 = Dot(ac, cp);
	if (d6 >= 0.0f && d5 <= d6) return c; // barycentric coordinates (0,0,1)
	// Check if P in edge region of AC, if so return projection of P onto AC
	float vb = d5*d2- d1*d6;
	if (vb <= 0.0f && d2 >= 0.0f && d6 <= 0.0f) {
	float w = d2 / (d2-d6);
	return a + w * ac; // barycentric coordinates (1-w,0,w)
	}
	// Check if P in edge region of BC, if so return projection of P onto BC
	float va = d3*d6- d5*d4;
	if (va <= 0.0f && (d4- d3) >= 0.0f && (d5- d6) >= 0.0f) {
	float w = (d4- d3) / ((d4- d3) + (d5- d6));
	return b + w * (c- b); // barycentric coordinates (0,1-w,w)
	}
	// P inside face region. Compute Q through its barycentric coordinates (u,v,w)
	float denom = 1.0f / (va + vb + vc);
	floatv = vb * denom;
	floatw = vc * denom;
	returna + ab * v + ac * w; // = u*a +v*b+w*c,u=va* denom = 1.0f-v-w
	}
	*/
}
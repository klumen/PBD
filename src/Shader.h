#ifndef SHADER_H
#define SHADER_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

class Shader
{
public:
	Shader();
	virtual ~Shader() = 0;

	void use() const;
	void close() const;

	void set_bool(const std::string& name, bool value) const;
	void set_int(const std::string& name, int value) const;
	void set_uint(const std::string& name, unsigned value) const;
	void set_float(const std::string& name, float value) const;
	void set_vec2(const std::string& name, const glm::vec2& value) const;
	void set_vec2(const std::string& name, float x, float y) const;
	void set_vec3(const std::string& name, const glm::vec3& value) const;
	void set_vec3(const std::string& name, float x, float y, float z) const;
	void set_vec4(const std::string& name, const glm::vec4& value) const;
	void set_vec4(const std::string& name, float x, float y, float z, float w) const;
	void set_mat2(const std::string& name, const glm::mat2& value) const;
	void set_mat3(const std::string& name, const glm::mat3& value) const;
	void set_mat4(const std::string& name, const glm::mat4& value) const;

	unsigned int ID;

private:

};

class PipelineShader : public Shader
{
public:
	PipelineShader(const char* vertexPath, const char* fragmentPath, const char* geometryPath = nullptr);
	~PipelineShader() = default;

private:

};

class ComputeShader : public Shader
{
public:
	ComputeShader(const std::string& computePath);
	~ComputeShader() = default;

private:

};

#endif // !SHADER_H
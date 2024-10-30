#ifndef GLOBAL_H
#define GLOBAL_H

#include <glad/glad.h>

#include "Shader.h"
#include "Camera.h"
#include "Light.h"

class Global
{
public:
	Global() = delete;
	~Global() = delete;

	static void init();

	static unsigned screenWidth, screenHeight;

	static float lastFrame, deltaTime;

	static float screenQuadVertices[];
	static unsigned screenQuadVAO;
	static unsigned screenQuadVBO;
	static void init_screen_quad();

	static PipelineShader* textureShader;
	static void init_texture_shader();
	static void texture_shader(unsigned texture);

	static Camera* mainCamera;
	static void init_camera();

	static std::vector<DirLight*> dirLights;

};

#endif // !GLOBAL_H
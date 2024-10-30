#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <stb/stb_image.h>
#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_opengl3.h>

#include "Global.h"
#include "Shader.h"
#include "Camera.h"
#include "Model.h"
#include "Light.h"

#include "Particle.h"
#include "RigidBody.h"
#include "Cloth.h"
#include "PBD.h"

float lastX = Global::screenWidth / 2.f, lastY = Global::screenHeight / 2.f;
bool firstMouse = true;

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void process_input(GLFWwindow* window);
void mouse_callback(GLFWwindow* window, double x, double y);

int main()
{
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	//glfwWindowHint(GL_MULTISAMPLE, 4);

	GLFWwindow* window = glfwCreateWindow(Global::screenWidth, Global::screenHeight, "PBD", nullptr, nullptr);
	if (window == nullptr)
	{
		std::cerr << "Failed to create GLFW window." << std::endl;
		glfwTerminate();
		return -1;
	}

	glfwMakeContextCurrent(window);
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}
	std::cout << glGetString(GL_VERSION) << std::endl;
	
	glfwSwapInterval(1);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	glfwSetCursorPosCallback(window, mouse_callback);

	stbi_set_flip_vertically_on_load(true);
	/*int data;
	glGetIntegerv(GL_NUM_EXTENSIONS, &data);
	std::cout << data << std::endl;
	for (int i = 0; i < data; i++)
	{
		const GLubyte* name = glGetStringi(GL_EXTENSIONS, i);
		{
			std::cout << (const char*)name << std::endl;
		}
	}*/

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
	ImFont* font = io.Fonts->AddFontFromFileTTF("C:/Windows/Fonts/Arial.ttf", 30.0f);
	IM_ASSERT(font != nullptr);
	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsLight();
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init();

	Global::init();
	PBD pbd;

	Model* mModel = new Model("D:/Assets/Model/cloth/cloth.obj");
	Model* Cube = new Model("asset/model/Cube.obj");
	Model* bunny = new Model("D:/Unity project/games103/Assets/bunny.obj");
	Sphere sphere(glm::vec3(0.f, -1.f, 0.f), 0.5f);
	sphere.generate_mesh(16, 32);

	auto create_cloth = []() {
		unsigned int n = 21;
		std::vector<glm::vec3> vertices(n * n);
		std::vector<glm::vec2> texCoords(n * n);
		std::vector<unsigned int> indices((n - 1) * (n - 1) * 6);
		
		for (unsigned int i = 0; i < n ; i++)
			for (unsigned int j = 0; j < n; j++)
			{
				vertices[j * n + i] = glm::vec3(5 - 10.f * i / (n - 1), 0.f, 5 - 10.f * j / (n - 1));
				texCoords[j * n + i] = glm::vec2(i / (n - 1.f), j / (n - 1.f));
			}

		unsigned int t = 0;
		for (unsigned int i = 0; i < n - 1; i++)
			for (unsigned int j = 0; j < n - 1; j++)
			{
				indices[t * 6 + 0] = j * n + i;
				indices[t * 6 + 1] = j * n + i + 1;
				indices[t * 6 + 2] = (j + 1) * n + i + 1;
				indices[t * 6 + 3] = j * n + i;
				indices[t * 6 + 4] = (j + 1) * n + i + 1;
				indices[t * 6 + 5] = (j + 1) * n + i;
				t++;
			}

		Mesh* mesh = new Mesh("cloth");
		/*mesh->set_vertices(vertices);
		mesh->set_texCoords(texCoords);
		mesh->set_indices(indices);*/

		return mesh;
		};
	Mesh* clothMesh = create_cloth();
	//clothMesh->recalculate_normals();

	RigidBody rb1(bunny->find_mesh("defaultobject"));
	Cloth cloth(mModel->find_mesh("cloth"));
	Fluid water(Cube->find_mesh("Cube"));

	pbd.add_rigid_body(&rb1);
	pbd.add_cloth(&cloth);
	pbd.add_fluid(&water);
	pbd.init();

	PipelineShader shader("shader/vertexShader.glsl", "shader/fragmentShader.glsl");

	DirLight* dirLight = new DirLight(glm::vec3(-0.2f, -1.0f, -0.3f), glm::vec3(0.05f), glm::vec3(0.5f), glm::vec3(0.5f));
	SpotLight spotLight(Global::mainCamera->position, Global::mainCamera->front, glm::vec3(0.f), glm::vec3(1.f), glm::vec3(1.f), 1.f, 0.09f, 0.032f, glm::cos(glm::radians(12.5f)), glm::cos(glm::radians(15.f)));
	Global::dirLights.emplace_back(dirLight);

	glm::mat4 model = glm::mat4(1.f);
	glm::mat4 view = glm::mat4(1.f);
	glm::mat4 projection = glm::mat4(1.f);

	shader.use();

	dirLight->setup(shader);

	shader.set_float("material.Ns", 225.f);

	ImVec4 clearColor = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

	bool launch = false;
	glEnable(GL_DEPTH_TEST);
	// glEnable(GL_MULTISAMPLE);
	// glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	while (!glfwWindowShouldClose(window))
	{
		//std::cout << glGetError() << " ";

		process_input(window);
		if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS)
			launch = true;

		glfwPollEvents();
		if (glfwGetWindowAttrib(window, GLFW_ICONIFIED) != 0)
		{
			ImGui_ImplGlfw_Sleep(10);
			continue;
		}

		float currentFrame = static_cast<float>(glfwGetTime());
		Global::deltaTime = currentFrame - Global::lastFrame;
		Global::lastFrame = currentFrame;
		int w, h;
		glfwGetWindowSize(window, &w, &h);
		Global::screenWidth = static_cast<unsigned>(w);
		Global::screenHeight = static_cast<unsigned>(h);
		glViewport(0, 0, Global::screenWidth, Global::screenHeight);

		// Start the Dear ImGui frame
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		{
			ImGui::Begin("PBD");

			ImGui::ColorEdit3("clear color", (float*)&clearColor);
			ImGui::Text("Application average %.1f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
			
			ImGui::End();
		}

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glClearColor(clearColor.x * clearColor.w, clearColor.y * clearColor.w, clearColor.z * clearColor.w, clearColor.w);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		shader.use();
		model = glm::mat4(1.f);
		view = Global::mainCamera->get_view_matrix();
		projection = Global::mainCamera->get_perspective_matrix();

		spotLight.position = Global::mainCamera->position;
		spotLight.direction = Global::mainCamera->front;
		spotLight.setup(shader);

		if (launch)
		{
			pbd.update(Global::deltaTime);
		}
		//pbd.draw(shader);

		for (unsigned i = 0; i < cloth.particleNr; i++)
		{
			model = glm::mat4(1.f);
			model = glm::translate(model, cloth.particles[i].x);
			model = glm::scale(model, glm::vec3(2 * Particle::radius));
			shader.use();
			shader.set_mat4("model", model);
			shader.set_mat4("view", view);
			shader.set_mat4("projection", projection);
			sphere.mesh->draw(shader);
		}

		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		glfwSwapBuffers(window);
	}

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
}

void process_input(GLFWwindow* window)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);

	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		Global::mainCamera->process_keyboard(FORWARD, Global::deltaTime);
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		Global::mainCamera->process_keyboard(BACKWARD, Global::deltaTime);
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		Global::mainCamera->process_keyboard(LEFT, Global::deltaTime);
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		Global::mainCamera->process_keyboard(RIGHT, Global::deltaTime);
}

void mouse_callback(GLFWwindow* window, double x, double y)
{
	float xPos = static_cast<float>(x);
	float yPos = static_cast<float>(y);

	if (firstMouse)
	{
		lastX = xPos;
		lastY = yPos;
		firstMouse = false;
	}

	float xOffset = xPos - lastX;
	float yOffset = lastY - yPos;
	lastX = xPos, lastY = yPos;

	Global::mainCamera->process_mouse_movement(xOffset, yOffset);
}
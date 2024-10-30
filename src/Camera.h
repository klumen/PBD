#ifndef CAMERA_H
#define CAMERA_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

enum CameraMovement
{
	FORWARD,
	BACKWARD,
	LEFT,
	RIGHT
};

class Camera
{
public:
	Camera(const glm::vec3& position = glm::vec3(0.f), float pitch = 0.f, float yaw = -90.f);
	~Camera() = default;

	glm::mat4 get_view_matrix();
	glm::mat4 get_perspective_matrix();
	void process_keyboard(CameraMovement direction, float deltaTime);
	void process_mouse_movement(float xOffset, float yOffset);

	glm::vec3 position = glm::vec3(0.f);
	glm::vec3 front = glm::vec3(0.f, 0.f, -1.f), up = glm::vec3(0.f, 1.f, 0.f);
	float pitch = 0.f, yaw = -90.f;
	float fov = 45.f, aspect = 1.f;
	float moveSpeed = 2.5f, mouseSensitivity = 0.1f;
	float near = 0.1f, far = 100.f;

	glm::mat4 view{ 1.f };
	glm::mat4 projection{ 1.f };

private:
	void update_orientation();

	glm::vec3 cameraX, cameraY, cameraZ;

};

#endif // !CAMERA_H
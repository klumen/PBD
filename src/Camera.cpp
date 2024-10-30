#include "Camera.h"
#include "Global.h"

Camera::Camera(const glm::vec3& position /*= glm::vec3(0.f)*/, float pitch /*= 0.f*/, float yaw /*= -90.f*/) : position(position), pitch(pitch), yaw(yaw)
{
	update_orientation();
}

glm::mat4 Camera::get_view_matrix()
{
	return view = glm::lookAt(position, position + front, up);
}

glm::mat4 Camera::get_perspective_matrix()
{
	aspect = Global::screenWidth * 1.f / Global::screenHeight;
	return projection = glm::perspective(glm::radians(fov), aspect, near, far);
}

void Camera::process_keyboard(CameraMovement direction, float deltaTime)
{
	float distance = deltaTime * moveSpeed;
	if (direction == FORWARD)
		position -= cameraZ * distance;
	if (direction == BACKWARD)
		position += cameraZ * distance;
	if (direction == LEFT)
		position -= cameraX * distance;
	if (direction == RIGHT)
		position += cameraX * distance;
}

void Camera::process_mouse_movement(float xOffset, float yOffset)
{
	xOffset *= mouseSensitivity;
	yOffset *= mouseSensitivity;

	pitch += yOffset;
	yaw += xOffset;

	if (pitch > 89.f)
		pitch = 89.f;
	if (pitch < -89.f)
		pitch = -89.f;

	update_orientation();
}

void Camera::update_orientation()
{
	glm::vec3 frontTemp;
	frontTemp.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
	frontTemp.y = sin(glm::radians(pitch));
	frontTemp.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));

	front = glm::normalize(frontTemp);

	cameraZ = -front;
	cameraX = glm::normalize(glm::cross(front, up));
	cameraY = glm::normalize(glm::cross(cameraZ, cameraX));
}
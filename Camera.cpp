#define GLM_ENABLE_EXPERIMENTAL
#include"Camera.h"

Camera::Camera(int width, int height, glm::vec3 position)
{
	Camera::width = width;
	Camera::height = height;
	Position = position;
}

void Camera::updateMatrix(float FOVdeg, float nearPlane, float farPlane)
{
	glm::mat4 view = glm::mat4(1.0f);
	glm::mat4 projection = glm::mat4(1.0f);

	view = glm::lookAt(Position, Position + Orientation, Up);
	projection = glm::perspective(glm::radians(FOVdeg), (float)width / height, nearPlane, farPlane);

	cameraMatrix = projection * view;
}

void Camera::Matrix(Shader& shader, const char* uniform)
{
	glUniformMatrix4fv(glGetUniformLocation(shader.ID, uniform), 1, GL_FALSE, glm::value_ptr(cameraMatrix));
}



void Camera::Inputs(GLFWwindow* window, bool allowMovement)
{
	if (allowMovement)
	{
		glm::vec3 forward = glm::normalize(glm::vec3(Orientation.x, 0.0f, Orientation.z));
		glm::vec3 right = glm::normalize(glm::cross(forward, Up));
		if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
			Position += speed * forward;
		if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
			Position += speed * -right;
		if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
			Position += speed * -forward;
		if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
			Position += speed * right;
		if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
			speed = 0.4f;
		else
			speed = 0.1f;
	}


	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	double mouseX = 0.0;
	double mouseY = 0.0;
	glfwGetCursorPos(window, &mouseX, &mouseY);

	if (!mouseInitialized)
	{
		lastMouseX = mouseX;
		lastMouseY = mouseY;
		mouseInitialized = true;
		return;
	}

	float offsetX = static_cast<float>(mouseX - lastMouseX) * sensitivity;
	float offsetY = static_cast<float>(mouseY - lastMouseY) * sensitivity;
	lastMouseX = mouseX;
	lastMouseY = mouseY;

	yaw += offsetX;
	pitch -= offsetY;
	pitch = glm::clamp(pitch, -89.0f, 89.0f);

	glm::vec3 direction;
	direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
	direction.y = sin(glm::radians(pitch));
	direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
	Orientation = glm::normalize(direction);
}

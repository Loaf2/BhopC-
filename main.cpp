#include<filesystem>
namespace fs = std::filesystem;

#define GLM_ENABLE_EXPERIMENTAL
#include"Model.h"
#include<iostream>
#include<array>
#include<algorithm>
#include<cmath>
#include<glad/glad.h>
#include<GLFW/glfw3.h>
#include<stb/stb_image.h>
#include<glm/glm.hpp>
#include<glm/gtc/matrix_transform.hpp>
#include<glm/gtc/type_ptr.hpp>

#include"Texture.h"
#include"shaderClass.h"
#include"VAO.h"
#include"VBO.h"
#include"EBO.h"

const unsigned int width = 800;
const unsigned int height = 800;

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow *window);

int main()
{
	glfwInit();

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow* window = glfwCreateWindow(width, height, "YoutubeOpenGL", NULL, NULL);
	if (window == NULL)
	{
		std::cerr << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

	gladLoadGL();
	glViewport(0, 0, width, height);

	Shader shaderProgram("default.vert", "default.frag");

	glm::vec4 lightColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
	glm::vec3 lightPos = glm::vec3(0.5f, 0.5f, 0.5f);
	glm::mat4 lightModel = glm::mat4(1.0f);
	lightModel = glm::translate(lightModel, lightPos);

	shaderProgram.Activate();
	glUniform4f(glGetUniformLocation(shaderProgram.ID, "lightColor"), lightColor.x, lightColor.y, lightColor.z, lightColor.w);
	glUniform3f(glGetUniformLocation(shaderProgram.ID, "lightPos"), lightPos.x, lightPos.y, lightPos.z);

	glEnable(GL_DEPTH_TEST);

	Camera camera(width, height, glm::vec3(0.0f, 2.5f, 6.0f));

	fs::path modelPath = fs::current_path() / "Resources" / "models" / "plane" / "plane.gltf";
	std::string modelFile = modelPath.string();

	if (!fs::exists(modelPath))
	{
		std::cerr << "Model file not found: " << modelPath << std::endl;
		glfwDestroyWindow(window);
		glfwTerminate();
		return -1;
	}

	Model model(modelFile.c_str());

	fs::path orbPath = fs::current_path() / "Resources" / "models" / "orb" / "redorb.gltf";
	std::string orbFile = orbPath.string();
	if (!fs::exists(orbPath))
	{
		std::cerr << "Orb model not found: " << orbPath << std::endl;
		glfwDestroyWindow(window);
		glfwTerminate();
		return -1;
	}
	Model orbModel(orbFile.c_str());

	struct Ball
	{
		glm::vec3 position;
		glm::vec3 velocity = glm::vec3(0.0f);
		bool grounded = false;
	};

	const float platformTop = -0.42756f;
	const float platformHalfWidth = 24.206254f + 0.25f;
	const float platformHalfDepth = 21.531103f + 0.25f;
	const float gravity = 15.0f;
	const float moveSpeed = 5.0f;
	const float jumpSpeed = 7.0f;
	const float cameraGroundY = platformTop + 1.5f;
	const float cameraRadius = 0.5f;
	const float ballRadius = 1.0f;
	const float ballGroundY = platformTop - ballRadius;
	const float orbSpawnHeight = ballGroundY - 15.0f;
	std::array<Ball, 3> balls =
	{
		Ball{ glm::vec3(-3.0f, orbSpawnHeight, 0.0f) },
		Ball{ glm::vec3(0.0f, orbSpawnHeight, 0.0f) },
		Ball{ glm::vec3(3.0f, orbSpawnHeight, 0.0f) }
	};
	int activeBall = 0;
	bool previousX = false;
	bool previousZ = false;
	float cameraVerticalVelocity = 0.0f;
	double previousTime = glfwGetTime();

	while (!glfwWindowShouldClose(window))
	{
		processInput(window);

		double currentTime = glfwGetTime();
		float deltaTime = static_cast<float>(std::min(currentTime - previousTime, 0.05));
		previousTime = currentTime;

		bool cameraJumpPressed = glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS;
		bool cameraOverPlatform =
			std::abs(camera.Position.x) <= platformHalfWidth &&
			std::abs(camera.Position.z) <= platformHalfDepth;
		bool cameraGrounded = cameraOverPlatform && camera.Position.y <= cameraGroundY + 0.001f;
		if (cameraGrounded)
		{
			camera.Position.y = cameraGroundY;
			cameraVerticalVelocity = 0.0f;
		}
		if (cameraJumpPressed && cameraGrounded)
			cameraVerticalVelocity = jumpSpeed;

		cameraVerticalVelocity -= gravity * deltaTime;
		camera.Position.y += cameraVerticalVelocity * deltaTime;
		if (cameraOverPlatform && camera.Position.y < cameraGroundY)
		{
			camera.Position.y = cameraGroundY;
			cameraVerticalVelocity = 0.0f;
		}

		Ball& ball = balls[activeBall];
		glm::vec3 movement(0.0f);
		if (glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS) movement.z -= 1.0f;
		if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS) movement.z += 1.0f;
		if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS) movement.x -= 1.0f;
		if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS) movement.x += 1.0f;
		if (glm::length(movement) > 0.0f)
		{
			glm::vec3 direction = glm::normalize(movement);
			ball.velocity.x = direction.x * moveSpeed;
			ball.velocity.z = direction.z * moveSpeed;
		}
		bool xPressed = glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS;
		if (xPressed && !previousX)
			activeBall = (activeBall + 1) % static_cast<int>(balls.size());
		previousX = xPressed;

		bool zPressed = glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS;
		if (zPressed && !previousZ)
			activeBall = (activeBall + static_cast<int>(balls.size()) - 1) % static_cast<int>(balls.size());
		previousZ = zPressed;

		for (Ball& currentBall : balls)
		{
			currentBall.velocity.y += gravity * deltaTime;
			currentBall.position += currentBall.velocity * deltaTime;

			bool overPlatform =
				std::abs(currentBall.position.x) <= platformHalfWidth &&
				std::abs(currentBall.position.z) <= platformHalfDepth;
			if (overPlatform && currentBall.position.y >= ballGroundY)
			{
				currentBall.position.y = ballGroundY;
				currentBall.velocity.y = 0.0f;
				currentBall.grounded = true;
			}
			else
			{
				currentBall.grounded = false;
			}
		}

		glClearColor(0.07f, 0.13f, 0.17f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		camera.Inputs(window, true);

		for (const Ball& currentBall : balls)
		{
			glm::vec2 horizontalOffset = glm::vec2
			(
				camera.Position.x - currentBall.position.x,
				camera.Position.z - currentBall.position.z
			);
			float distance = glm::length(horizontalOffset);
			float minimumDistance = cameraRadius + ballRadius;

			if (distance < minimumDistance)
			{
				glm::vec2 collisionNormal = distance > 0.0001f
					? horizontalOffset / distance
					: glm::vec2(0.0f, 1.0f);
				float penetration = minimumDistance - distance;
				camera.Position.x += collisionNormal.x * penetration;
				camera.Position.z += collisionNormal.y * penetration;

				cameraVerticalVelocity = jumpSpeed;
			}
		}

		camera.updateMatrix(45.0f, 0.1f, 100.0f);

		model.Draw(shaderProgram, camera);
		for (const Ball& currentBall : balls)
			orbModel.Draw(shaderProgram, camera, currentBall.position);

		glfwSwapBuffers(window);
		glfwPollEvents();
	};

	shaderProgram.Delete();
	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
};

void processInput(GLFWwindow *window)
{
    if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
};

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
};
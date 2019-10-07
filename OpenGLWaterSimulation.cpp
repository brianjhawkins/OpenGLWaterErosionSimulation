#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <assimp/config.h>
#include "stb_image.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/noise.hpp>
#include <glm/gtx/normal.hpp>

#include "shader.h"
#include "camera.h"
#include "model.h"
#include "mesh.h"

#include <iostream>

using namespace std;

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow *window);
unsigned int loadTexture(const char* path);
vector<float> GenerateMeshVertices(unsigned int width, unsigned int height);
vector<unsigned int> GenerateMeshIndices(unsigned int width, unsigned int height);
void GenerateMeshTextures(unsigned int width, unsigned int height);
void GenerateBaseTextures(unsigned int width, unsigned int height);

// window settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// camera
Camera camera(glm::vec3(0.0f, 10.0f, 0.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;	// time between current frame and last frame
float lastFrame = 0.0f;

// light
glm::vec3 lightPos(1.2f, 1.0f, 2.0f);

// mesh settings
const unsigned int MESH_WIDTH = 256;
const unsigned int MESH_HEIGHT = MESH_WIDTH;
const unsigned int MESH_TOTAL_SIZE = 10;
const float MESH_SCALE = (float)MESH_TOTAL_SIZE / (float)MESH_WIDTH;
unsigned int meshVAO, meshVBO, meshEBO;

// texture settings
const float HEIGHT_SCALING_VALUE = 5.0f;
vector<float> CDTexture(MESH_WIDTH * MESH_HEIGHT * 4);
vector<float> FTexture(MESH_WIDTH * MESH_HEIGHT * 4);
vector<float> VTexture(MESH_WIDTH * MESH_HEIGHT * 4);
unsigned int CDTextureID, FTextureID, VTextureID;

// frame buffer settings
unsigned int CDFBO, FFBO, VFBO;

// texture settings
const GLenum TEXTURE_FORMAT = GL_RGBA;
const GLenum INTERNAL_TEXTURE_FORMAT = GL_RGBA32F;

// debug settings
bool drawPolygon = false;

int main()
{
	// glfw: initialize and configure
	// ------------------------------
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // uncomment this statement to fix compilation on OS X
#endif

	// glfw window creation
	// --------------------
	GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "OpenGLWaterSimulation", NULL, NULL);
	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetScrollCallback(window, scroll_callback);

	// tell GLFW to capture our mouse
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	// glad: load all OpenGL function pointers
	// ---------------------------------------
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}

	// configure global opengl state
	// -----------------------------
	glEnable(GL_DEPTH_TEST);

	// build and compile our shader zprogram
	// ------------------------------------
	Shader waterIncrementShader("waterIncrement.vs", "waterIncrement.fs");
	Shader terrainRenderShader("terrainRender.vs", "terrainRender.fs");
	Shader normalShader("displayNormals.vs", "displayNormals.fs", "displayNormals.gs");

	// set up vertex data (and buffer(s)) and configure vertex attributes
	// ------------------------------------------------------------------
	Model ourModel("nanosuit/nanosuit.obj");

	vector<unsigned int> meshIndices = GenerateMeshIndices(MESH_WIDTH, MESH_HEIGHT);
	vector<float> meshVertices = GenerateMeshVertices(MESH_WIDTH, MESH_HEIGHT);
	GenerateMeshTextures(MESH_WIDTH, MESH_HEIGHT);
	
	// Create mesh vertex array buffer
	glGenVertexArrays(1, &meshVAO);
	glGenBuffers(1, &meshVBO);
	glGenBuffers(1, &meshEBO);

	glBindVertexArray(meshVAO);
	glBindBuffer(GL_ARRAY_BUFFER, meshVBO);

	glBufferData(GL_ARRAY_BUFFER, meshVertices.size() * sizeof(float), &meshVertices[0], GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, meshEBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, meshIndices.size() * sizeof(unsigned int), &meshIndices[0], GL_STATIC_DRAW);

	// vertex positions
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
	// vertex texture coords
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));

	glBindVertexArray(0);

	// Create framebuffers for textures
	// column data framebuffer
	glGenFramebuffers(1, &CDFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, CDFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, CDTextureID, 0);

	// check that framebuffer was successfully completed
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << endl;
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// flux framebuffer
	glGenFramebuffers(1, &FFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, FFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, FTextureID, 0);

	// check that framebuffer was successfully completed
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << endl;
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// velocity framebuffer
	glGenFramebuffers(1, &VFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, VFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, VTextureID, 0);

	// check that framebuffer was successfully completed
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << endl;
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// render loop
	// -----------
	while (!glfwWindowShouldClose(window))
	{
		// per-frame time logic
		// --------------------
		float currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		// input
		// -----
		processInput(window);

		// First Pass: Water Increment Step
		glBindFramebuffer(GL_FRAMEBUFFER, CDFBO);
		waterIncrementShader.use();
		waterIncrementShader.setFloat("CDTexture", CDTextureID);
		glBindVertexArray(meshVAO);
		glBindTexture(GL_TEXTURE_2D, CDTextureID);
		glDrawElements(GL_TRIANGLES, meshIndices.size(), GL_UNSIGNED_INT, 0);
		glBindVertexArray(0);

		// Bind back to default frame buffer
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		
		// Final Pass: Terrain Render Step
		// render
		// ------
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);	

		// activate terrain render shader
		terrainRenderShader.use();
		// set shader properties
		terrainRenderShader.setVec3("viewPos", camera.Position);
		terrainRenderShader.setFloat("size", MESH_TOTAL_SIZE);
		terrainRenderShader.setFloat("terrainTexture", CDTextureID);
		terrainRenderShader.setFloat("terrainShininess", 1.0f);
		terrainRenderShader.setFloat("waterShininess", 64.0f);
		terrainRenderShader.setVec3("terrainColor", 0.7f, 0.6f, 0.35f);
		terrainRenderShader.setVec3("waterColor", 0.0f, 0.0f, 0.7f);
		terrainRenderShader.setVec3("dirLight.direction", -0.0f, -1.0f, -0.0f);
		terrainRenderShader.setVec3("dirLight.ambient", 0.3f, 0.3f, 0.3f);
		terrainRenderShader.setVec3("dirLight.diffuse", 0.6f, 0.6f, 0.6f);
		terrainRenderShader.setVec3("dirLight.specular", 0.7f, 0.7f, 0.7f);
		terrainRenderShader.setVec3("dirLight.lightColor", 1.0f, 1.0f, 1.0f);

		// pass projection matrix to shader (note that in this case it could change every frame)
		glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
		terrainRenderShader.setMat4("projection", projection);

		// camera/view transformation
		glm::mat4 view = camera.GetViewMatrix();
		terrainRenderShader.setMat4("view", view);

		// world transformation
		//glm::mat4 model = glm::mat4(1.0f);
		//model = glm::translate(model, glm::vec3(0.0f, -1.75f, 0.0f));
		//model = glm::scale(model, glm::vec3(0.2f, 0.2f, 0.2f));
		//ourShader.setMat4("model", model);
		//ourModel.Draw(ourShader);
		
		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(-(float)MESH_TOTAL_SIZE / (float)2, 0.0f, (float)MESH_TOTAL_SIZE / (float)2));
		terrainRenderShader.setMat4("model", model);
		
		// bind texture
		glBindTexture(GL_TEXTURE_2D, CDTextureID);

		// render mesh
		glBindVertexArray(meshVAO);
		glDrawElements(GL_TRIANGLES, meshIndices.size(), GL_UNSIGNED_INT, 0);
		glBindVertexArray(0);

		// display normals with normalShader
		//normalShader.use();
		//normalShader.setMat4("projection", projection);
		//normalShader.setMat4("view", view);
		//normalShader.setMat4("model", model);
		//baseMesh.Draw(normalShader);

		// glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
		// -------------------------------------------------------------------------------
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	// glfw: terminate, clearing all previously allocated GLFW resources.
	// ------------------------------------------------------------------
	glfwTerminate();
	return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);

	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
		camera.ProcessKeyboard(FORWARD, deltaTime);
	}
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
		camera.ProcessKeyboard(BACKWARD, deltaTime);
	}
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
		camera.ProcessKeyboard(LEFT, deltaTime);
	}
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
		camera.ProcessKeyboard(RIGHT, deltaTime);
	}
	if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS) {
		drawPolygon = !drawPolygon;
		if (drawPolygon) {
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		}
		else {
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		}
	}
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	// make sure the viewport matches the new window dimensions; note that width and 
	// height will be significantly larger than specified on retina displays.
	glViewport(0, 0, width, height);
}


// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
	if (firstMouse)
	{
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
	}

	float xoffset = xpos - lastX;
	float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

	lastX = xpos;
	lastY = ypos;

	camera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	camera.ProcessMouseScroll(yoffset);
}

unsigned int loadTexture(char const* path) {
	unsigned int textureID;
	glGenTextures(1, &textureID);

	int width, height, nrComponents;
	unsigned char* data = stbi_load(path, &width, &height, &nrComponents, 0);

	if (data) {
		GLenum format;
		if (nrComponents == 1) {
			format = GL_RED;
		}
		else if (nrComponents == 3) {
			format = GL_RGB;
		}
		else if (nrComponents == 4) {
			format = GL_RGBA;
		}

		glBindTexture(GL_TEXTURE_2D, textureID);
		glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	}
	else {
		cout << "Texture failed to load at path: " << path << endl;
	}

	stbi_image_free(data);

	return textureID;
}

vector<float> GenerateMeshVertices(unsigned int width, unsigned int height) {
	vector<float> vertexList;

	// Generate vertex positions and texture coordinates
	for (int j = 0; j < height; j++) {
		for (int i = 0; i < width; i++) {
			// vertex position
			vertexList.push_back(i * MESH_SCALE);
			vertexList.push_back(0);
			vertexList.push_back(-j * MESH_SCALE);

			// vertex texture coordinate
			vertexList.push_back((float)i / (width - 1));
			vertexList.push_back((float)j / (height - 1));
		}
	}

	return vertexList;
}

vector<unsigned int> GenerateMeshIndices(unsigned int width, unsigned int height) {
	vector<unsigned int> indexList;
	unsigned int index;

	for (int j = 0; j < height - 1; j++) {
		for (int i = 0; i < width - 1; i++) {
			index = i + j * width;

			// first triangle
			indexList.push_back(index);
			indexList.push_back(index + width);
			indexList.push_back(index + 1);

			// second triangle
			indexList.push_back(index + width + 1);
			indexList.push_back(index + 1);
			indexList.push_back(index + width);
		}
	}

	return indexList;
}

void GenerateMeshTextures(unsigned int width, unsigned int height) {
	GenerateBaseTextures(width, height);

	// column data texture binding
	glGenTextures(1, &CDTextureID);
	glBindTexture(GL_TEXTURE_2D, CDTextureID);
	glTexImage2D(GL_TEXTURE_2D, 0, INTERNAL_TEXTURE_FORMAT, width, height, 0, TEXTURE_FORMAT, GL_FLOAT, &CDTexture[0]);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	// flux texture binding
	glGenTextures(1, &FTextureID);
	glBindTexture(GL_TEXTURE_2D, FTextureID);
	glTexImage2D(GL_TEXTURE_2D, 0, INTERNAL_TEXTURE_FORMAT, width, height, 0, TEXTURE_FORMAT, GL_FLOAT, &FTexture[0]);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	// velocity texture binding
	glGenTextures(1, &VTextureID);
	glBindTexture(GL_TEXTURE_2D, VTextureID);
	glTexImage2D(GL_TEXTURE_2D, 0, INTERNAL_TEXTURE_FORMAT, width, height, 0, TEXTURE_FORMAT, GL_FLOAT, &VTexture[0]);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
}

void GenerateBaseTextures(unsigned int width, unsigned int height) {
	unsigned int location;

	for (int j = 0; j < height; j++) {
		for (int i = 0; i < width; i++) {
			location = (i + j * width) * 4;

			float iCoord = (float)i / (width - 1);
			float jCoord = (float)j / (height - 1);
			float noiseValue = glm::perlin(glm::tvec2<float, glm::precision::highp>(iCoord, jCoord)) + 0.5f * glm::perlin(glm::tvec2<float, glm::precision::highp>(2 * iCoord, 2 * jCoord)) + 0.25f * glm::perlin(glm::tvec2<float, glm::precision::highp>(4 * iCoord, 4 * jCoord));
			noiseValue += 1;

			noiseValue /= HEIGHT_SCALING_VALUE;

			// initial column data texture
			CDTexture[location + 0] = 0.0f; // R = water height value
			CDTexture[location + 1] = noiseValue; // G = terrain height value
			CDTexture[location + 2] = 0.0f; // B = dissolved sediment value
			CDTexture[location + 3] = 1.0f; // A

			// initial flux texture
			FTexture[location + 0] = 0.0f; // R = left flux value
			FTexture[location + 1] = 0.0f; // G = right flux value
			FTexture[location + 2] = 0.0f; // B = top flux value
			FTexture[location + 3] = 0.0f; // A = bottom flux value

			// initial velocity texture
			VTexture[location + 0] = 0.0f; // R = velocity in x-direction
			VTexture[location + 1] = 0.0f; // G = velocity in y-direction
			VTexture[location + 2] = 0.0f; // B 
			VTexture[location + 3] = 0.0f; // A 
		}
	}
}
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
vector<Vertex> GenerateMeshVertices(unsigned int width, unsigned int height, vector<unsigned int> indices);
vector<unsigned int> GenerateMeshIndices(unsigned int width, unsigned int height);
vector<Texture> GenerateMeshTextures(unsigned int width, unsigned int height);
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
const unsigned int MESH_TOTAL_SIZE = 5;
const float MESH_SCALE = (float)MESH_TOTAL_SIZE / (float)MESH_WIDTH;
static vector<unsigned char> baseDiffuseTexture(MESH_WIDTH * MESH_HEIGHT * 4);
static vector<unsigned char> baseSpecularTexture(MESH_WIDTH * MESH_HEIGHT * 4);

// texture settings
const GLenum TEXTURE_FORMAT = GL_RGBA;

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
	Shader ourShader("4.6.model_loading.vs", "4.6.model_loading.fs");

	// set up vertex data (and buffer(s)) and configure vertex attributes
	// ------------------------------------------------------------------
	Model ourModel("nanosuit/nanosuit.obj");

	vector<unsigned int> meshIndices = GenerateMeshIndices(MESH_WIDTH, MESH_HEIGHT);
	vector<Vertex> meshVertices = GenerateMeshVertices(MESH_WIDTH, MESH_HEIGHT, meshIndices);
	vector<Texture> meshTextures = GenerateMeshTextures(MESH_WIDTH, MESH_HEIGHT);

	Mesh baseMesh(meshVertices, meshIndices, meshTextures);

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

		// render
		// ------
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);	

		// activate shader
		ourShader.use();
		// set shader properties
		ourShader.setVec3("viewPos", camera.Position);
		ourShader.setFloat("material.shininess", 4.0f);
		ourShader.setVec3("dirLight.direction", -0.2f, -1.0f, -0.3f);
		ourShader.setVec3("dirLight.ambient", 0.3f, 0.3f, 0.3f);
		ourShader.setVec3("dirLight.diffuse", 0.4f, 0.4f, 0.4f);
		ourShader.setVec3("dirLight.specular", 0.5f, 0.5f, 0.5f);

		// pass projection matrix to shader (note that in this case it could change every frame)
		glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
		ourShader.setMat4("projection", projection);

		// camera/view transformation
		glm::mat4 view = camera.GetViewMatrix();
		ourShader.setMat4("view", view);

		// world transformation
		//glm::mat4 model = glm::mat4(1.0f);
		//model = glm::translate(model, glm::vec3(0.0f, -1.75f, 0.0f));
		//model = glm::scale(model, glm::vec3(0.2f, 0.2f, 0.2f));
		//ourShader.setMat4("model", model);
		//ourModel.Draw(ourShader);
		
		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(-(float)MESH_TOTAL_SIZE / (float)2, 0.0f, (float)MESH_TOTAL_SIZE / (float)2));
		ourShader.setMat4("model", model);
		baseMesh.Draw(ourShader);

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

vector<Vertex> GenerateMeshVertices(unsigned int width, unsigned int height, vector<unsigned int> indices) {
	vector<Vertex> vertexList;
	Vertex newVertex;
	unsigned int index;
	float noiseValue;
	float iCoord;
	float jCoord;

	// Generate vertex positions first
	// Used to calculate correct normal values later
	for (int j = 0; j < height; j++) {
		for (int i = 0; i < width; i++) {
			index = i + j * width;
			
			iCoord = (float)i / width;
			jCoord = (float)j / height;
			noiseValue = glm::perlin(glm::tvec2<float, glm::precision::highp>(iCoord, jCoord)) + 0.5f * glm::perlin(glm::tvec2<float, glm::precision::highp>(2 * iCoord, 2 * jCoord)) + 0.25f * glm::perlin(glm::tvec2<float, glm::precision::highp>(4 * iCoord, 4 * jCoord));
			newVertex.Position = glm::vec3(i * MESH_SCALE, noiseValue, -j * MESH_SCALE);
			newVertex.Normal = glm::vec3(0.0f, 1.0f, 0.0f); // temporary normal value, will be changed below
			newVertex.TexCoords = glm::vec2((float)i / (width - 1), (float)j / (height - 1));

			vertexList.push_back(newVertex);
		}
	}

	glm::vec3 pointA;
	glm::vec3 pointB;
	glm::vec3 pointC;
	glm::vec3 normal;

	// Calculate Normals and add new vertex into vertexList
	for (int i = 0; i < indices.size(); i += 3) {
		pointA = vertexList[indices[i + 0]].Position;
		pointB = vertexList[indices[i + 1]].Position;
		pointC = vertexList[indices[i + 2]].Position;
	
		normal = glm::triangleNormal(pointA, pointB, pointC);
		vertexList[indices[i + 0]].Normal = normal;
		vertexList[indices[i + 1]].Normal = normal;
		vertexList[indices[i + 2]].Normal = normal;
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

vector<Texture> GenerateMeshTextures(unsigned int width, unsigned int height) {
	vector<Texture> textureList;
	Texture diffuseTexture;
	Texture specularTexture;
	unsigned int diffuseTextureID, specularTextureID;

	GenerateBaseTextures(width, height);

	glGenTextures(1, &diffuseTextureID);
	glBindTexture(GL_TEXTURE_2D, diffuseTextureID);
	glTexImage2D(GL_TEXTURE_2D, 0, TEXTURE_FORMAT, width, height, 0, TEXTURE_FORMAT, GL_UNSIGNED_BYTE, &baseDiffuseTexture[0]);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	diffuseTexture.id = diffuseTextureID;
	diffuseTexture.type = "texture_diffuse";
	diffuseTexture.path = "";

	textureList.push_back(diffuseTexture);

	glGenTextures(1, &specularTextureID);
	glBindTexture(GL_TEXTURE_2D, specularTextureID);
	glTexImage2D(GL_TEXTURE_2D, 0, TEXTURE_FORMAT, width, height, 0, TEXTURE_FORMAT, GL_UNSIGNED_BYTE, &baseSpecularTexture[0]);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	specularTexture.id = specularTextureID;
	specularTexture.type = "texture_specular";
	specularTexture.path = "";

	textureList.push_back(specularTexture);

	return textureList;
}

void GenerateBaseTextures(unsigned int width, unsigned int height) {
	unsigned int location;

	for (int j = 0; j < height; j++) {
		for (int i = 0; i < width; i++) {
			location = (i + j * width) * 4;

			// diffuse texture
			baseDiffuseTexture[location + 0] = 0.7f * 255; // R
			baseDiffuseTexture[location + 1] = 0.55f * 255; // G
			baseDiffuseTexture[location + 2] = 0.35f * 255; // B
			baseDiffuseTexture[location + 3] = 1 * 255; // A

			// specular texture
			baseSpecularTexture[location + 0] = 0 * 255; // R
			baseSpecularTexture[location + 1] = 0 * 255; // G
			baseSpecularTexture[location + 2] = 0 * 255; // B
			baseSpecularTexture[location + 3] = 0 * 255; // A
		}
	}
}
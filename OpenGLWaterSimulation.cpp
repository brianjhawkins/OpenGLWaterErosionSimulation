#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <assimp/config.h>
#include "stb_image.h"
#include "FastNoise.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/noise.hpp>
#include <glm/gtc/random.hpp>
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
unsigned int GetLocation(unsigned int i, unsigned int j);
float TalusHeightDifference(float angle);

// window settings
const unsigned int SCR_WIDTH = 1980;
const unsigned int SCR_HEIGHT = 1018;

// timing
float deltaTime = 0.0f;	// time between current frame and last frame
float lastFrame = 0.0f;

// light
glm::vec3 lightPos(1.2f, 1.0f, 2.0f);

// mesh settings
const unsigned int MESH_WIDTH = 512;
const unsigned int MESH_HEIGHT = MESH_WIDTH;
const unsigned int MESH_TOTAL_SIZE = 5;
const float MESH_SCALE = (float)MESH_TOTAL_SIZE / (float)MESH_WIDTH;
unsigned int meshVAO, meshVBO, meshEBO;
unsigned int wMeshVAO, wMeshVBO, wMeshEBO;
vector<unsigned int> meshIndices;
vector<float> meshVertices;

// camera
// Camera above the middle of the map
//Camera camera(glm::vec3(0.0f, 1.5f * MESH_TOTAL_SIZE, 0.0f));

// Camera just above the ground, focusing on distinction between vegetated and non-vegetated erosion
//Camera camera(glm::vec3(-0.2f * MESH_TOTAL_SIZE / 2.0f, 0.4f * MESH_TOTAL_SIZE, -0.4f * MESH_TOTAL_SIZE / 2.0f), glm::vec3(0, 1, 0), 35, -55);

// Camera zoomed out to view entire terrain
Camera camera(glm::vec3(-1.9f * MESH_TOTAL_SIZE / 2.0f, 1.0f * MESH_TOTAL_SIZE, -1.3f * MESH_TOTAL_SIZE / 2.0f), glm::vec3(0, 1, 0), 35, -45);

float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// texture settings
const float HEIGHT_SCALING_VALUE = 10.0f;
vector<float> CDTexture(MESH_WIDTH * MESH_HEIGHT * 4);
vector<float> EmptyTexture(MESH_WIDTH * MESH_HEIGHT * 4);
unsigned int CDTextureID, WTextureID, FTextureID, VTextureID, RTextureID, STextureID, SCTextureID;
unsigned int tempCDTextureID, tempWTextureID, tempFTextureID, tempVTextureID, tempRTextureID, tempSTextureID, tempSCTextureID;

// texture settings
const GLenum TEXTURE_FORMAT = GL_RGBA;
const GLenum INTERNAL_TEXTURE_FORMAT = GL_RGBA32F;

// compute shader settings
const unsigned int WORK_GROUP_SIZE_X = 32;
const unsigned int WORK_GROUP_SIZE_Y = 32;
const unsigned int WORK_GROUP_SIZE_Z = MESH_WIDTH; // Must be kept as MESH_WIDTh so that the number of groups in z remains 1
const unsigned int NUM_GROUPS_X = MESH_WIDTH / WORK_GROUP_SIZE_X;
const unsigned int NUM_GROUPS_Y = MESH_WIDTH / WORK_GROUP_SIZE_Y;
const unsigned int NUM_GROUPS_Z = MESH_WIDTH / WORK_GROUP_SIZE_Z;

// debug settings
bool drawPolygon = false;

////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////  Main Simulation Settings  //////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////

// Vegetation Boolean Values
bool isVegetation = true;
bool isVegetationSeed = false;

// Terrain Generation Seeds
const int terrainSeed = 8675309;
const int vegetationSeed = 8675309;
float maxVegetationValue = 0.1f;

// Boolean Settings For Erosion Effects
bool isSourceFlow = true;
bool isRain = true;
bool isErosion = true;
bool isRegolith = true;
bool isSoilFlow = true;

// Water Increment Source and Rain settings
const unsigned int SOURCE_FLOW_CUTOFF_TIME = (unsigned int)(15 * max(1.0f, MESH_WIDTH / 256.0f));
const unsigned int RAIN_CUTOFF_TIME = (unsigned int)(15 * max(1.0f, MESH_WIDTH / 256.0f));
const float Km = 0.00005f; // Regolith Max Height Constant
int numberOfRaindrops = 1;
int rainRadius;

// Flux Update Settings
const float wKf = 0.999f; // Water Friction Coefficient
const float rKf = 0.05f; // Regolith Frition Coefficient
const float g = 9.81f; // Gravity Coefficient

// Soil Flow Settings
const float Kt = 2.0f;

// Sediment Erosion and Deposition Settings
const float Kdmax = 0.00001f; // Max Erosion Ramp Constant
const float Kc = 0.0003f; // Sediment Capacity Constant
const float Ks = 0.0001f; // Dissolving Constant
const float Kd = 0.0001f; // Deposition Constant

// Evaporation Settings
const float Ke = 3; // Evaporation Constant

// Simulation Settings
const float TIME_STEP = min(0.002f, 1 / (MESH_WIDTH * 2.0f));
const float PIPE_LENGTH = 256.0f / MESH_WIDTH;
const float PIPE_CROSS_SECTION_AREA = 20 * PIPE_LENGTH;

////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////

// Key Press Settings
const float KEY_PRESS_DELAY = 1.0f;
float pLastPressTime = 0;

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
	glfwMaximizeWindow(window);

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
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// build and compile our shader zprogram
	// ------------------------------------
	Shader waterIncrementComputeShader("waterIncrement.ComputeShader");
	Shader fluxUpdateComputeShader("fluxUpdate.ComputeShader");
	//Shader regolithFluxUpdateComputeShader("regolithFluxUpdate.ComputeShader");
	Shader heightUpdateComputeShader("heightUpdate.ComputeShader");
	//Shader waterHeightUpdateComputeShader("waterHeightUpdate.ComputeShader");
	//Shader regolithHeightUpdateComputeShader("regolithHeightUpdate.ComputeShader");
	Shader velocityFieldUpdateComputeShader("velocityFieldUpdate.ComputeShader");
	Shader soilFlowComputeShader("soilFlow.ComputeShader");
	Shader sedimentErosionAndDepositionComputeShader("sedimentErosionAndDeposition.ComputeShader");
	Shader sedimentTransportationComputeShader("sedimentTransportation.ComputeShader");
	Shader soilFlowDepositionComputeShader("soilFlowDeposition.ComputeShader");
	Shader evaporationComputeShader("evaporation.ComputeShader");
	Shader terrainRenderShader("terrainRender.vs", "terrainRender.fs");
	Shader waterRenderShader("waterRender.vs", "waterRender.fs");
	Shader swapBuffersComputeShader("swapBuffers.ComputeShader");

	// set up vertex data (and buffer(s)) and configure vertex attributes
	// ------------------------------------------------------------------
	//Model ourModel("nanosuit/nanosuit.obj");

	meshIndices = GenerateMeshIndices(MESH_WIDTH, MESH_HEIGHT);
	meshVertices = GenerateMeshVertices(MESH_WIDTH, MESH_HEIGHT);
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

	// Create mesh vertex array buffer
	glGenVertexArrays(1, &wMeshVAO);
	glGenBuffers(1, &wMeshVBO);
	glGenBuffers(1, &wMeshEBO);

	glBindVertexArray(wMeshVAO);
	glBindBuffer(GL_ARRAY_BUFFER, wMeshVBO);

	glBufferData(GL_ARRAY_BUFFER, meshVertices.size() * sizeof(float), &meshVertices[0], GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, wMeshEBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, meshIndices.size() * sizeof(unsigned int), &meshIndices[0], GL_STATIC_DRAW);

	// vertex positions
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
	// vertex texture coords
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));

	glBindVertexArray(0);

	// Set static shader settings
	// water increment shader static properties
	waterIncrementComputeShader.use();
	waterIncrementComputeShader.setFloat("Km", Km);
	waterIncrementComputeShader.setFloat("maxVegetationValue", maxVegetationValue);
	waterIncrementComputeShader.setFloat("timeStep", TIME_STEP);
	// set source values
	waterIncrementComputeShader.setInt("currentNumberSources", 2);
	// Source 1
	waterIncrementComputeShader.setIVec2("sources[0].position", (int)(0.25f * MESH_WIDTH), (int)(0.25f * MESH_HEIGHT));
	waterIncrementComputeShader.setInt("sources[0].radius", MESH_WIDTH / 20);
	waterIncrementComputeShader.setFloat("sources[0].Kis", 0.5f);
	// Source 2
	waterIncrementComputeShader.setIVec2("sources[1].position", (int)(0.75f * MESH_WIDTH), (int)(0.75f * MESH_HEIGHT));
	waterIncrementComputeShader.setInt("sources[1].radius", MESH_WIDTH / 40);
	waterIncrementComputeShader.setFloat("sources[1].Kis", 0.75f);
	// set rain value
	waterIncrementComputeShader.setInt("currentNumberRaindrops", numberOfRaindrops);
	// Set source flow boolean
	waterIncrementComputeShader.setBool("isSourceFlow", isSourceFlow);
	// Set rain fall boolean
	waterIncrementComputeShader.setBool("isRain", isRain);

	// flux shader static properties
	fluxUpdateComputeShader.use();
	fluxUpdateComputeShader.setBool("isRegolith", isRegolith);
	fluxUpdateComputeShader.setFloat("wKf", wKf);
	fluxUpdateComputeShader.setFloat("rKf", rKf);
	fluxUpdateComputeShader.setFloat("g", g);
	fluxUpdateComputeShader.setFloat("pipeLength", PIPE_LENGTH);
	fluxUpdateComputeShader.setFloat("pipeArea", PIPE_CROSS_SECTION_AREA);
	fluxUpdateComputeShader.setFloat("width", MESH_WIDTH);
	fluxUpdateComputeShader.setFloat("height", MESH_HEIGHT);
	fluxUpdateComputeShader.setFloat("timeStep", TIME_STEP);

	// regolith flux shader static properties
	//regolithFluxUpdateComputeShader.use();
	//regolithFluxUpdateComputeShader.setFloat("timeStep", TIME_STEP);

	// height shader static properties
	heightUpdateComputeShader.use();
	heightUpdateComputeShader.setFloat("pipeLength", PIPE_LENGTH);
	heightUpdateComputeShader.setFloat("width", MESH_WIDTH);
	heightUpdateComputeShader.setFloat("height", MESH_HEIGHT);
	heightUpdateComputeShader.setFloat("timeStep", TIME_STEP);

	// water height shader static properties
	//waterHeightUpdateComputeShader.use();
	//waterHeightUpdateComputeShader.setFloat("timeStep", TIME_STEP);

	// regolith height shader static properties
	//regolithHeightUpdateComputeShader.use();
	//regolithHeightUpdateComputeShader.setFloat("timeStep", TIME_STEP);

	// velocity update static properties
	velocityFieldUpdateComputeShader.use();
	velocityFieldUpdateComputeShader.setFloat("pipeLength", PIPE_LENGTH);
	velocityFieldUpdateComputeShader.setFloat("width", MESH_WIDTH);
	velocityFieldUpdateComputeShader.setFloat("height", MESH_HEIGHT);

	// soil flow shader static properties
	soilFlowComputeShader.use();
	soilFlowComputeShader.setBool("isSoilFlow", isSoilFlow);
	soilFlowComputeShader.setFloat("talusHeightDiff", TalusHeightDifference(60));
	soilFlowComputeShader.setFloat("Kt", Kt);
	soilFlowComputeShader.setFloat("pipeLength", PIPE_LENGTH);
	soilFlowComputeShader.setFloat("width", MESH_WIDTH);
	soilFlowComputeShader.setFloat("height", MESH_HEIGHT);
	soilFlowComputeShader.setFloat("timeStep", TIME_STEP);

	// sediment erosion and deposition shader static properties
	sedimentErosionAndDepositionComputeShader.use();
	sedimentErosionAndDepositionComputeShader.setBool("isErosion", isErosion);
	sedimentErosionAndDepositionComputeShader.setFloat("Kdmax", Kdmax);
	sedimentErosionAndDepositionComputeShader.setFloat("Kc", Kc);
	sedimentErosionAndDepositionComputeShader.setFloat("dissolvingConstant", Ks);
	sedimentErosionAndDepositionComputeShader.setFloat("Kd", Kd);
	sedimentErosionAndDepositionComputeShader.setFloat("width", MESH_WIDTH);
	sedimentErosionAndDepositionComputeShader.setFloat("height", MESH_HEIGHT);
	sedimentErosionAndDepositionComputeShader.setFloat("maxVegetationValue", maxVegetationValue);

	// sediment transportation shader static properties
	sedimentTransportationComputeShader.use();
	sedimentTransportationComputeShader.setFloat("width", MESH_WIDTH);
	sedimentTransportationComputeShader.setFloat("height", MESH_HEIGHT);
	sedimentTransportationComputeShader.setFloat("timeStep", TIME_STEP);

	// soil deposition shader static properties
	soilFlowDepositionComputeShader.use();
	soilFlowDepositionComputeShader.setFloat("width", MESH_WIDTH);
	soilFlowDepositionComputeShader.setFloat("height", MESH_HEIGHT);

	// evaporation shader static properties
	evaporationComputeShader.use();
	evaporationComputeShader.setFloat("evaporationConstant", Ke);
	evaporationComputeShader.setFloat("timeStep", TIME_STEP);

	// terrain render shader static properties
	terrainRenderShader.use();
	terrainRenderShader.setFloat("size", MESH_TOTAL_SIZE);
	terrainRenderShader.setFloat("terrainShininess", 1.0f);
	terrainRenderShader.setFloat("waterShininess", 64.0f);
	terrainRenderShader.setVec3("terrainColor", 0.87f, 0.85f, 0.6f);
	terrainRenderShader.setVec3("vegetationColor", 0.31f, 0.5f, 0.1f);
	terrainRenderShader.setVec3("deadVegetationColor", 0.6f, 0.4f, 0.2f);
	terrainRenderShader.setVec3("terrainSpecularColor", 0.0f, 0.0f, 0.0f);
	terrainRenderShader.setVec3("waterColor", 0.1f, 0.6f, 1.0f);
	terrainRenderShader.setVec3("waterSpecularColor", 1.0f, 1.0f, 1.0f);
	// set terrain render light properties
	terrainRenderShader.setVec3("dirLight.direction", -0.0f, -1.0f, -0.0f);
	terrainRenderShader.setVec3("dirLight.ambient", 0.4f, 0.4f, 0.4f);
	terrainRenderShader.setVec3("dirLight.diffuse", 0.5f, 0.5f, 0.5f);
	terrainRenderShader.setVec3("dirLight.specular", 1.0f, 1.0f, 1.0f);

	// water render shader static properties
	waterRenderShader.use();
	waterRenderShader.setFloat("size", MESH_TOTAL_SIZE);
	waterRenderShader.setFloat("terrainShininess", 1.0f);
	waterRenderShader.setFloat("waterShininess", 64.0f);
	waterRenderShader.setVec3("terrainColor", 0.87f, 0.85f, 0.6f);
	waterRenderShader.setVec3("vegetationColor", 0.31f, 0.5f, 0.1f);
	waterRenderShader.setVec3("terrainSpecularColor", 0.0f, 0.0f, 0.0f);
	waterRenderShader.setVec3("waterColor", 0.1f, 0.6f, 1.0f);
	waterRenderShader.setVec3("waterSpecularColor", 1.0f, 1.0f, 1.0f);
	// set terrain render light properties
	waterRenderShader.setVec3("dirLight.direction", -0.0f, -1.0f, -0.0f);
	waterRenderShader.setVec3("dirLight.ambient", 0.4f, 0.4f, 0.4f);
	waterRenderShader.setVec3("dirLight.diffuse", 0.5f, 0.5f, 0.5f);
	waterRenderShader.setVec3("dirLight.specular", 1.0f, 1.0f, 1.0f);

	float sourceFlowTime = 0;
	float rainFallTime = 0;

	glm::mat4 projection;
	glm::mat4 view;
	glm::mat4 model;

	glfwSetTime(0);

	// render loop
	// -----------
	while (!glfwWindowShouldClose(window))
	{
		// per-frame time logic
		// --------------------
		float currentFrame = (float)glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		//cout << 1 / deltaTime << endl;

		// input
		// -----
		processInput(window);

		// First Pass: Water Increment Step
		waterIncrementComputeShader.use();
		// Link tempCDTextureID to the output (binding = 0) of the water increment shader
		glBindImageTexture(0, tempCDTextureID, 0, GL_FALSE, 0, GL_WRITE_ONLY, INTERNAL_TEXTURE_FORMAT);
		// Link tempWTextureID to the output (binding = 1) of the water increment shader
		glBindImageTexture(1, tempWTextureID, 0, GL_FALSE, 0, GL_WRITE_ONLY, INTERNAL_TEXTURE_FORMAT);
		// Link CDTextureID to binding = 2 in water increment shader
		glBindImageTexture(2, CDTextureID, 0, GL_FALSE, 0, GL_READ_ONLY, INTERNAL_TEXTURE_FORMAT);
		// Link WTextureID to binding = 3 in water increment shader
		glBindImageTexture(3, WTextureID, 0, GL_FALSE, 0, GL_READ_ONLY, INTERNAL_TEXTURE_FORMAT);
		// Set Water Increment Shader Properties
		if (isSourceFlow && sourceFlowTime < SOURCE_FLOW_CUTOFF_TIME) {
			sourceFlowTime += deltaTime;
		}
		else if (isSourceFlow) {
			isSourceFlow = false;
			waterIncrementComputeShader.setBool("isSourceFlow", isSourceFlow);
		}

		if (isRain && rainFallTime < RAIN_CUTOFF_TIME) {
			rainFallTime += deltaTime;
			for (int i = 0; i < numberOfRaindrops; i++) {
				string raindrop = "raindrops[";
				raindrop += std::to_string(i);
				string position = "].position";
				string radius = "].radius";
				string increment = "].Kir";

				float Kir = (float)glm::linearRand(3, 5);

				rainRadius = MESH_WIDTH / 100;

				int x = glm::linearRand(rainRadius, (int)MESH_WIDTH - rainRadius);
				int y = glm::linearRand(rainRadius, (int)MESH_HEIGHT - rainRadius);

				waterIncrementComputeShader.setIVec2(raindrop + position, x, y);
				waterIncrementComputeShader.setInt(raindrop + radius, rainRadius);
				waterIncrementComputeShader.setFloat(raindrop + increment, Kir);
			}
		}
		else if (isRain) {
			isRain = false;
			waterIncrementComputeShader.setBool("isRain", isRain);
		}

		glDispatchCompute((GLuint)NUM_GROUPS_X, (GLuint)NUM_GROUPS_Y, NUM_GROUPS_Z);
		// Prevent from moving on until all compute shader calculations are done
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

		// Second Pass: Flux(Water and Regolith) Update Step
		fluxUpdateComputeShader.use();
		// Link tempFTextureID to the output (binding = 0) in flux update shader
		glBindImageTexture(0, tempFTextureID, 0, GL_FALSE, 0, GL_WRITE_ONLY, INTERNAL_TEXTURE_FORMAT);
		// Link tempRTextureID to the output (binding = 1) in flux update shader
		glBindImageTexture(1, tempRTextureID, 0, GL_FALSE, 0, GL_WRITE_ONLY, INTERNAL_TEXTURE_FORMAT);
		// Link tempCDTextureID to binding = 2 in flux update shader
		glBindImageTexture(2, tempCDTextureID, 0, GL_FALSE, 0, GL_READ_ONLY, INTERNAL_TEXTURE_FORMAT);
		// Link tempWTextureID to binding = 3 in flux update shader
		glBindImageTexture(3, tempWTextureID, 0, GL_FALSE, 0, GL_READ_ONLY, INTERNAL_TEXTURE_FORMAT);
		// Link FTextureID to binding = 4 in flux update shader
		glBindImageTexture(4, FTextureID, 0, GL_FALSE, 0, GL_READ_ONLY, INTERNAL_TEXTURE_FORMAT);
		// Link RTextureID to binding = 5 in flux update shader
		glBindImageTexture(5, RTextureID, 0, GL_FALSE, 0, GL_READ_ONLY, INTERNAL_TEXTURE_FORMAT);

		glDispatchCompute((GLuint)NUM_GROUPS_X, (GLuint)NUM_GROUPS_Y, NUM_GROUPS_Z);
		// Prevent from moving on until all compute shader calculations are done
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

		/*
		// Second Pass: Flux Update Step
		fluxUpdateComputeShader.use();
		// Link tempFTextureID to the output (binding = 0) in flux update shader
		glBindImageTexture(0, tempFTextureID, 0, GL_FALSE, 0, GL_WRITE_ONLY, INTERNAL_TEXTURE_FORMAT);
		// Link tempCDTextureID to binding = 1 in flux update shader
		glBindImageTexture(1, tempCDTextureID, 0, GL_FALSE, 0, GL_READ_ONLY, INTERNAL_TEXTURE_FORMAT);
		// Link FTextureID to binding = 2 in flux update shader
		glBindImageTexture(2, FTextureID, 0, GL_FALSE, 0, GL_READ_ONLY, INTERNAL_TEXTURE_FORMAT);
		
		glDispatchCompute((GLuint)NUM_GROUPS_X, (GLuint)NUM_GROUPS_Y, NUM_GROUPS_Z);
		// Prevent from moving on until all compute shader calculations are done
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

		// Third Pass: Regolith Flux Update Step
		regolithFluxUpdateComputeShader.use();
		// Link tempFTextureID to binding = 0 in flux update shader
		glBindImageTexture(0, tempRTextureID, 0, GL_FALSE, 0, GL_WRITE_ONLY, INTERNAL_TEXTURE_FORMAT);
		// Link tempCDTextureID to binding = 1 in flux update shader
		glBindImageTexture(1, tempCDTextureID, 0, GL_FALSE, 0, GL_READ_ONLY, INTERNAL_TEXTURE_FORMAT);
		// Link FTextureID to binding = 2 in flux update shader
		glBindImageTexture(2, RTextureID, 0, GL_FALSE, 0, GL_READ_ONLY, INTERNAL_TEXTURE_FORMAT);

		glDispatchCompute((GLuint)NUM_GROUPS_X, (GLuint)NUM_GROUPS_Y, NUM_GROUPS_Z);
		// Prevent from moving on until all compute shader calculations are done
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
		*/

		// Third Pass: Height (Water and Regolith) Update Step
		heightUpdateComputeShader.use();
		// Link CDTextureID to binding = 0 in water height update shader
		glBindImageTexture(0, CDTextureID, 0, GL_FALSE, 0, GL_WRITE_ONLY, INTERNAL_TEXTURE_FORMAT);
		// Link tempCDTextureID to binding = 1 in water height update shader
		glBindImageTexture(1, tempCDTextureID, 0, GL_FALSE, 0, GL_READ_ONLY, INTERNAL_TEXTURE_FORMAT);
		// Link tempFTextureID to binding = 2 in water height update shader
		glBindImageTexture(2, tempFTextureID, 0, GL_FALSE, 0, GL_READ_ONLY, INTERNAL_TEXTURE_FORMAT);
		// Link tempRTextureID to binding = 2 in water height update shader
		glBindImageTexture(3, tempRTextureID, 0, GL_FALSE, 0, GL_READ_ONLY, INTERNAL_TEXTURE_FORMAT);

		glDispatchCompute((GLuint)NUM_GROUPS_X, (GLuint)NUM_GROUPS_Y, NUM_GROUPS_Z);
		// Prevent from moving on until all compute shader calculations are done
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

		/*
		// Fourth Pass: Water Height Update Step
		waterHeightUpdateComputeShader.use();
		// Link CDTextureID to binding = 0 in water height update shader
		glBindImageTexture(0, CDTextureID, 0, GL_FALSE, 0, GL_WRITE_ONLY, INTERNAL_TEXTURE_FORMAT);
		// Link tempCDTextureID to binding = 1 in water height update shader
		glBindImageTexture(1, tempCDTextureID, 0, GL_FALSE, 0, GL_READ_ONLY, INTERNAL_TEXTURE_FORMAT);
		// Link tempFTextureID to binding = 2 in water height update shader
		glBindImageTexture(2, tempFTextureID, 0, GL_FALSE, 0, GL_READ_ONLY, INTERNAL_TEXTURE_FORMAT);
		
		glDispatchCompute((GLuint)NUM_GROUPS_X, (GLuint)NUM_GROUPS_Y, NUM_GROUPS_Z);
		// Prevent from moving on until all compute shader calculations are done
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

		// Fifth Pass: Regolith Height Update Step
		regolithHeightUpdateComputeShader.use();
		// Link tempCDTextureID to binding = 0 in regolith height update shader
		glBindImageTexture(0, tempCDTextureID, 0, GL_FALSE, 0, GL_WRITE_ONLY, INTERNAL_TEXTURE_FORMAT);
		// Link CDTextureID to binding = 1 in regolith height update shader
		glBindImageTexture(1, CDTextureID, 0, GL_FALSE, 0, GL_READ_ONLY, INTERNAL_TEXTURE_FORMAT);
		// Link tempFTextureID to binding = 2 in regolith height update shader
		glBindImageTexture(2, tempRTextureID, 0, GL_FALSE, 0, GL_READ_ONLY, INTERNAL_TEXTURE_FORMAT);

		glDispatchCompute((GLuint)NUM_GROUPS_X, (GLuint)NUM_GROUPS_Y, NUM_GROUPS_Z);
		// Prevent from moving on until all compute shader calculations are done
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
		*/
	
		// Sixth Pass: Velocity Field Update Step
		velocityFieldUpdateComputeShader.use();
		// Link tempVTextureID to binding = 0 in velocity field update shader
		glBindImageTexture(0, tempVTextureID, 0, GL_FALSE, 0, GL_WRITE_ONLY, INTERNAL_TEXTURE_FORMAT);
		// Link tempCDTextureID to binding = 1 in velocity field update shader
		glBindImageTexture(1, tempCDTextureID, 0, GL_FALSE, 0, GL_READ_ONLY, INTERNAL_TEXTURE_FORMAT);
		// Link CDTextureID to binding = 2 in velocity field update shader
		glBindImageTexture(2, CDTextureID, 0, GL_FALSE, 0, GL_READ_ONLY, INTERNAL_TEXTURE_FORMAT);
		// Link tempFTextureID to binding = 3 in velocity field update shader
		glBindImageTexture(3, tempFTextureID, 0, GL_FALSE, 0, GL_READ_ONLY, INTERNAL_TEXTURE_FORMAT);
		// Link VTextureID to binding = 4 in velocity field update shader
		glBindImageTexture(4, VTextureID, 0, GL_FALSE, 0, GL_READ_ONLY, INTERNAL_TEXTURE_FORMAT);
		
		glDispatchCompute((GLuint)NUM_GROUPS_X, (GLuint)NUM_GROUPS_Y, NUM_GROUPS_Z);
		// Prevent from moving on until all compute shader calculations are done
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

		// Sixth Pass: Soil Flow Step
		soilFlowComputeShader.use();
		// Link STextureID to binding = 0 in soil flow shader
		glBindImageTexture(0, STextureID, 0, GL_FALSE, 0, GL_WRITE_ONLY, INTERNAL_TEXTURE_FORMAT);
		// Link SCTextureID to binding = 1 in soil flow shader
		glBindImageTexture(1, SCTextureID, 0, GL_FALSE, 0, GL_WRITE_ONLY, INTERNAL_TEXTURE_FORMAT);
		// Link CDTextureID to binding = 2 in soil flow shader
		glBindImageTexture(2, CDTextureID, 0, GL_FALSE, 0, GL_READ_ONLY, INTERNAL_TEXTURE_FORMAT);
		// Link tempWTextureID to binding = 3 in soil flow shader
		glBindImageTexture(3, tempWTextureID, 0, GL_FALSE, 0, GL_READ_ONLY, INTERNAL_TEXTURE_FORMAT);

		glDispatchCompute((GLuint)NUM_GROUPS_X, (GLuint)NUM_GROUPS_Y, NUM_GROUPS_Z);
		// Prevent from moving on until all compute shader calculations are done
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

		// Seventh Pass: Sediment Erosion/Deposition Step
		sedimentErosionAndDepositionComputeShader.use();
		// Link tempCDTextureID to output (binding = 0) in sediment erosion/deposition shader
		glBindImageTexture(0, tempCDTextureID, 0, GL_FALSE, 0, GL_WRITE_ONLY, INTERNAL_TEXTURE_FORMAT);
		// Link WTextureID to output (binding = 1) in sediment erosion/deposition shader
		glBindImageTexture(1, WTextureID, 0, GL_FALSE, 0, GL_WRITE_ONLY, INTERNAL_TEXTURE_FORMAT);
		// Link CDTextureID to binding = 2 in sediment erosion/deposition shader
		glBindImageTexture(2, CDTextureID, 0, GL_FALSE, 0, GL_READ_ONLY, INTERNAL_TEXTURE_FORMAT);
		// Link tempWTextureID to binding = 3 in sediment erosion/deposition shader
		glBindImageTexture(3, tempWTextureID, 0, GL_FALSE, 0, GL_READ_ONLY, INTERNAL_TEXTURE_FORMAT);
		// Link tempVTextureID to binding = 4 in sediment erosion/deposition shader
		glBindImageTexture(4, tempVTextureID, 0, GL_FALSE, 0, GL_READ_ONLY, INTERNAL_TEXTURE_FORMAT);
		
		glDispatchCompute((GLuint)NUM_GROUPS_X, (GLuint)NUM_GROUPS_Y, NUM_GROUPS_Z);
		// Prevent from moving on until all compute shader calculations are done
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

		// Eighth Pass: Sediment Transportation Step
		sedimentTransportationComputeShader.use();
		// Link tempWTextureID to binding = 0 in sediment transportation shader
		glBindImageTexture(0, tempWTextureID, 0, GL_FALSE, 0, GL_WRITE_ONLY, INTERNAL_TEXTURE_FORMAT);
		// Link WTextureID to binding = 1 in sediment transportation shader
		glBindImageTexture(1, WTextureID, 0, GL_FALSE, 0, GL_READ_ONLY, INTERNAL_TEXTURE_FORMAT);
		// Link tempVTextureID to binding = 2 in sediment transportation shader
		glBindImageTexture(2, tempVTextureID, 0, GL_FALSE, 0, GL_READ_ONLY, INTERNAL_TEXTURE_FORMAT);
		
		glDispatchCompute((GLuint)NUM_GROUPS_X, (GLuint)NUM_GROUPS_Y, NUM_GROUPS_Z);
		// Prevent from moving on until all compute shader calculations are done
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

		// Sixth Pass: Soil Flow Deposition Step
		soilFlowDepositionComputeShader.use();
		// Link CDTextureID to binding = 0 in soil flow deposition shader
		glBindImageTexture(0, CDTextureID, 0, GL_FALSE, 0, GL_WRITE_ONLY, INTERNAL_TEXTURE_FORMAT);
		// Link tempCDTextureID to binding = 1 in soil flow deposition shader
		glBindImageTexture(1, tempCDTextureID, 0, GL_FALSE, 0, GL_READ_ONLY, INTERNAL_TEXTURE_FORMAT);
		// Link SCTextureID to binding = 2 in soil flow deposition shader
		glBindImageTexture(2, STextureID, 0, GL_FALSE, 0, GL_READ_ONLY, INTERNAL_TEXTURE_FORMAT);
		// Link CDTextureID to binding = 3 in soil flow deposition shader
		glBindImageTexture(3, SCTextureID, 0, GL_FALSE, 0, GL_READ_ONLY, INTERNAL_TEXTURE_FORMAT);

		glDispatchCompute((GLuint)NUM_GROUPS_X, (GLuint)NUM_GROUPS_Y, NUM_GROUPS_Z);
		// Prevent from moving on until all compute shader calculations are done
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

		// Ninth Pass: Evaporation Step
		evaporationComputeShader.use();
		// Link tempCDTextureID to the output (binding = 0) of the evaporation shader
		glBindImageTexture(0, tempCDTextureID, 0, GL_FALSE, 0, GL_WRITE_ONLY, INTERNAL_TEXTURE_FORMAT);
		// Link CDTextureID to binding = 1 in the evaporation shader
		glBindImageTexture(1, CDTextureID, 0, GL_FALSE, 0, GL_READ_ONLY, INTERNAL_TEXTURE_FORMAT);
		
		glDispatchCompute((GLuint)NUM_GROUPS_X, (GLuint)NUM_GROUPS_Y, NUM_GROUPS_Z);
		// Prevent from moving on until all compute shader calculations are done
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

		// Final Pass: Terrain Render Step
		// render
		// ------
		glClearColor(0.8f, 0.8f, 0.8f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);	

		// projection matrix
		projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);

		// camera/view transformation
		view = camera.GetViewMatrix();

		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(-(MESH_TOTAL_SIZE / 2.0f), 0.0f, -(MESH_TOTAL_SIZE / 2.0f)));

		// activate terrain render shader
		terrainRenderShader.use();
		// set shader properties
		terrainRenderShader.setVec3("viewPos", camera.Position);
		terrainRenderShader.setInt("columnDataTexture", 0);
		terrainRenderShader.setInt("waterDataTexture", 1);
		
		terrainRenderShader.setMat4("projection", projection);		
		terrainRenderShader.setMat4("view", view);				
		terrainRenderShader.setMat4("model", model);
		
		// bind texture
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, tempCDTextureID);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, tempWTextureID);

		// render mesh
		glBindVertexArray(meshVAO);
		glDrawElements(GL_TRIANGLES, meshIndices.size(), GL_UNSIGNED_INT, 0);
		glBindVertexArray(0);

		// activate water render shader
		waterRenderShader.use();
		// set shader properties
		waterRenderShader.setVec3("viewPos", camera.Position);
		waterRenderShader.setInt("columnDataTexture", 0);
		waterRenderShader.setInt("waterDataTexture", 1);

		waterRenderShader.setMat4("projection", projection);
		waterRenderShader.setMat4("view", view);
		waterRenderShader.setMat4("model", model);

		// bind texture
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, tempCDTextureID);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, tempWTextureID);

		// render mesh
		glBindVertexArray(wMeshVAO);
		glDrawElements(GL_TRIANGLES, meshIndices.size(), GL_UNSIGNED_INT, 0);
		glBindVertexArray(0);

		// Swap info in tempCDTexture to CDTexture
		swapBuffersComputeShader.use();
		// Link CDTextureID to the output (binding = 0) of the swap buffers shader
		glBindImageTexture(0, CDTextureID, 0, GL_FALSE, 0, GL_WRITE_ONLY, INTERNAL_TEXTURE_FORMAT);
		// Link tempCDTextureID to binding = 1 in the swap buffers shader
		glBindImageTexture(1, tempCDTextureID, 0, GL_FALSE, 0, GL_READ_ONLY, INTERNAL_TEXTURE_FORMAT);
		
		glDispatchCompute((GLuint)NUM_GROUPS_X, (GLuint)NUM_GROUPS_Y, NUM_GROUPS_Z);
		// Prevent from moving on until all compute shader calculations are done
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
		

		// Swap info in tempWTexture to WTexture
		swapBuffersComputeShader.use();
		// Link CDTextureID to the output (binding = 0) of the swap buffers shader
		glBindImageTexture(0, WTextureID, 0, GL_FALSE, 0, GL_WRITE_ONLY, INTERNAL_TEXTURE_FORMAT);
		// Link tempCDTextureID to binding = 1 in the swap buffers shader
		glBindImageTexture(1, tempWTextureID, 0, GL_FALSE, 0, GL_READ_ONLY, INTERNAL_TEXTURE_FORMAT);

		glDispatchCompute((GLuint)NUM_GROUPS_X, (GLuint)NUM_GROUPS_Y, NUM_GROUPS_Z);
		// Prevent from moving on until all compute shader calculations are done
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

		// Swap info in tempFTexture to FTexture
		swapBuffersComputeShader.use();
		// Link FTextureID to the output (binding = 0) of the swap buffers shader
		glBindImageTexture(0, FTextureID, 0, GL_FALSE, 0, GL_WRITE_ONLY, INTERNAL_TEXTURE_FORMAT);
		// Link tempFTextureID to binding = 1 in the swap buffers shader
		glBindImageTexture(1, tempFTextureID, 0, GL_FALSE, 0, GL_READ_ONLY, INTERNAL_TEXTURE_FORMAT);
		
		glDispatchCompute((GLuint)NUM_GROUPS_X, (GLuint)NUM_GROUPS_Y, NUM_GROUPS_Z);
		// Prevent from moving on until all compute shader calculations are done
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

		// Swap info in tempRTexture to RTexture
		swapBuffersComputeShader.use();
		// Link VTextureID to the output (binding = 0) of the swap buffers shader
		glBindImageTexture(0, RTextureID, 0, GL_FALSE, 0, GL_WRITE_ONLY, INTERNAL_TEXTURE_FORMAT);
		// Link tempVTextureID to binding = 1 in the swap buffers shader
		glBindImageTexture(1, tempRTextureID, 0, GL_FALSE, 0, GL_READ_ONLY, INTERNAL_TEXTURE_FORMAT);

		glDispatchCompute((GLuint)NUM_GROUPS_X, (GLuint)NUM_GROUPS_Y, NUM_GROUPS_Z);
		// Prevent from moving on until all compute shader calculations are done
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

		// Swap info in tempVTexture to VTexture
		swapBuffersComputeShader.use();
		// Link VTextureID to the output (binding = 0) of the swap buffers shader
		glBindImageTexture(0, VTextureID, 0, GL_FALSE, 0, GL_WRITE_ONLY, INTERNAL_TEXTURE_FORMAT);
		// Link tempVTextureID to binding = 1 in the swap buffers shader
		glBindImageTexture(1, tempVTextureID, 0, GL_FALSE, 0, GL_READ_ONLY, INTERNAL_TEXTURE_FORMAT);
		
		glDispatchCompute((GLuint)NUM_GROUPS_X, (GLuint)NUM_GROUPS_Y, NUM_GROUPS_Z);
		// Prevent from moving on until all compute shader calculations are done
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);		

		// glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
		// -------------------------------------------------------------------------------
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	// Deallocate all opengl resources
	// -----------------------------------------------------------
	glDeleteVertexArrays(1, &meshVAO);
	glDeleteBuffers(1, &meshVBO);
	glDeleteBuffers(1, &meshEBO);
	glDeleteProgram(waterIncrementComputeShader.ID);
	glDeleteProgram(fluxUpdateComputeShader.ID);
	//glDeleteProgram(waterHeightUpdateComputeShader.ID);
	glDeleteProgram(velocityFieldUpdateComputeShader.ID);
	glDeleteProgram(soilFlowComputeShader.ID);
	glDeleteProgram(sedimentErosionAndDepositionComputeShader.ID);
	glDeleteProgram(sedimentTransportationComputeShader.ID);
	glDeleteProgram(soilFlowDepositionComputeShader.ID);
	glDeleteProgram(evaporationComputeShader.ID);
	glDeleteProgram(terrainRenderShader.ID);
	glDeleteProgram(swapBuffersComputeShader.ID);

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
		float currentPressTime = (float)glfwGetTime();

		if (currentPressTime - pLastPressTime > KEY_PRESS_DELAY) {
			pLastPressTime = currentPressTime;
			drawPolygon = !drawPolygon;
			if (drawPolygon) {
				glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			}
			else {
				glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			}
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
		lastX = (float)xpos;
		lastY = (float)ypos;
		firstMouse = false;
	}

	float xoffset = (float)xpos - lastX;
	float yoffset = lastY - (float)ypos; // reversed since y-coordinates go from bottom to top

	lastX = (float)xpos;
	lastY = (float)ypos;

	camera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	camera.ProcessMouseScroll((float)yoffset);
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
	for (unsigned int j = 0; j < height; j++) {
		for (unsigned int i = 0; i < width; i++) {
			// vertex position
			vertexList.push_back(i * MESH_SCALE);
			vertexList.push_back(0);
			vertexList.push_back(j * MESH_SCALE);

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

	for (unsigned int j = 0; j < height - 1; j++) {
		for (unsigned int i = 0; i < width - 1; i++) {
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

	// create texture for initial terrain data
	glGenTextures(1, &CDTextureID);
	glBindTexture(GL_TEXTURE_2D, CDTextureID);
	glTexImage2D(GL_TEXTURE_2D, 0, INTERNAL_TEXTURE_FORMAT, MESH_WIDTH, MESH_HEIGHT, 0, TEXTURE_FORMAT, GL_FLOAT, &CDTexture[0]);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	// create texture for initial water data
	glGenTextures(1, &WTextureID);
	glBindTexture(GL_TEXTURE_2D, WTextureID);
	glTexImage2D(GL_TEXTURE_2D, 0, INTERNAL_TEXTURE_FORMAT, MESH_WIDTH, MESH_HEIGHT, 0, TEXTURE_FORMAT, GL_FLOAT, &EmptyTexture[0]);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	// create texture for initial flux
	glGenTextures(1, &FTextureID);
	glBindTexture(GL_TEXTURE_2D, FTextureID);
	glTexImage2D(GL_TEXTURE_2D, 0, INTERNAL_TEXTURE_FORMAT, MESH_WIDTH, MESH_HEIGHT, 0, TEXTURE_FORMAT, GL_FLOAT, &EmptyTexture[0]);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	// create texture for initial velocity
	glGenTextures(1, &VTextureID);
	glBindTexture(GL_TEXTURE_2D, VTextureID);
	glTexImage2D(GL_TEXTURE_2D, 0, INTERNAL_TEXTURE_FORMAT, MESH_WIDTH, MESH_HEIGHT, 0, TEXTURE_FORMAT, GL_FLOAT, &EmptyTexture[0]);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	// create texture for initial regolith flux
	glGenTextures(1, &RTextureID);
	glBindTexture(GL_TEXTURE_2D, RTextureID);
	glTexImage2D(GL_TEXTURE_2D, 0, INTERNAL_TEXTURE_FORMAT, MESH_WIDTH, MESH_HEIGHT, 0, TEXTURE_FORMAT, GL_FLOAT, &EmptyTexture[0]);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	// create texture for initial sediment flux (Left, Right, Top, Bottom)
	glGenTextures(1, &STextureID);
	glBindTexture(GL_TEXTURE_2D, STextureID);
	glTexImage2D(GL_TEXTURE_2D, 0, INTERNAL_TEXTURE_FORMAT, MESH_WIDTH, MESH_HEIGHT, 0, TEXTURE_FORMAT, GL_FLOAT, &EmptyTexture[0]);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	// create texture for initial sediment corner flux (Bottom Left, Bottom Right, Top Left, Top Right)
	glGenTextures(1, &SCTextureID);
	glBindTexture(GL_TEXTURE_2D, SCTextureID);
	glTexImage2D(GL_TEXTURE_2D, 0, INTERNAL_TEXTURE_FORMAT, MESH_WIDTH, MESH_HEIGHT, 0, TEXTURE_FORMAT, GL_FLOAT, &EmptyTexture[0]);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	// create texture for terrain data output
	glGenTextures(1, &tempCDTextureID);
	glBindTexture(GL_TEXTURE_2D, tempCDTextureID);
	glTexImage2D(GL_TEXTURE_2D, 0, INTERNAL_TEXTURE_FORMAT, MESH_WIDTH, MESH_HEIGHT, 0, TEXTURE_FORMAT, GL_FLOAT, NULL);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	// create texture for water data output
	glGenTextures(1, &tempWTextureID);
	glBindTexture(GL_TEXTURE_2D, tempWTextureID);
	glTexImage2D(GL_TEXTURE_2D, 0, INTERNAL_TEXTURE_FORMAT, MESH_WIDTH, MESH_HEIGHT, 0, TEXTURE_FORMAT, GL_FLOAT, NULL);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	// create texture for flux output
	glGenTextures(1, &tempFTextureID);
	glBindTexture(GL_TEXTURE_2D, tempFTextureID);
	glTexImage2D(GL_TEXTURE_2D, 0, INTERNAL_TEXTURE_FORMAT, MESH_WIDTH, MESH_HEIGHT, 0, TEXTURE_FORMAT, GL_FLOAT, NULL);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	// create texture for velocity output
	glGenTextures(1, &tempVTextureID);
	glBindTexture(GL_TEXTURE_2D, tempVTextureID);
	glTexImage2D(GL_TEXTURE_2D, 0, INTERNAL_TEXTURE_FORMAT, MESH_WIDTH, MESH_HEIGHT, 0, TEXTURE_FORMAT, GL_FLOAT, NULL);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	// create texture for regolith flux output
	glGenTextures(1, &tempRTextureID);
	glBindTexture(GL_TEXTURE_2D, tempRTextureID);
	glTexImage2D(GL_TEXTURE_2D, 0, INTERNAL_TEXTURE_FORMAT, MESH_WIDTH, MESH_HEIGHT, 0, TEXTURE_FORMAT, GL_FLOAT, NULL);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	// create texture for sediment flux output
	glGenTextures(1, &tempSTextureID);
	glBindTexture(GL_TEXTURE_2D, tempSTextureID);
	glTexImage2D(GL_TEXTURE_2D, 0, INTERNAL_TEXTURE_FORMAT, MESH_WIDTH, MESH_HEIGHT, 0, TEXTURE_FORMAT, GL_FLOAT, NULL);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	// create texture for sediment corner flux output
	glGenTextures(1, &tempSCTextureID);
	glBindTexture(GL_TEXTURE_2D, tempSCTextureID);
	glTexImage2D(GL_TEXTURE_2D, 0, INTERNAL_TEXTURE_FORMAT, MESH_WIDTH, MESH_HEIGHT, 0, TEXTURE_FORMAT, GL_FLOAT, NULL);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
}

void GenerateBaseTextures(unsigned int width, unsigned int height) {
	unsigned int location;

	FastNoise terrainNoise;
	terrainNoise.SetNoiseType(FastNoise::Perlin);
	terrainNoise.SetSeed(terrainSeed);

	FastNoise vegetationNoise;
	vegetationNoise.SetNoiseType(FastNoise::Perlin);
	vegetationNoise.SetSeed(vegetationSeed);

	for (unsigned int j = 0; j < height; j++) {
		for (unsigned int i = 0; i < width; i++) {
			location = GetLocation(i, j);

			float iCoord = (float)i * 128 / MESH_WIDTH;
			float jCoord = (float)j * 128 / MESH_HEIGHT;

			float terrainFrequencyScale = 2;
			float terrainNoiseValue = terrainNoise.GetNoise(terrainFrequencyScale * iCoord, terrainFrequencyScale * jCoord);
			terrainNoiseValue += 0.5f * terrainNoise.GetNoise(terrainFrequencyScale * 2 * iCoord, terrainFrequencyScale * 2 * jCoord);
			terrainNoiseValue += 0.25f * terrainNoise.GetNoise(terrainFrequencyScale * 4 * iCoord, terrainFrequencyScale * 4 * jCoord);
			terrainNoiseValue += 0.125f * terrainNoise.GetNoise(terrainFrequencyScale * 8 * iCoord, terrainFrequencyScale * 8 * jCoord);

			terrainNoiseValue /= HEIGHT_SCALING_VALUE;

			float vegetationFrequencyScale = 4;
			float vegetationNoiseValue = 0;
			if (isVegetation) {
				if (isVegetationSeed) {
					vegetationNoiseValue = vegetationNoise.GetNoise(iCoord * vegetationFrequencyScale * 0.93f, jCoord * vegetationFrequencyScale * 0.93f);
					vegetationNoiseValue += 0.7f * vegetationNoise.GetNoise(iCoord / 0.7f * vegetationFrequencyScale, jCoord / 0.7f * vegetationFrequencyScale);
					vegetationNoiseValue += 0.4f * vegetationNoise.GetNoise(iCoord / 0.4f * vegetationFrequencyScale, jCoord / 0.4f * vegetationFrequencyScale);

					vegetationNoiseValue = max(0.0f, vegetationNoiseValue);
					vegetationNoiseValue /= HEIGHT_SCALING_VALUE;
				}
			}

			//////////////////////////////
			// Initial Column Data Texture (CDTexture)
			//////////////////////////////
			// R = water height value
			// G = regolith height value
			// B = vegetation height value
			// A = terrain height value
			//////////////////////////////
			CDTexture[location + 0] = 0.0f; 
			CDTexture[location + 1] = 0.0f; 
			CDTexture[location + 2] = vegetationNoiseValue;
			CDTexture[location + 3] = terrainNoiseValue - vegetationNoiseValue;
		
			////////////////////////////////////////////////////
			// All of the textures below are initially empty (0)
			////////////////////////////////////////////////////

			//////////////////////////////
			// Initial Water Data Texture (WTexture)
			//////////////////////////////
			// R = terrain sediment value
			// G = dead vegetation sediment value
			// B = time covered in water value
			// A = dead vegetation height value
			//////////////////////////////
			//////////////////////////////
			// Initial Flux Texture (FTexture)
			//////////////////////////////
			// R = left flux value
			// G = right flux value
			// B = top flux value
			// A = bottom flux value
			//////////////////////////////
			//////////////////////////////
			// Initial Velocity Texture (VTexture)
			//////////////////////////////
			// R = velocity in x-direction
			// G = velocity in y-direction
			// B = 
			// A = 
			//////////////////////////////
			EmptyTexture[location + 0] = 0.0f; 
			EmptyTexture[location + 1] = 0.0f; 
			EmptyTexture[location + 2] = 0.0f; 
			EmptyTexture[location + 3] = 0.0f; 
		}
	}

	if (!isVegetationSeed) {
		float vegetationValue;
		
		float centerHeight;
		float leftHeight;
		float rightHeight;
		float topHeight;
		float bottomHeight;

		float lrHeightDifference;
		float tbHeightDifference;
		float totalHeightDifference;

		float MAX_HEIGHT_DIFFERENCE = 0.03f / HEIGHT_SCALING_VALUE;

		float percentage;

		for (unsigned int j = 0; j < height; j++) {
			for (unsigned int i = 0; i < width; i++) {
				vegetationValue = maxVegetationValue;

				location = GetLocation(i, j);
				centerHeight = CDTexture[location + 3] + CDTexture[location + 2];
				location = GetLocation(i - 1, j);
				leftHeight = CDTexture[location + 3] + CDTexture[location + 2];
				location = GetLocation(i + 1, j);
				rightHeight = CDTexture[location + 3] + CDTexture[location + 2];
				location = GetLocation(i, j + 1);
				topHeight = CDTexture[location + 3] + CDTexture[location + 2];
				location = GetLocation(i, j - 1);
				bottomHeight = CDTexture[location + 3] + CDTexture[location + 2];
				
				lrHeightDifference = abs(leftHeight - rightHeight);
				tbHeightDifference = abs(topHeight - bottomHeight);

				totalHeightDifference = lrHeightDifference + tbHeightDifference;

				/*if (centerHeight < 0.1f / HEIGHT_SCALING_VALUE) {
					CDTexture[location + 2] = vegetationValue;
					CDTexture[location + 3] -= vegetationValue;
				}*/
				
				if (totalHeightDifference < MAX_HEIGHT_DIFFERENCE) {
					percentage = min(1.0f, ((MAX_HEIGHT_DIFFERENCE - totalHeightDifference) / MAX_HEIGHT_DIFFERENCE) + 0.4f);
					vegetationValue *= percentage;
					CDTexture[location + 2] = vegetationValue;
					CDTexture[location + 3] -= vegetationValue;
				}
			}
		}
	}
}

unsigned int GetLocation(unsigned int i, unsigned int j) {
	unsigned int x = i;
	unsigned int y = j;

	if (i < 0) {
		x = 0;
	}
	else if (i >= MESH_WIDTH) {
		x = MESH_WIDTH - 1;
	}

	if (j < 0) {
		y = 0;
	}
	else if (j >= MESH_HEIGHT) {
		y = MESH_HEIGHT - 1;
	}

	return (x + y * MESH_WIDTH) * 4;
}

float TalusHeightDifference(float angle) {
	float height = (1.0f / MESH_WIDTH) * atan(glm::radians(angle));
	//cout << "Talus Height Difference: " << height << endl;
	return height;
}
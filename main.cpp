#ifndef PI
#define PI 3.141592f
#endif

#include <GL/glew.h>
#include <GL/freeglut.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <stdio.h>
#include <stdlib.h> 
#include <vector>

#include "Terrain.h"
#include "Skybox.h"
#include "Object.h"
#include "Camera.h"
#include "WaterFramebuffer.h"
#include "SimpleShaders.h"
#include "TerrainShaders.h"
#include "WaterShaders.h"
#include "SkyboxShaders.h"
#include "ObjectsShaders.h"

using namespace glm;

// Dimensions of the windows
int width = 1280;
int height = 720;

const int terrainResolution = 256;	// Size of the seafloor terrain
const int tileNumber = 15;			// No. of tiles of terrain
const int tileSea = 10;				// No. of tiles of water surface
const float waterheight = 50.0f;	// Height of water surface	
float amplitude = 15.0f;			// Amplitude for noise function
float frequency = 0.02f;			// frequency for noise function
float waterFrequency = 0.08f;		// Parameters for height generation of water surface
float waterAmplitude = 2.5f;

const char* seafloorFrag = "Shaders/SeafloorShader.frag";	//location of shader files
const char* seafloorVert = "Shaders/SeafloorShader.vert";
const char* waterFrag = "Shaders/WaterShader.frag";
const char* waterVert = "Shaders/WaterShader.vert";
const char* skyboxFrag = "Shaders/skybox.frag";
const char* skyboxVert = "Shaders/skybox.vert";
const char* objFrag = "Shaders/ObjectsShader.frag";
const char* objVert = "Shaders/ObjectsShader.vert";

// Texture locations
std::vector<std::string> seafloorTexture = { "Textures/Sand002.png", "Textures/Sand001Normal.png", "Textures/Caustics/caust_001.png", "Textures/Caustics/caust_002.png" , "Textures/Caustics/caust_003.png" , "Textures/Caustics/caust_004.png" , "Textures/Caustics/caust_005.png" , "Textures/Caustics/caust_006.png" , "Textures/Caustics/caust_007.png" , "Textures/Caustics/caust_008.png" , "Textures/Caustics/caust_009.png" , "Textures/Caustics/caust_010.png" , "Textures/Caustics/caust_011.png" , "Textures/Caustics/caust_012.png" , "Textures/Caustics/caust_013.png" , "Textures/Caustics/caust_014.png" , "Textures/Caustics/caust_015.png" , "Textures/Caustics/caust_016.png" };
std::vector<std::string>  waterTextures = { "Textures/0001.png", "Textures/0002.png", "Textures/DUDV0001.png", "Textures/DUDV0002.png" };
std::vector<std::string> skyboxTextures = { "Textures/Skybox/Right.png", "Textures/Skybox/Left.png", "Textures/Skybox/Top.png", "Textures/Skybox/Bottom.png", "Textures/Skybox/Back.png", "Textures/Skybox/Front.png" };
std::vector<std::string> objTextureAtlas = { "Textures/Objects/TextureAtlas.png", "Textures/Objects/TextureAtlasNormals.png", "Textures/Caustics/caust_001.png", "Textures/Caustics/caust_002.png" , "Textures/Caustics/caust_003.png" , "Textures/Caustics/caust_004.png" , "Textures/Caustics/caust_005.png" , "Textures/Caustics/caust_006.png" , "Textures/Caustics/caust_007.png" , "Textures/Caustics/caust_008.png" , "Textures/Caustics/caust_009.png" , "Textures/Caustics/caust_010.png" , "Textures/Caustics/caust_011.png" , "Textures/Caustics/caust_012.png" , "Textures/Caustics/caust_013.png" , "Textures/Caustics/caust_014.png" , "Textures/Caustics/caust_015.png" , "Textures/Caustics/caust_016.png" };

// Texture resolutions
const int waterTexRes = 1024;
const int seafloorTexRes = 2048;
const int terrainTextureRes = 2048;
const int objTextureRes = 2048;
const int noOfRows = 4; // no. of rows in the texture atlas for the objects
float causticSpeed = 0.0106f; // speed parameter to control animation of underwater caustic textures

// some parameters of the world
const vec3 terrainCenter = vec3(static_cast<float>(terrainResolution / 2.0), 0.0f, static_cast<float>(terrainResolution / 2.0));
const vec3 cameraPosition = vec3(0.0f, 70.0f, 0.0f);
const vec4 sunDirection = normalize(vec4(1.0f, 1.0f, 0.0f, 0.0f));	// Direction of sunlight in world space

//Some pointers
Camera* camera = nullptr;
Terrain* seafloor = nullptr;
TerrainShaders* seafloorShaders = nullptr;
Terrain* water = nullptr;
WaterShaders* waterShaders = nullptr;
Skybox* skybox = nullptr;
SkyboxShaders* skyboxShaders = nullptr;
std::vector<Object*> objects;
ObjectsShaders* objShaders = nullptr;
WaterFramebuffer* fbos = nullptr;

float rotationXAngle = 0.0f;	// rotation angles for camera movement, currently disabled
float rotationYAngle = 0.0f;

bool renderSeafloor = true; // to dis- or enable rendering certain aspects
bool renderWater = true;
bool renderSkybox = true;
bool renderObjects = true;

//For the 3D models
struct Objects {
	const char* file;
	vec3 pos;
	float scaleFactor;
	int index;
};

//on resize window, re-create projection matrix
void glut_resize(int32_t _width, int32_t _height) {
	width = _width;
	height = _height;
	camera->updateProjection(width / (float)height);
	fbos->setScreenViewport(width, height);
}

// If mouse is moves in direction (x,y)
void mouseMotion(int x, int y)
{
	camera->mouseMove(x, y);
	glutPostRedisplay();
}

// if mouse button button is pressed, left click: camera panning, right click: move camera forwards or backwards
void mouse(int button, int state, int x, int y)
{
	camera->mouseButton(button, state, x, y);
	glutPostRedisplay();
}

// Transformation of terrain with angles for x and y axis, place its center in (0,0)
mat4 calcTerrainTransformation(float rotationXAngle, float rotationYAngle)
{
	mat4 transformMatrix = mat4(1.0f);
	transformMatrix = rotate(transformMatrix, rotationYAngle, vec3(0.0f, 1.0f, 0.0f));
	transformMatrix = rotate(transformMatrix, rotationXAngle, vec3(1.0f, 0.0f, 0.0f));
	transformMatrix = translate(transformMatrix, -terrainCenter);
	return transformMatrix;
}

//function to render scene andits aspects seperately
void renderScene()
{
	float timeInMS = glutGet(GLUT_ELAPSED_TIME);
	float timeSinceStart = static_cast<float>(timeInMS/ 1000.0f); // time since glutInit in s												
	mat4 worldMatrix = calcTerrainTransformation(rotationXAngle, rotationYAngle);

	if (renderSeafloor)
	{
		seafloorShaders->setModelMatrix(worldMatrix * mat4(1.0));
		seafloorShaders->setViewMatrix(camera->getViewMatrix());
		seafloorShaders->setProjectionMatrix(camera->getProjectionMatrix());
		seafloorShaders->setTime(timeSinceStart, timeInMS*causticSpeed);
		seafloorShaders->setCameraPos(camera->getPosition());
		seafloorShaders->activate();
		seafloor->draw();
	}
	if (renderObjects)
	{
		for (Object* obj : objects)
		{
			float x1, z1;
			mat4 transl, rot = mat4(1.0f);
			// move fish up and down on sine wave to simulate some movement 
			if (obj->getName().find("TropicalFish") != std::string::npos || obj->getName().find("Clownfish") != std::string::npos)
			{
				float height = sin(static_cast<int>(timeInMS / 30) % 360 * PI / 180.0); // transform time to radian angle
				if (obj->getName().find("TropicalFish02") != std::string::npos) 		// fish in the center, swim in circle around stone
				{
					float angle = static_cast<int>(timeInMS / 90.0) % 360;
					float radianAngle = angle * PI / 180.0;
					x1 = obj->getPostion().x + 16.0f * sin(-radianAngle)*-1.0;			//clockwise movement
					z1 = obj->getPostion().z + 16.0f * cos(-radianAngle)*-1.0;
					transl = translate(transl, vec3(x1, 0.0f, z1));
					rot = rotate(rot, 90.0f - angle, vec3(0.0f, 1.0f, 0.0f));			// rotate to simulate fish swimming in circle
					obj->mRotationMatrix *= rot;
					height *= 3.0f;
				}
				transl = translate(transl, vec3(0.0f, height, 0.0f));
				obj->mTranslationMatrix *= transl;
			}

			objShaders->setModelMatrix(obj->getModelMatrix());
			objShaders->setViewMatrix(camera->getViewMatrix());
			objShaders->setProjectionMatrix(camera->getProjectionMatrix());
			objShaders->setIndex(obj->getIndex());
			objShaders->setTime(timeInMS*causticSpeed);
			objShaders->setCameraPos(camera->getPosition());
			objShaders->activate();
			obj->draw();
			// reset matrices after movement
			if (obj->getName().find("TropicalFish") != std::string::npos || obj->getName().find("Clownfish") != std::string::npos)
			{
				obj->mTranslationMatrix *= glm::inverse(transl);
				if (obj->getName().find("TropicalFish02") != std::string::npos)
					obj->mRotationMatrix *= glm::inverse(rot);
			}
		}
	}
	if (renderWater)
	{
		mat4 waterModelMatrix = worldMatrix;
		waterModelMatrix = translate(waterModelMatrix, vec3(0.0f, waterheight, 0.0f));
		waterShaders->setModelMatrix(waterModelMatrix * mat4(1.0));
		waterShaders->setViewMatrix(camera->getViewMatrix());
		waterShaders->setProjectionMatrix(camera->getProjectionMatrix());
		waterShaders->setTime(timeSinceStart);
		waterShaders->activate();
		water->draw();
	}
	if (renderSkybox)
	{
		glDisable(GL_CLIP_DISTANCE0);						// disable clip distance because otherwise some parts would be cut off that are needed for the reflection
		glDepthFunc(GL_LEQUAL);								// change depth function so depth test passes when values are equal to depth buffer's content
		mat4 viewMat = mat4(mat3(camera->getViewMatrix())); // remove translation from the view matrix
		skyboxShaders->setViewMatrix(viewMat);
		skyboxShaders->setProjectionMatrix(camera->getProjectionMatrix());
		skyboxShaders->setCameraPos(camera->getPosition());
		skyboxShaders->activate();
		skybox->draw();
		glDepthFunc(GL_LESS);
		glEnable(GL_CLIP_DISTANCE0);
	}
}

//preparing each render pass
void prepare()
{
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glCullFace(GL_BACK);
}

//rendering pass to get a reflection texture
void renderReflectionTexture()
{
	fbos->bindReflectionFrameBuffer();
	prepare();
	glEnable(GL_CLIP_DISTANCE0);
	camera->reflect();				// place camera down to see what will be reflected
	renderWater = false;
	renderSkybox = true;
	seafloorShaders->setClipPlane(vec4(0.0f, 1.0f, 0.0f, -(waterheight + amplitude)));	//clip everything underwater since it's not needed
	objShaders->setClipPlane(vec4(0.0f, 1.0f, 0.0f, -(waterheight + amplitude)));
	renderScene();
}

//rendering pass to get a refration texture
void renderRefractionTexture()
{
	fbos->bindRefractionFrameBuffer();
	prepare();
	camera->reflect();		// reset the camera
	seafloorShaders->setClipPlane(vec4(0.0f, -1.0f, 0.0f, (waterheight + amplitude)));	// clip everything above the water surface
	objShaders->setClipPlane(vec4(0.0f, -1.0f, 0.0f, (waterheight + amplitude)));
	renderScene();
}

//render the whole scene
void renderMainFrame()
{
	fbos->unbindCurrentFramebuffer();
	prepare();
	glDisable(GL_CLIP_DISTANCE0);
	renderWater = true;
	renderSkybox = true;
	seafloorShaders->setClipPlane(vec4(0.0f, -1.0f, 0.0f, 10000)); //sometimes glDisable(GL_CLIP_DISTANCE0) gets ignored, this "fixes" it
	renderScene();
}

void display(void)
{
	camera->update();
	
	renderReflectionTexture();
	renderRefractionTexture();
	renderMainFrame();

	glutSwapBuffers();
}

void keyboard(unsigned char key, int x, int y)
{
	switch (key) {
	case ' ':
		camera->stop();
		break;
	//escape key
	case 27:
		exit(0);
		break;
	case '.':
		glutFullScreenToggle();
		break;
	case 't':
		seafloorShaders->loadVertexFragmentShaders(seafloorVert, seafloorFrag);
		seafloorShaders->locateUniforms();
		break;
	case 'z':
		waterShaders->loadVertexFragmentShaders(waterVert, waterFrag);
		waterShaders->locateUniforms();
		break;
	}
}

int main(int argc, char** argv)
{
	// Initialize GLUT
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
	glutInitWindowSize(width, height);

	if (glutCreateWindow("rtr_ocean") == 0)
	{
		printf("Glut init failed\n");
		return -1;
	}

	glutDisplayFunc(display);
	glutIdleFunc(display);
	glutKeyboardFunc(keyboard);
	glutMotionFunc(mouseMotion);
	glutMouseFunc(mouse);

	glewExperimental = GL_TRUE;
	if (glewInit() != GLEW_OK)
	{
		printf("GLEW init failed!\n");
		return -1;
	}

	// Camera 
	camera = new Camera((width / (float)height), cameraPosition, waterheight);
	
	//WaterFramebuffer
	fbos = new WaterFramebuffer(width, height);

	// Seafloor  
	seafloor = new Terrain(terrainResolution, tileSea, true, frequency, amplitude);
	seafloorShaders = new TerrainShaders(seafloorTexture, seafloorTexRes, sunDirection, waterheight);
	seafloorShaders->loadVertexFragmentShaders(seafloorVert, seafloorFrag);
	seafloorShaders->locateUniforms();

	// Objects
	std::vector<Objects> objs{
	// Center
	{ "Objects/TropicalFish02.obj", vec3(0.0f, 28.0f, 0.0f), 1.0f, 1 },
	{ "Objects/TropicalFish02.obj", vec3(-2.0f, 25.0f, 0.0f), 1.0f, 1 },
	{ "Objects/TropicalFish02.obj", vec3(2.0f, 25.0f, 1.0f), 1.0f, 1 },
	{ "Objects/TropicalFish02.obj", vec3(4.0f, 23.0f, -1.0f), 1.0f, 1 },
	{ "Objects/TropicalFish02.obj", vec3(-3.0f, 22.0f, 3.0f), 1.0f, 1 },
	{ "Objects/TropicalFish02.obj", vec3(0.0f, 21.0f, -1.0f), 1.0f, 1 },
	{ "Objects/TropicalFish02.obj", vec3(-1.0f, 19.0f, 2.0f), 1.0f, 1 },
	{ "Objects/TropicalFish02.obj", vec3(-4.0f, 18.0f, 3.0f), 1.0f, 1 },
	{ "Objects/TropicalFish02.obj", vec3(3.0f, 17.0f, -2.0f), 1.0f, 1 },
	{ "Objects/TropicalFish02.obj", vec3(-1.0f, 17.0f, 0.0f), 1.0f, 1 },
	{ "Objects/Grass_01.obj", vec3(-1.0f, seafloor->getHeightValue(-1.0f, 2.0f), 2.0f), 1.5f, 15 },
	{ "Objects/stone4.obj",  vec3(-4.0f, seafloor->getHeightValue(6.0f, -2.0f)+3.0f, -2.0f), 1.0f, 9 },
	{ "Objects/stone5.obj",  vec3(6.0f, seafloor->getHeightValue(-4.0f, 1.0f)-2.0f, 1.0f), 1.0f, 10 },
	// lower left
	{ "Objects/TropicalFish01.obj",	vec3(-50.0f, 20.0f, 64.0f), 1.0f, 0 },
	{ "Objects/TropicalFish01.obj",	vec3(-55.0f, 21.0f, 66.0f), 1.0f, 0 },
	{ "Objects/TropicalFish01.obj",	vec3(-53.0f, 23.0f, 65.0f), 1.0f, 0 },
	{ "Objects/TropicalFish01.obj",	vec3(-93.0f, seafloor->getHeightValue(-93.0f, 23.0f)+2.0f, 23.0f), 0.7f, 0 },
	{ "Objects/CoralRock002.obj", vec3(-48.0f, seafloor->getHeightValue(-48.0f, 64.0f)+2.0f, 64.0f), 2.0f, 6 },
	{ "Objects/CoralRock003.obj", vec3(-52.0f, seafloor->getHeightValue(-52.0f, 68.0f)+2.0f, 68.0f), 2.0f, 6 },
	{ "Objects/CoralRock001.obj", vec3(-48.0f, seafloor->getHeightValue(-48.0f, 60.0f)+2.0f, 60.0f), 2.0f, 7 },
	{ "Objects/CoralRock004.obj", vec3(-51.0f, seafloor->getHeightValue(-51.0f, 64.0f)+2.0f, 64.0f), 2.0f, 7 },
	{ "Objects/Starfish.obj", vec3(-50.0f, seafloor->getHeightValue(-50.0f, 66.0f), 66.0f), 3.0f, 11 },
	{ "Objects/Grass_01.obj", vec3(-82.0f, seafloor->getHeightValue(-82.0f, 75.0f), 75.0f), 2.0f, 15 },
	{ "Objects/grass_high.obj", vec3(-58.0f, seafloor->getHeightValue(-58.0f, 70.0f), 70.0f), 1.0f, 13 },
	{ "Objects/grass_high.obj", vec3(-100.0f, seafloor->getHeightValue(-100.0f, 28.0f), 28.0f), 0.4f, 13 },
	{ "Objects/grass_high.obj", vec3(-97.0f, seafloor->getHeightValue(-97.0f, 22.0f), 22.0f), 0.4f, 13 },
	{ "Objects/grass_high.obj", vec3(-99.0f, seafloor->getHeightValue(-99.0f, 24.0f), 24.0f), 0.4f, 13 },
	{ "Objects/grass_high.obj", vec3(-96.0f, seafloor->getHeightValue(-96.0f, 24.0f), 24.0f), 0.4f, 13 },
	{ "Objects/grass_high.obj", vec3(-92.0f, seafloor->getHeightValue(-92.0f, 23.0f), 23.0f), 0.4f, 13 },
	{ "Objects/grass_high.obj", vec3(-89.0f, seafloor->getHeightValue(-89.0f, 26.0f), 26.0f), 0.4f, 13 },
	{ "Objects/grass_high.obj", vec3(-91.0f, seafloor->getHeightValue(-91.0f, 24.0f), 24.0f), 0.4f, 13 },
	{ "Objects/grass_high.obj", vec3(-88.0f, seafloor->getHeightValue(-88.0f, 24.0f), 24.0f), 0.4f, 13 },
	{ "Objects/grass_high.obj", vec3(-85.0f, seafloor->getHeightValue(-85.0f, 23.0f), 23.0f), 0.4f, 13 },
	{ "Objects/grass_high.obj", vec3(-82.0f, seafloor->getHeightValue(-82.0f, 22.0f), 22.0f), 0.4f, 13 },
	{ "Objects/grass_high.obj", vec3(-84.0f, seafloor->getHeightValue(-84.0f, 27.0f), 27.0f), 0.4f, 13 },
	{ "Objects/grass_high.obj", vec3(-81.0f, seafloor->getHeightValue(-81.0f, 24.0f), 24.0f), 0.4f, 13 },
	{ "Objects/grass_high.obj", vec3(-27.0f, seafloor->getHeightValue(-27.0f, 110.0f), 110.0f), 0.5f, 13 },
	// upper left
	{ "Objects/TropicalFish05.obj", vec3(-62.0f, 20.0f, -77.0f), 1.0f, 4 },
	{ "Objects/TropicalFish05.obj", vec3(-84.0f, 16.0f, -82.0f), 1.0f, 4 },
	{ "Objects/TropicalFish05.obj", vec3(-91.0f, 21.0f, -56.0f), 1.0f, 4 },
	{ "Objects/TropicalFish05.obj", vec3(-53.0f, 12.0f, -63.0f), 1.0f, 4 },
	{ "Objects/stone1.obj", vec3(-25.0f,  seafloor->getHeightValue(-25.0f, -90.0f) ,-90.0f), 1.0f, 8 },
	{ "Objects/stone4.obj",  vec3(-11.0f, seafloor->getHeightValue(-11.0f, -96.0f), -96.0f), 1.0f, 9 },
	{ "Objects/stone5.obj",  vec3(-20.0f, seafloor->getHeightValue(-20.0f, -85.0f), -85.0f), 1.0f, 10 },
	{ "Objects/stone1.obj", vec3(-12.0f,  seafloor->getHeightValue(-15.0f, -83.0f) ,-83.0f), 1.0f, 8 },
	{ "Objects/Grass_01.obj", vec3(-20.0f, seafloor->getHeightValue(-20.0f, -30.0f), -30.0f), 1.0f, 15 },
	{ "Objects/grass_high.obj", vec3(-2.0f, seafloor->getHeightValue(-2.0f, -98.0f), -98.0f), 0.6f, 13 },
	{ "Objects/Grass_01.obj", vec3(-20.0f, seafloor->getHeightValue(-20.0f, -82.0f), -82.0f), 1.0f, 15 },
	{ "Objects/grass_high.obj", vec3(-89.0f, seafloor->getHeightValue(-89.0f, -96.0f), -96.0f), 1.0f, 13 },
	// upper right
	{ "Objects/Clownfish002.obj", vec3(83.0f, 16.0f, -58.0f), 1.0f, 2 },
	{ "Objects/Clownfish001.obj", vec3(85.0f, 16.0f, -56.0f), 1.0f, 3 },
	{ "Objects/TropicalFish12.obj", vec3(82.0f, 16.0f, -60.0f), 1.0f, 5 },
	{ "Objects/TropicalFish12.obj", vec3(85.0f, 16.0f, -64.0f), 1.0f, 5 },
	{ "Objects/Starfish.obj", vec3(84.0f, seafloor->getHeightValue(84.0f, -57.0f)-1.0f, -57.0f), 3.0f, 11 },
	{ "Objects/CoralRock002.obj", vec3(86.0f, seafloor->getHeightValue(86.0f, -59.0f)+3.0f, -59.0f), 3.0f, 6 },
	{ "Objects/CoralRock003.obj", vec3(82.0f, seafloor->getHeightValue(82.0f, -59.0f)+3.0f, -59.0f), 3.0f, 6 },
	{ "Objects/CoralRock001.obj", vec3(77.0f, seafloor->getHeightValue(77.0f, -62.0f)+1.0f, -62.0f), 3.0f, 7 },
	{ "Objects/CoralRock004.obj", vec3(75.0f, seafloor->getHeightValue(75.0f, -62.0f)+1.0f, -62.0f), 3.0f, 7 },
	{ "Objects/grass_high.obj", vec3(65.0f, seafloor->getHeightValue(65.0f, 0.0f), 0.0f), 1.0f, 13 },
	// lower right
	{ "Objects/stone1.obj", vec3(84.0f,  seafloor->getHeightValue(84.0f, 113.0f) , 113.0f), 1.0f, 8 },
	{ "Objects/stone4.obj",  vec3(77.0f, seafloor->getHeightValue(77.0f, 112.0f), 112.0f), 1.0f, 9 },
	{ "Objects/stone5.obj",  vec3(30.0f, seafloor->getHeightValue(30.0f, 100.0f), 100.0f), 1.0f, 10 },
	{ "Objects/Starfish.obj", vec3(80.0f, seafloor->getHeightValue(80.0f, 90.0f), 90.0f), 2.0f, 11 },
	{ "Objects/oldboat.obj", vec3(80.0f, seafloor->getHeightValue(80.0f, 90.0f), 90.0f), 1.0f, 12 },
//	{ "Objects/grass_high.obj", vec3(0.0f, seafloor->getHeightValue(0.0f, 0.0f), 0.0f), 1.0f, 13 },
	{ "Objects/Grass_01.obj", vec3(71.0f, seafloor->getHeightValue(71.0f, 100.0f), 100.0f), 1.4f, 15 },
	{ "Objects/grass_high.obj", vec3(88.0f, seafloor->getHeightValue(88.0f, 102.0f), 102.0f), 0.7f, 13 },
//	{ "Objects/grass_high.obj", vec3(0.0f, seafloor->getHeightValue(0.0f, 0.0f), 0.0f), 1.0f, 13 } 
	};
	printf("Loading models...\n");
	for (Objects obj : objs)
		objects.push_back(new Object(obj.file, obj.pos, obj.scaleFactor, obj.index));
	printf("All models loaded!\n");
	objShaders = new ObjectsShaders(objTextureAtlas, objTextureRes, noOfRows, waterheight, sunDirection, tileSea, terrainResolution);
	objShaders->loadVertexFragmentShaders(objVert, objFrag);
	objShaders->locateUniforms();

	// Water  
	water = new Terrain(terrainResolution, tileNumber, false);
	waterShaders = new WaterShaders(waterTextures, waterTexRes, sunDirection, fbos, skyboxTextures, waterFrequency, waterAmplitude, tileNumber, terrainResolution);
	waterShaders->loadVertexFragmentShaders(waterVert, waterFrag);
	waterShaders->locateUniforms();

	//Skybox
	skybox = new Skybox(terrainResolution);
	skyboxShaders = new SkyboxShaders(terrainTextureRes, skyboxTextures, waterheight);
	skyboxShaders->loadVertexFragmentShaders(skyboxVert, skyboxFrag);
	skyboxShaders->locateUniforms();

	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glEnable(GL_DEPTH_TEST);

	glutMainLoop();
	return 0;
}


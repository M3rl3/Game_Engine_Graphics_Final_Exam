#include "OpenGL.h"
#include "cMeshInfo.h"
#include "LoadModel.h"
#include "ParticleAccelerator.h"
#include "DrawBoundingBox.h"

#include <glm/glm.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "cShaderManager/cShaderManager.h"
#include "cVAOManager/cVAOManager.h"
#include "cBasicTextureManager/cBasicTextureManager.h"
#include "cRenderReticle.h"

#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>
#include <stdlib.h>
#include <stdio.h>

GLFWwindow* window;
GLint mvp_location = 0;
GLuint shaderID = 0;

cVAOManager* VAOMan;
cBasicTextureManager* TextureMan;
ParticleAccelerator partAcc;
cRenderReticle crosshair;

sModelDrawInfo player_obj;
sModelDrawInfo cube_obj;

cMeshInfo* skybox_sphere_mesh;
cMeshInfo* player_mesh;
cMeshInfo* cube_mesh;
cMeshInfo* bulb_mesh;

cMeshInfo* beholder_mesh;
cMeshInfo* beholder_mesh1;
cMeshInfo* beholder_mesh2;
std::vector <cMeshInfo*> beholders;

unsigned int readIndex = 0;
int object_index = 0;
int elapsed_frames = 0;
int mouse_hover = 0;
float x, y, z, l = 1.f;
float speed = 0.f;
double seconds = 0.0;
int f_count = 0;
int counter = 0;

bool wireFrame = false;
bool doOnce = true;
bool enableMouse = false;
bool mouseClick = false;
bool spectating = false;

std::vector <std::string> meshFiles;
std::vector <cMeshInfo*> meshArray;
std::vector <cMeshInfo*> waypoints;

void ReadFromFile();
void ReadSceneDescription();
void LoadModel(std::string fileName, sModelDrawInfo& plyModel);
void LoadDungeonFloorPlan();
void ManageLights();
float RandomFloat(float a, float b);
bool RandomizePositions(cMeshInfo* mesh);

enum eEditMode
{
    MOVING_CAMERA,
    MOVING_LIGHT,
    MOVING_SELECTED_OBJECT,
    TAKE_CONTROL,
    SPECTATE
};

glm::vec3 cameraEye; //loaded from external file
//glm::vec3 cameraTarget = glm::vec3(-75.0f, 2.0f, 0.0f);

// now controlled by mouse!
glm::vec3 cameraTarget = glm::vec3(0.f, 0.f, -1.f);
eEditMode theEditMode = MOVING_CAMERA;

glm::vec3 cameraDest = glm::vec3(0.f);

float yaw = 0.f;
float pitch = 0.f;
float fov = 45.f;

// mouse state
bool firstMouse = true;
float lastX = 800.f / 2.f;
float lastY = 600.f / 2.f;

float beginTime = 0.f;
float currentTime = 0.f;
float timeDiff = 0.f;
int frameCount = 0;

static void ErrorCallback(int error, const char* description)
{
    fprintf(stderr, "Error: %s\n", description);
}

static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
    if (key == GLFW_KEY_C && action == GLFW_PRESS)
    {
        theEditMode = MOVING_CAMERA;
    }
    if (key == GLFW_KEY_O && action == GLFW_PRESS)
    {
        theEditMode = MOVING_SELECTED_OBJECT;
    }
    if (key == GLFW_KEY_F && action == GLFW_PRESS)
    {
        theEditMode = TAKE_CONTROL;
        //cameraTarget = player_mesh->position;
        cameraEye = player_mesh->position - glm::vec3(20.f, -4.f, 0.f);
    }
    // Wireframe
    if (key == GLFW_KEY_X && action == GLFW_PRESS) {
        for (int i = 0; i < meshArray.size(); i++) {
            meshArray[i]->isWireframe = true;
        }
    }
    if (key == GLFW_KEY_X && action == GLFW_RELEASE) {
        for (int i = 0; i < meshArray.size(); i++) {
            meshArray[i]->isWireframe = false;
        }
    }
    if (key == GLFW_KEY_LEFT_ALT) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
    if (key == GLFW_KEY_LEFT_ALT && action == GLFW_RELEASE) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    }
    /* 
    *    updates translation of all objects in the scene based on changes made to scene 
    *    description files, resets everything if no changes were made
    */
    if (key == GLFW_KEY_U && action == GLFW_PRESS) {
        ReadSceneDescription();
        player_mesh->particle->position = player_mesh->position;
    }
    if (key == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        mouseClick = true;
        printf("Clicked\n");
    }
    if (key == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) {
        mouseClick = false;
    }
    if (key == GLFW_KEY_F1 && action == GLFW_PRESS) {
        enableMouse = !enableMouse;
    }
    if (key == GLFW_KEY_F6 && action == GLFW_PRESS) {
        theEditMode = SPECTATE;
        counter++;
    }

    switch (theEditMode)
    {
        case MOVING_CAMERA:
        {
            const float CAMERA_MOVE_SPEED = 1.f;
            if (key == GLFW_KEY_A)     // Left
            {
                ::cameraEye.x -= CAMERA_MOVE_SPEED;
            }
            if (key == GLFW_KEY_D)     // Right
            {
                ::cameraEye.x += CAMERA_MOVE_SPEED;
            }
            if (key == GLFW_KEY_W)     // Forward
            {
                ::cameraEye.z += CAMERA_MOVE_SPEED;
            }
            if (key == GLFW_KEY_S)     // Backwards
            {
                ::cameraEye.z -= CAMERA_MOVE_SPEED;
            }
            if (key == GLFW_KEY_Q)     // Down
            {
                ::cameraEye.y -= CAMERA_MOVE_SPEED;
            }
            if (key == GLFW_KEY_E)     // Up
            {
                ::cameraEye.y += CAMERA_MOVE_SPEED;
            }

            if (key == GLFW_KEY_1)
            {
                ::cameraEye = glm::vec3(0.f, 0.f, -5.f);
            }
        }
        break;
        case MOVING_SELECTED_OBJECT:
        {
            const float OBJECT_MOVE_SPEED = 1.f;
            if (key == GLFW_KEY_A)     // Left
            {
                meshArray[object_index]->position.x -= OBJECT_MOVE_SPEED;
            }
            if (key == GLFW_KEY_D)     // Right
            {
                meshArray[object_index]->position.x += OBJECT_MOVE_SPEED;
            }
            if (key == GLFW_KEY_W)     // Forward
            {
                meshArray[object_index]->position.z += OBJECT_MOVE_SPEED;
            }
            if (key == GLFW_KEY_S)     // Backwards
            {
                meshArray[object_index]->position.z -= OBJECT_MOVE_SPEED;
            }
            if (key == GLFW_KEY_Q)     // Down
            {
                meshArray[object_index]->position.y -= OBJECT_MOVE_SPEED;
            }
            if (key == GLFW_KEY_E)     // Up
            {
                meshArray[object_index]->position.y += OBJECT_MOVE_SPEED;
            }

            // Cycle through objects in the scene
            if (key == GLFW_KEY_1 && action == GLFW_PRESS)
            {
                cameraTarget = glm::vec3(0.f, 0.f, 0.f);
            }
            if (key == GLFW_KEY_2 && action == GLFW_PRESS)
            {
                object_index++;
                if (object_index > meshArray.size()-1) {
                    object_index = 0;
                }
                cameraTarget = meshArray[object_index]->position;
            }
            if (key == GLFW_KEY_3 && action == GLFW_PRESS)
            {
                object_index--;
                if (object_index < 0) {
                    object_index = meshArray.size() - 1;
                }
                cameraTarget = meshArray[object_index]->position;
            }    
        }
        break;
        case TAKE_CONTROL: {
            if (key == GLFW_KEY_W) {
                player_mesh->particle->position.x += 1.f;
                //partAcc.UpdateStep(glm::vec3(1, 0, 0), 0.1f);
            }
            if (key == GLFW_KEY_S) {
                player_mesh->particle->position.x -= 1.f;
                //partAcc.UpdateStep(glm::vec3(-1, 0, 0), 0.1f);
            }
            if (key == GLFW_KEY_A) {
                player_mesh->particle->position.z -= 1.f;
                //partAcc.UpdateStep(glm::vec3(0, 0, -1), 0.1f);
            }
            if (key == GLFW_KEY_D) {
                player_mesh->particle->position.z += 1.f;
                //partAcc.UpdateStep(glm::vec3(0, 0, 1), 0.1f);
            }
            if (key == GLFW_KEY_Q) {
                player_mesh->particle->position.y += 1.f;
                //partAcc.UpdateStep(glm::vec3(0, 1, 0), 0.1f);
            }
            if (key == GLFW_KEY_E) {
                player_mesh->particle->position.y -= 1.f;
                //partAcc.UpdateStep(glm::vec3(0, -1, 0), 0.1f);
            }
            // Roatation
            if (key == GLFW_KEY_UP) {
                player_mesh->rotation.x += 1.f;
            }
            if (key == GLFW_KEY_DOWN) {
                player_mesh->rotation.x -= 1.f;
            }
            if (key == GLFW_KEY_LEFT) {
                player_mesh->rotation.z += 1.f;
            }
            if (key == GLFW_KEY_RIGHT) {
                player_mesh->rotation.z -= 1.f;
            }
            if (key == GLFW_KEY_PAGE_UP) {
                player_mesh->rotation.y += 1.f;
            }
            if (key == GLFW_KEY_PAGE_DOWN) {
                player_mesh->rotation.y -= 1.f;
            }
            // player Speed
            if (key == GLFW_KEY_1 && action == GLFW_PRESS) {
                speed += 0.01f;
            }
            if (key == GLFW_KEY_2 && action == GLFW_PRESS) {
                speed -= 0.01f;
            }
        }
        break;
    }
}

static void MouseCallBack(GLFWwindow* window, double xposition, double yposition) {

    if (firstMouse) {
        lastX = xposition;
        lastY = yposition;
        firstMouse = false;
    }

    float xoffset = xposition - lastX;
    float yoffset = lastY - yposition;  // reversed since y coordinates go from bottom to up
    lastX = xposition;
    lastY = yposition;

    float sensitivity = 0.1f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    yaw += xoffset;
    pitch += yoffset;

    // prevent perspective from getting flipped by capping it
    if (pitch > 89.f) {
        pitch = 89.f;
    }
    if (pitch < -89.f) {
        pitch = -89.f;
    }

    glm::vec3 front;
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    if (enableMouse) {
        cameraTarget = glm::normalize(front);
    }
}

static void ScrollCallBack(GLFWwindow* window, double xoffset, double yoffset) {
    if (fov >= 1.f && fov <= 45.f) {
        fov -= yoffset;
    }
    if (fov <= 1.f) {
        fov = 1.f;
    }
    if (fov >= 45.f) {
        fov = 45.f;
    }
}

void Initialize() {

    if (!glfwInit()) {
        std::cerr << "GLFW init failed." << std::endl;
        glfwTerminate();
        return;
    }

    const char* glsl_version = "#version 420";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    // glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWmonitor* currentMonitor = glfwGetPrimaryMonitor();

    const GLFWvidmode* mode = glfwGetVideoMode(currentMonitor);

    glfwWindowHint(GLFW_RED_BITS, mode->redBits);
    glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
    glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
    glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);

    window = glfwCreateWindow(1366, 768, "Final-Exam", NULL, NULL);

    // Uncomment for fullscreen support based on current monitor
    // window = glfwCreateWindow(mode->height, mode->width, "Physics 3", currentMonitor, NULL);
    
    if (!window) {
        std::cerr << "Window creation failed." << std::endl;
        glfwTerminate();
        return;
    }

    glfwSetWindowAspectRatio(window, 16, 9);

    // keyboard callback
    glfwSetKeyCallback(window, KeyCallback);

    // mouse and scroll callback
    glfwSetCursorPosCallback(window, MouseCallBack);
    glfwSetScrollCallback(window, ScrollCallBack);

    // capture mouse input
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    glfwSetErrorCallback(ErrorCallback);

    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)(glfwGetProcAddress))) {
        std::cerr << "Error: unable to obtain pocess address." << std::endl;
        return;
    }
    glfwSwapInterval(1); //vsync

    // Init imgui for crosshair
    crosshair.Initialize(window, glsl_version);

    x = 0.1f; y = 0.5f; z = 19.f;
}

void Render() {
    
    GLint vpos_location = 0;
    GLint vcol_location = 0;
    GLuint vertex_buffer = 0;

    glCullFace(GL_BACK);
    glEnable(GL_DEPTH_TEST);

    //Shader Manager
    cShaderManager* shadyMan = new cShaderManager();

    cShaderManager::cShader vertexShader;
    cShaderManager::cShader fragmentShader;

    vertexShader.fileName = "./shaders/vertexShader.glsl";
    fragmentShader.fileName = "./shaders/fragmentShader.glsl";

    if (!shadyMan->createProgramFromFile("ShadyProgram", vertexShader, fragmentShader)) {
        std::cout << "Error: Shader program failed to compile." << std::endl;
        std::cout << shadyMan->getLastError();
        return;
    }
    else {
        std::cout << "Shaders compiled." << std::endl;
    }

    shadyMan->useShaderProgram("ShadyProgram");
    shaderID = shadyMan->getIDFromFriendlyName("ShadyProgram");
    glUseProgram(shaderID);

    // Load asset paths from external file
    ReadFromFile();

    // VAO Manager
    VAOMan = new cVAOManager();
    
    // Scene
    sModelDrawInfo long_highway;
    LoadModel(meshFiles[2], long_highway);
    if (!VAOMan->LoadModelIntoVAO("long_highway", long_highway, shaderID)) {
        std::cerr << "Could not load model into VAO" << std::endl;
    }
    cMeshInfo* long_highway_mesh = new cMeshInfo();
    long_highway_mesh->meshName = "long_highway";
    long_highway_mesh->friendlyName = "long_highway";
    long_highway_mesh->isWireframe = wireFrame;
    long_highway_mesh->RGBAColour = glm::vec4(15.f, 18.f, 13.f, 1.f);
    long_highway_mesh->useRGBAColour = true;
    meshArray.push_back(long_highway_mesh);
    
    sModelDrawInfo bulb;
    LoadModel(meshFiles[0], bulb);
    if (!VAOMan->LoadModelIntoVAO("bulb", bulb, shaderID)) {
        std::cerr << "Could not load model into VAO" << std::endl;
    }
    bulb_mesh = new cMeshInfo();
    bulb_mesh->meshName = "bulb";
    bulb_mesh->friendlyName = "bulb";
    bulb_mesh->isWireframe = wireFrame;
    meshArray.push_back(bulb_mesh);
    
    cMeshInfo* bulb_mesh1 = new cMeshInfo();
    bulb_mesh1->meshName = "bulb";
    bulb_mesh1->friendlyName = "bulb1";
    bulb_mesh1->isWireframe = wireFrame;
    meshArray.push_back(bulb_mesh1);
    
    cMeshInfo* bulb_mesh2 = new cMeshInfo();
    bulb_mesh2->meshName = "bulb";
    bulb_mesh2->friendlyName = "bulb2";
    bulb_mesh2->isWireframe = wireFrame;
    meshArray.push_back(bulb_mesh2); 
    
    cMeshInfo* bulb_mesh3 = new cMeshInfo();
    bulb_mesh3->meshName = "bulb";
    bulb_mesh3->friendlyName = "bulb3";
    bulb_mesh3->isWireframe = wireFrame;
    meshArray.push_back(bulb_mesh3);
    
    cMeshInfo* bulb_mesh4 = new cMeshInfo();
    bulb_mesh4->meshName = "bulb";
    bulb_mesh4->friendlyName = "bulb4";
    bulb_mesh4->isWireframe = wireFrame;
    meshArray.push_back(bulb_mesh4);
    
    cMeshInfo* bulb_mesh5 = new cMeshInfo();
    bulb_mesh5->meshName = "bulb";
    bulb_mesh5->friendlyName = "bulb5";
    bulb_mesh5->isWireframe = wireFrame;
    meshArray.push_back(bulb_mesh5);
    
    cMeshInfo* bulb_mesh6 = new cMeshInfo();
    bulb_mesh6->meshName = "bulb";
    bulb_mesh6->friendlyName = "bulb6";
    bulb_mesh6->isWireframe = wireFrame;
    meshArray.push_back(bulb_mesh6);
    
    cMeshInfo* bulb_mesh7 = new cMeshInfo();
    bulb_mesh7->meshName = "bulb";
    bulb_mesh7->friendlyName = "bulb7";
    bulb_mesh7->isWireframe = wireFrame;
    meshArray.push_back(bulb_mesh7);
    
    cMeshInfo* bulb_mesh8 = new cMeshInfo();
    bulb_mesh8->meshName = "bulb";
    bulb_mesh8->friendlyName = "bulb8";
    bulb_mesh8->isWireframe = wireFrame;
    meshArray.push_back(bulb_mesh8);
    
    if (!VAOMan->LoadModelIntoVAO("bulb9", bulb, shaderID)) {
        std::cerr << "Could not load model into VAO" << std::endl;
    }
    cMeshInfo* bulb_mesh9 = new cMeshInfo();
    bulb_mesh9->meshName = "bulb9";
    bulb_mesh9->isWireframe = wireFrame;
    meshArray.push_back(bulb_mesh9);
    
    cMeshInfo* bulb_mesh10 = new cMeshInfo();
    bulb_mesh10->meshName = "bulb";
    bulb_mesh10->friendlyName = "bulb10";
    bulb_mesh10->isWireframe = wireFrame;
    meshArray.push_back(bulb_mesh10);
    
    cMeshInfo* bulb_mesh11 = new cMeshInfo();
    bulb_mesh11->meshName = "bulb";
    bulb_mesh11->friendlyName = "bulb11";
    bulb_mesh11->isWireframe = wireFrame;
    meshArray.push_back(bulb_mesh11);
    
    cMeshInfo* bulb_mesh12 = new cMeshInfo();
    bulb_mesh12->meshName = "bulb";
    bulb_mesh12->friendlyName = "bulb12";
    bulb_mesh12->isWireframe = wireFrame;
    meshArray.push_back(bulb_mesh12);

    sModelDrawInfo long_sidewalk;
    LoadModel(meshFiles[3], long_sidewalk);
    if (!VAOMan->LoadModelIntoVAO("long_sidewalk", long_sidewalk, shaderID)) {
        std::cerr << "Could not load model into VAO" << std::endl;
    }
    cMeshInfo* long_sidewalk_mesh = new cMeshInfo();
    long_sidewalk_mesh->meshName = "long_sidewalk";
    long_sidewalk_mesh->friendlyName = "long_sidewalk";
    long_sidewalk_mesh->isWireframe = wireFrame;
    long_sidewalk_mesh->RGBAColour = glm::vec4(1.f, 5.f, 1.f, 1.f);
    long_sidewalk_mesh->useRGBAColour = true;
    meshArray.push_back(long_sidewalk_mesh);
    
    cMeshInfo* long_sidewalk_mesh1 = new cMeshInfo();
    long_sidewalk_mesh1->meshName = "long_sidewalk";
    long_sidewalk_mesh1->friendlyName = "long_sidewalk1";
    long_sidewalk_mesh1->isWireframe = wireFrame;
    long_sidewalk_mesh1->RGBAColour = glm::vec4(1.f, 5.f, 1.f, 1.f);
    long_sidewalk_mesh1->useRGBAColour = true;
    meshArray.push_back(long_sidewalk_mesh1);
    //long_sidewalk_mesh1->CopyVertices(long_sidewalk);
    
    sModelDrawInfo beholder_obj;
    LoadModel(meshFiles[9], beholder_obj);
    if (!VAOMan->LoadModelIntoVAO("beholderr", beholder_obj, shaderID)) {
        std::cerr << "Could not load model into VAO" << std::endl;
    }
    cMeshInfo* flat_beholder_mesh = new cMeshInfo();
    flat_beholder_mesh->meshName = "beholderr";
    flat_beholder_mesh->friendlyName = "flat_beholder_mesh";
    flat_beholder_mesh->isWireframe = wireFrame;
    flat_beholder_mesh->RGBAColour = glm::vec4(34.5f, 34.5f, 34.5f, 1.f);
    flat_beholder_mesh->useRGBAColour = true;
    meshArray.push_back(flat_beholder_mesh);
    
    sModelDrawInfo terrain_obj;
    LoadModel(meshFiles[10], terrain_obj);
    if (!VAOMan->LoadModelIntoVAO("terrain", terrain_obj, shaderID)) {
        std::cerr << "Could not load model into VAO" << std::endl;
    }
    cMeshInfo* terrain_mesh = new cMeshInfo();
    terrain_mesh->meshName = "terrain";
    terrain_mesh->isWireframe = wireFrame;
    terrain_mesh->RGBAColour = glm::vec4(25.f, 25.f, 25.f, 1.f);
    terrain_mesh->doNotLight = false;
    terrain_mesh->useRGBAColour = true;
    terrain_mesh->isTerrainMesh = false;
    meshArray.push_back(terrain_mesh);

    LoadModel(meshFiles[9], player_obj);
    if (!VAOMan->LoadModelIntoVAO("player", player_obj, shaderID)) {
        std::cerr << "Could not load model into VAO" << std::endl;
    }
    player_mesh = new cMeshInfo();
    player_mesh->meshName = "player";
    player_mesh->friendlyName = "player";
    player_mesh->isWireframe = wireFrame;
    player_mesh->RGBAColour = glm::vec4(25.f, 25.f, 25.f, 1.f);
    player_mesh->useRGBAColour = false;
    player_mesh->drawBBox = true;
    player_mesh->CopyVertices(player_obj);
    meshArray.push_back(player_mesh);

    sModelDrawInfo pyramid_obj;
    LoadModel(meshFiles[6], pyramid_obj);
    if (!VAOMan->LoadModelIntoVAO("pyramid", pyramid_obj, shaderID)) {
        std::cerr << "Could not load model into VAO" << std::endl;
    }
    cMeshInfo* pyramid_mesh = new cMeshInfo();
    pyramid_mesh->meshName = "pyramid";
    pyramid_mesh->friendlyName = "pyramid";
    pyramid_mesh->isWireframe = wireFrame;
    pyramid_mesh->RGBAColour = glm::vec4(25.f, 25.f, 25.f, 1.f);
    pyramid_mesh->doNotLight = false;
    pyramid_mesh->useRGBAColour = true;
    meshArray.push_back(pyramid_mesh);

    sModelDrawInfo moon_obj;
    LoadModel(meshFiles[11], moon_obj);
    if (!VAOMan->LoadModelIntoVAO("moon", moon_obj, shaderID)) {
        std::cerr << "Could not load model into VAO" << std::endl;
    }
    cMeshInfo* moon_mesh = new cMeshInfo();
    moon_mesh->meshName = "moon";
    moon_mesh->friendlyName = "moon";
    moon_mesh->isWireframe = wireFrame;
    moon_mesh->useRGBAColour = false;
    moon_mesh->RGBAColour = glm::vec4(100.f, 25.f, 25.f, 1.f);
    moon_mesh->hasTexture = true;
    moon_mesh->textures[0] = "moon_texture.bmp";
    moon_mesh->textureRatios[0] = 1.0f;
    meshArray.push_back(moon_mesh);

    // skybox sphere with inverted normals
    sModelDrawInfo skybox_sphere_obj;
    LoadModel(meshFiles[12], skybox_sphere_obj);
    if (!VAOMan->LoadModelIntoVAO("skybox_sphere", skybox_sphere_obj, shaderID)) {
        std::cerr << "Could not load model into VAO" << std::endl;
    }
    skybox_sphere_mesh = new cMeshInfo();
    skybox_sphere_mesh->meshName = "skybox_sphere";
    skybox_sphere_mesh->friendlyName = "skybox_sphere";
    skybox_sphere_mesh->isSkyBoxMesh = true;
    meshArray.push_back(skybox_sphere_mesh);

    // Dungeon Models Loaded here
    LoadDungeonFloorPlan();

    //Beholder0
    if (!VAOMan->LoadModelIntoVAO("beholder", beholder_obj, shaderID)) {
        std::cerr << "Could not load model into VAO" << std::endl;
    }
    beholder_mesh = new cMeshInfo();
    beholder_mesh->meshName = "beholder";
    beholder_mesh->friendlyName = "beholder0";
    beholder_mesh->isWireframe = wireFrame;
    beholder_mesh->RGBAColour = glm::vec4(34.5f, 34.5f, 34.5f, 1.f);
    beholder_mesh->useRGBAColour = false;
    beholder_mesh->hasChildMeshes = true;
    meshArray.push_back(beholder_mesh);

    sModelDrawInfo vision_cone;
    LoadModel(meshFiles[19], vision_cone);
    if (!VAOMan->LoadModelIntoVAO("vision_cone", vision_cone, shaderID)) {
        std::cerr << "Could not load model into VAO" << std::endl;
    }
    cMeshInfo* beholder0_cone = new cMeshInfo();
    beholder0_cone->meshName = "vision_cone";
    beholder0_cone->friendlyName = "beholder0_cone";
    beholder_mesh->vecChildMeshes.push_back(beholder0_cone);

    //Beholder1
    beholder_mesh1 = new cMeshInfo();
    beholder_mesh1->meshName = "beholder";
    beholder_mesh1->friendlyName = "beholder1";
    beholder_mesh1->isWireframe = wireFrame;
    beholder_mesh1->RGBAColour = glm::vec4(34.5f, 34.5f, 34.5f, 1.f);
    beholder_mesh1->useRGBAColour = false;
    beholder_mesh1->hasChildMeshes = true;
    meshArray.push_back(beholder_mesh1);

    cMeshInfo* beholder1_cone = new cMeshInfo();
    beholder1_cone->meshName = "vision_cone";
    beholder1_cone->friendlyName = "beholder1_cone";
    beholder_mesh1->vecChildMeshes.push_back(beholder1_cone);

    //Beholder2
    beholder_mesh2 = new cMeshInfo();
    beholder_mesh2->meshName = "beholder";
    beholder_mesh2->friendlyName = "beholder2";
    beholder_mesh2->isWireframe = wireFrame;
    beholder_mesh2->RGBAColour = glm::vec4(34.5f, 34.5f, 34.5f, 1.f);
    beholder_mesh2->useRGBAColour = false;
    beholder_mesh2->hasChildMeshes = true;
    meshArray.push_back(beholder_mesh2);

    cMeshInfo* beholder2_cone = new cMeshInfo();
    beholder2_cone->meshName = "vision_cone";
    beholder2_cone->friendlyName = "beholder2_cone";
    beholder_mesh2->vecChildMeshes.push_back(beholder2_cone);

    //Waypoints
    sModelDrawInfo waypoint_sphere;
    LoadModel(meshFiles[18], waypoint_sphere);
    if (!VAOMan->LoadModelIntoVAO("waypoint_sphere", waypoint_sphere, shaderID)) {
        std::cerr << "Could not load model into VAO" << std::endl;
    }
    cMeshInfo* waypoint_sphere0 = new cMeshInfo();
    waypoint_sphere0->meshName = "waypoint_sphere";
    waypoint_sphere0->friendlyName = "waypoint0";
    meshArray.push_back(waypoint_sphere0);

    cMeshInfo* waypoint_sphere1 = new cMeshInfo();
    waypoint_sphere1->meshName = "waypoint_sphere";
    waypoint_sphere1->friendlyName = "waypoint1";
    meshArray.push_back(waypoint_sphere1);
    
    cMeshInfo* waypoint_sphere2 = new cMeshInfo();
    waypoint_sphere2->meshName = "waypoint_sphere";
    waypoint_sphere2->friendlyName = "waypoint2";
    meshArray.push_back(waypoint_sphere2);

    cMeshInfo* waypoint_sphere3 = new cMeshInfo();
    waypoint_sphere3->meshName = "waypoint_sphere";
    waypoint_sphere3->friendlyName = "waypoint3";
    meshArray.push_back(waypoint_sphere3);

    cMeshInfo* waypoint_sphere4 = new cMeshInfo();
    waypoint_sphere4->meshName = "waypoint_sphere";
    waypoint_sphere4->friendlyName = "waypoint4";
    meshArray.push_back(waypoint_sphere4);

    cMeshInfo* waypoint_sphere5 = new cMeshInfo();
    waypoint_sphere5->meshName = "waypoint_sphere";
    waypoint_sphere5->friendlyName = "waypoint5";
    meshArray.push_back(waypoint_sphere5);

    cMeshInfo* waypoint_sphere6 = new cMeshInfo();
    waypoint_sphere6->meshName = "waypoint_sphere";
    waypoint_sphere6->friendlyName = "waypoint6";
    meshArray.push_back(waypoint_sphere6);

    cMeshInfo* waypoint_sphere7 = new cMeshInfo();
    waypoint_sphere7->meshName = "waypoint_sphere";
    waypoint_sphere7->friendlyName = "waypoint7";
    meshArray.push_back(waypoint_sphere7);

    cMeshInfo* waypoint_sphere8 = new cMeshInfo();
    waypoint_sphere8->meshName = "waypoint_sphere";
    waypoint_sphere8->friendlyName = "waypoint8";
    meshArray.push_back(waypoint_sphere8);

    cMeshInfo* waypoint_sphere9 = new cMeshInfo();
    waypoint_sphere9->meshName = "waypoint_sphere";
    waypoint_sphere9->friendlyName = "waypoint9";
    meshArray.push_back(waypoint_sphere9);

    cMeshInfo* waypoint_sphere10 = new cMeshInfo();
    waypoint_sphere10->meshName = "waypoint_sphere";
    waypoint_sphere10->friendlyName = "waypoint10";
    meshArray.push_back(waypoint_sphere10);

    cMeshInfo* waypoint_sphere11 = new cMeshInfo();
    waypoint_sphere11->meshName = "waypoint_sphere";
    waypoint_sphere11->friendlyName = "waypoint11";
    meshArray.push_back(waypoint_sphere11);

    
    // Setting textures here
    for (int i = 0; i < meshArray.size(); i++) {
        cMeshInfo* currentMesh = meshArray[i];

        if (currentMesh->meshName == "wall" || 
            currentMesh->meshName == "floor" || 
            currentMesh->meshName == "arched_doorway")
        {
            currentMesh->useRGBAColour = true;
            currentMesh->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
            currentMesh->hasTexture = false;
            currentMesh->textures[1] = "Dungeons_2_Texture_01_A.bmp";
            currentMesh->textureRatios[1] = 1.f;
        }
    }

    // skybox/cubemap textures
    std::cout << "\nLoading Textures";

    std::string errorString = "";
    TextureMan = new cBasicTextureManager();

    TextureMan->SetBasePath("../assets/textures");
    const char* skybox_name = "NightSky";
    if (TextureMan->CreateCubeTextureFromBMPFiles("NightSky",
                                                  "SpaceBox_right1_posX.bmp",
                                                  "SpaceBox_left2_negX.bmp",
                                                  "SpaceBox_top3_posY.bmp",
                                                  "SpaceBox_bottom4_negY.bmp",
                                                  "SpaceBox_front5_posZ.bmp",
                                                  "SpaceBox_back6_negZ.bmp",
                                                  true, errorString)) 
    {
        std::cout << "\nLoaded skybox textures: " << skybox_name << std::endl;
    }
    else 
    {
        std::cout << "\nError: failed to load skybox because " << errorString;
    }

    // Basic texture2D
    if (TextureMan->Create2DTextureFromBMPFile("moon_texture.bmp"))
    {
        std::cout << "Loaded moon texture." << std::endl;
    }
    else 
    {
        std::cout << "Error: failed to load moon texture.";
    }
    
    if (TextureMan->Create2DTextureFromBMPFile("Dungeons_2_Texture_01_A.bmp"))
    {
        std::cout << "Loaded dungeon texture." << std::endl;
    }
    else 
    {
        std::cout << "Error: failed to load dungeon texture.";
    }

    // reads scene descripion files for positioning and other info
    ReadSceneDescription();

    waypoints.push_back(waypoint_sphere0);
    waypoints.push_back(waypoint_sphere1);
    waypoints.push_back(waypoint_sphere2);
    waypoints.push_back(waypoint_sphere3);
    waypoints.push_back(waypoint_sphere4);
    waypoints.push_back(waypoint_sphere5);
    waypoints.push_back(waypoint_sphere6);
    waypoints.push_back(waypoint_sphere7);
    waypoints.push_back(waypoint_sphere8);
    waypoints.push_back(waypoint_sphere9);
    waypoints.push_back(waypoint_sphere10);
    waypoints.push_back(waypoint_sphere11);

    beholders.push_back(beholder_mesh);
    beholders.push_back(beholder_mesh1);
    beholders.push_back(beholder_mesh2);

    for (int i = 0; i < waypoints.size(); i++) {
        cMeshInfo* currentWaypoint = waypoints[i];
        currentWaypoint->isVisible = false;
    }

    // initialize the particle to player position
    player_mesh->particle = partAcc.InitParticle(player_mesh->position);
}

void Update() {

    //MVP
    glm::mat4x4 model, view, projection;
    glm::vec3 upVector = glm::vec3(0.f, 1.f, 0.f);

    GLint modelLocaction = glGetUniformLocation(shaderID, "Model");
    GLint viewLocation = glGetUniformLocation(shaderID, "View");
    GLint projectionLocation = glGetUniformLocation(shaderID, "Projection");
    GLint modelInverseLocation = glGetUniformLocation(shaderID, "ModelInverse");
    
    //Lighting
    ManageLights();

    float ratio;
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    ratio = width / (float)height;
    glViewport(0, 0, width, height);

    glEnable(GL_DEPTH_TEST);
    glCullFace(GL_BACK);
    
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // mouse support
    if (enableMouse) {
        view = glm::lookAt(cameraEye, cameraEye + cameraTarget, upVector);
        projection = glm::perspective(glm::radians(fov), ratio, 0.1f, 10000.f);
    }
    else {
        view = glm::lookAt(cameraEye, cameraTarget, upVector);
        projection = glm::perspective(0.6f, ratio, 0.1f, 10000.f);
    }

    glm::vec4 viewport = glm::vec4(0, 0, width, height);

    GLint eyeLocationLocation = glGetUniformLocation(shaderID, "eyeLocation");
    glUniform4f(eyeLocationLocation, cameraEye.x, cameraEye.y, cameraEye.z, 1.f);

    currentTime = glfwGetTime();
    timeDiff = currentTime - beginTime;
    frameCount++;

    if (theEditMode == TAKE_CONTROL) {
        cameraEye = player_mesh->position - glm::vec3(15.f, -4.f, 0.f);
    }

    // Update particle position per frame
    partAcc.UpdateStep(glm::vec3(1, 0, 0), speed);
    player_mesh->position = player_mesh->particle->position;
    bulb_mesh->position = player_mesh->position - glm::vec3(75.f, -25.f, 0.f);

    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
        mouseClick = true;
    }
    else mouseClick = false;

    for (int i = 0; i < meshArray.size(); i++) {

        cMeshInfo* currentMesh = meshArray[i];
        model = glm::mat4x4(1.f);

        if (currentMesh->isVisible == false) {
            continue;
        }

        glm::mat4 translationMatrix = glm::translate(glm::mat4(1.f), currentMesh->position);
        glm::mat4 scaling = glm::scale(glm::mat4(1.f), currentMesh->scale);

        /*glm::mat4 scaling = glm::scale(glm::mat4(1.f), glm::vec3(currentMesh->scale.x,
                                                                 currentMesh->scale.y,
                                                                 currentMesh->scale.z));*/
        if (currentMesh->isSkyBoxMesh) {
            model = glm::mat4x4(1.f);
        }
        // just flatten a beholder for no reason
        if (currentMesh->meshName == "beholderr") {
            glm::mat4 rotation = glm::mat4(currentMesh->rotation);

            model *= translationMatrix;
            model *= rotation;
            model *= scaling;
        }
        else {

            glm::mat4 rotationX = glm::rotate(glm::mat4(1.f), currentMesh->rotation.x, glm::vec3(1.f, 0.f, 0.f));
            glm::mat4 rotationY = glm::rotate(glm::mat4(1.f), currentMesh->rotation.y, glm::vec3(0.f, 1.f, 0.f));
            glm::mat4 rotationZ = glm::rotate(glm::mat4(1.f), currentMesh->rotation.z, glm::vec3(0.f, 0.f, 1.f));

            model *= translationMatrix;
            model *= rotationX;
            model *= rotationY;
            model *= rotationZ;
            model *= scaling;
        }

        glUniformMatrix4fv(modelLocaction, 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(viewLocation, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projectionLocation, 1, GL_FALSE, glm::value_ptr(projection));

        glm::mat4 modelInverse = glm::inverse(glm::transpose(model));
        glUniformMatrix4fv(modelInverseLocation, 1, GL_FALSE, glm::value_ptr(modelInverse));

        if (currentMesh->isWireframe) 
        {
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        }
        else
        {
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }

        GLint useIsTerrainMeshLocation = glGetUniformLocation(shaderID, "bIsTerrainMesh");

        if (currentMesh->isTerrainMesh) 
        {
            glUniform1f(useIsTerrainMeshLocation, (GLfloat)GL_TRUE);
        }
        else 
        {
            glUniform1f(useIsTerrainMeshLocation, (GLfloat)GL_FALSE);
        }

        GLint RGBAColourLocation = glGetUniformLocation(shaderID, "RGBAColour");

        glUniform4f(RGBAColourLocation, currentMesh->RGBAColour.r, currentMesh->RGBAColour.g, currentMesh->RGBAColour.b, currentMesh->RGBAColour.w);

        GLint useRGBAColourLocation = glGetUniformLocation(shaderID, "useRGBAColour");

        if (currentMesh->useRGBAColour)
        {
            glUniform1f(useRGBAColourLocation, (GLfloat)GL_TRUE);
        }
        else
        {
            glUniform1f(useRGBAColourLocation, (GLfloat)GL_FALSE);
        }
        
        GLint bHasTextureLocation = glGetUniformLocation(shaderID, "bHasTexture");

        if (currentMesh->hasTexture) 
        {
            glUniform1f(bHasTextureLocation, (GLfloat)GL_TRUE);

            std::string texture0 = currentMesh->textures[0];    // moon
            std::string texture1 = currentMesh->textures[1];    // dungeon

            if (texture0 == "moon_texture.bmp") {
                
                GLuint texture0ID = TextureMan->getTextureIDFromName(texture0);

                GLuint texture0Unit = 0;
                glActiveTexture(texture0Unit + GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, texture0ID);

                GLint texture0Location = glGetUniformLocation(shaderID, "texture0");
                glUniform1i(texture0Location, texture0Unit);

                GLint texRatio_0_3 = glGetUniformLocation(shaderID, "texRatio_0_3");
                glUniform4f(texRatio_0_3,
                            currentMesh->textureRatios[0],
                            currentMesh->textureRatios[1],
                            currentMesh->textureRatios[2],
                            currentMesh->textureRatios[3]);
            }
            else if (texture1 == "Dungeons_2_Texture_01_A.bmp") {

                GLuint texture1ID = TextureMan->getTextureIDFromName(texture1);

                GLuint texture1unit = 1;
                glActiveTexture(texture1unit + GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, texture1ID);

                GLint texture1Location = glGetUniformLocation(shaderID, "texture1");
                glUniform1i(texture1Location, texture1unit);

                GLint texRatio_0_3 = glGetUniformLocation(shaderID, "texRatio_0_3");
                glUniform4f(texRatio_0_3,
                            currentMesh->textureRatios[0],
                            currentMesh->textureRatios[1],
                            currentMesh->textureRatios[2],
                            currentMesh->textureRatios[3]);
            }
        }
        else 
        {
            glUniform1f(bHasTextureLocation, (GLfloat)GL_FALSE);
        }

        GLint doNotLightLocation = glGetUniformLocation(shaderID, "doNotLight");

        if (currentMesh->doNotLight) 
        {
            glUniform1f(doNotLightLocation, (GLfloat)GL_TRUE);
        }
        else 
        {
            glUniform1f(doNotLightLocation, (GLfloat)GL_FALSE);
        }

        // Uncomment to:
        // Randomize the positions of ALL the objects
        // in the scene post every x amount of frames
        // Cause why not?

        /*elapsed_frames++;
        if (elapsed_frames > 100) {
            for (int j = 0; j < meshArray.size(); j++) {
                cMeshInfo* theMesh = meshArray[j];
                RandomizePositions(theMesh);
            }
            elapsed_frames = 0;
        }*/

        if (theEditMode == SPECTATE) {
           /* f_count++;
            
            if (f_count > 100000) {
                counter++;
                f_count = 0;
            }*/
            if (counter > 2) {
                counter = 0;
            }
            if (cameraTarget == beholders[counter]->position)
            cameraEye = beholders[counter]->position - glm::vec3(100.f, -75.f, 0.f);
            cameraTarget = beholders[counter]->position;
        }

        if (currentMesh->meshName == "beholder") {

            if (currentMesh->friendlyName == "beholder2") {
                if (currentMesh->position == waypoints[0]->position)
                {
                    currentMesh->target = waypoints[1]->position - currentMesh->position;
                    currentMesh->rotation.y = 0.f;
                }
                if (currentMesh->position == waypoints[1]->position)
                {
                    currentMesh->target = waypoints[2]->position - currentMesh->position;
                    currentMesh->rotation.y = 67.55f;
                }
                if (currentMesh->position == waypoints[2]->position)
                {
                    currentMesh->target = waypoints[3]->position - currentMesh->position;
                    currentMesh->rotation.y = -135.10f;
                }
                if (currentMesh->position == waypoints[3]->position)
                {
                    currentMesh->target = waypoints[0]->position - currentMesh->position;
                    currentMesh->rotation.y = -67.55f;
                }
                
                currentMesh->target = glm::normalize(currentMesh->target);
                currentMesh->velocity = currentMesh->target * 0.25f;
                currentMesh->position += currentMesh->velocity;
            }
            
            if (currentMesh->friendlyName == "beholder0") {
                if (currentMesh->position == waypoints[4]->position)
                {
                    currentMesh->target = waypoints[5]->position - currentMesh->position;
                    currentMesh->rotation.y = 0.f;
                }
                if (currentMesh->position == waypoints[5]->position)
                {
                    currentMesh->target = waypoints[6]->position - currentMesh->position;
                    currentMesh->rotation.y = 67.55f;
                }
                if (currentMesh->position == waypoints[6]->position)
                {
                    currentMesh->target = waypoints[7]->position - currentMesh->position;
                    currentMesh->rotation.y = -135.10f;
                }
                if (currentMesh->position == waypoints[7]->position)
                {
                    currentMesh->target = waypoints[4]->position - currentMesh->position;
                    currentMesh->rotation.y = -67.55f;
                }
                
                currentMesh->target = glm::normalize(currentMesh->target);
                currentMesh->velocity = currentMesh->target * 0.25f;
                currentMesh->position += currentMesh->velocity;
            }

            if (currentMesh->friendlyName == "beholder1") {
                if (currentMesh->position == waypoints[8]->position)
                {
                    currentMesh->target = waypoints[9]->position - currentMesh->position;

                    currentMesh->rotation.y = -67.55f;
                }
                if (currentMesh->position == waypoints[9]->position)
                {
                    currentMesh->target = waypoints[10]->position - currentMesh->position;
                    currentMesh->rotation.y = 0.f;
                }
                if (currentMesh->position == waypoints[10]->position)
                {
                    currentMesh->target = waypoints[11]->position - currentMesh->position;
                    currentMesh->rotation.y = 67.55f;
                }
                if (currentMesh->position == waypoints[11]->position)
                {
                    currentMesh->target = waypoints[8]->position - currentMesh->position;
                    currentMesh->rotation.y = -135.10f;
                }
                
                currentMesh->target = glm::normalize(currentMesh->target);
                currentMesh->velocity = currentMesh->target * 0.25f;
                currentMesh->position += currentMesh->velocity;
            }   
        }

        glm::vec3 cursorPos;

        // Division is expensive
        cursorPos.x = width * 0.5;
        cursorPos.y = height * 0.5;

        glm::vec3 worldSpaceCoordinates = glm::unProject(cursorPos, view, projection, viewport);
        
        glm::normalize(worldSpaceCoordinates);
        
        if (mouseClick) {}

        GLint bIsSkyboxObjectLocation = glGetUniformLocation(shaderID, "bIsSkyboxObject");

        if (currentMesh->isSkyBoxMesh) {

            //skybox texture
            GLuint cubeMapTextureNumber = TextureMan->getTextureIDFromName("NightSky");
            GLuint texture30Unit = 30;			// Texture unit go from 0 to 79
            glActiveTexture(texture30Unit + GL_TEXTURE0);	// GL_TEXTURE0 = 33984
            glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMapTextureNumber);
            GLint skyboxTextureLocation = glGetUniformLocation(shaderID, "skyboxTexture");
            glUniform1i(skyboxTextureLocation, texture30Unit);

            glUniform1f(bIsSkyboxObjectLocation, (GLfloat)GL_TRUE);
            currentMesh->position = cameraEye;
            currentMesh->SetUniformScale(7500.f);
        }
        else {
            glUniform1f(bIsSkyboxObjectLocation, (GLfloat)GL_FALSE);
        }
        
        sModelDrawInfo modelInfo;
        if (VAOMan->FindDrawInfoByModelName(meshArray[i]->meshName, modelInfo)) {

            glBindVertexArray(modelInfo.VAO_ID);
            glDrawElements(GL_TRIANGLES, modelInfo.numberOfIndices, GL_UNSIGNED_INT, (void*)0);
            glBindVertexArray(0);
        }
        else {
            std::cout << "Model not found." << std::endl;
        }

        if (currentMesh->hasChildMeshes) {

            sModelDrawInfo modelInfo;
            if (VAOMan->FindDrawInfoByModelName(currentMesh->vecChildMeshes[0]->meshName, modelInfo)) {

                glBindVertexArray(modelInfo.VAO_ID);
                glDrawElements(GL_TRIANGLES, modelInfo.numberOfIndices, GL_UNSIGNED_INT, (void*)0);
                glBindVertexArray(0);
            }
            else {
                std::cout << "Model not found." << std::endl;
            }
        }


        // Only draw bounding box around meshes with this boolean value set to true
        if (currentMesh->drawBBox) {
            draw_bbox(currentMesh, shaderID, model);  /* 
                                                       *  pass in the model matrix after drawing
                                                       *  so it doesnt screw with the matrix values
                                                       */ 
        }
        else {
            currentMesh->drawBBox = false;
        }
    }

    // Render the crosshair
    crosshair.Update();
    
    glfwSwapBuffers(window);
    glfwPollEvents();

    //const GLubyte* vendor = glad_glGetString(GL_VENDOR); // Returns the vendor
    const GLubyte* renderer = glad_glGetString(GL_RENDERER); // Returns a hint to the model

    if (timeDiff >= 1.f / 30.f) {
        std::string frameRate = std::to_string((1.f / timeDiff) * frameCount);
        std::string frameTime = std::to_string((timeDiff / frameCount) * 1000);

        std::stringstream ss;
        ss << " Camera: " << "(" << cameraEye.x << ", " << cameraEye.y << ", " << cameraEye.z << ")"
           << " Target: Index = " << object_index << ", MeshName: " << meshArray[object_index]->friendlyName << ", Position: (" << meshArray[object_index]->position.x << ", " << meshArray[object_index]->position.y << ", " << meshArray[object_index]->position.z << ")"
           << " FPS: " << frameRate << " ms: " << frameTime << " Ship dt: " << speed << " GPU: " << renderer << " " << l << " Light atten: " << x << ", " << y << ", " << z;

        glfwSetWindowTitle(window, ss.str().c_str());

        beginTime = currentTime;
        frameCount = 0;
    }
}

void Shutdown() {

    glfwDestroyWindow(window);
    glfwTerminate();

    window = nullptr;
    delete window;

    crosshair.Shutdown();

    exit(EXIT_SUCCESS);
}

void ReadFromFile() {

    std::ifstream readFile("readFile.txt");
    std::string input0;

    while (readFile >> input0) {
        meshFiles.push_back(input0);
        readIndex++;
    }  
}

// All dungeon models loaded here
void LoadDungeonFloorPlan() {
    // Dungeon floor plan
    sModelDrawInfo floor_obj;
    LoadModel(meshFiles[13], floor_obj);
    if (!VAOMan->LoadModelIntoVAO("floor", floor_obj, shaderID)) {
        std::cerr << "Could not load model into VAO" << std::endl;
    }
    cMeshInfo* floor_mesh = new cMeshInfo();
    floor_mesh->meshName = "floor";
    floor_mesh->friendlyName = "floor0";
    floor_mesh->isWireframe = wireFrame;
    floor_mesh->useRGBAColour = true;
    floor_mesh->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    floor_mesh->hasTexture = false;
    floor_mesh->textures[0] = "";
    floor_mesh->textureRatios[0] = 1.0f;
    meshArray.push_back(floor_mesh);

    cMeshInfo* floor_mesh1 = new cMeshInfo();
    floor_mesh1->meshName = "floor";
    floor_mesh1->friendlyName = "floor1";
    floor_mesh1->isWireframe = wireFrame;
    floor_mesh1->useRGBAColour = true;
    floor_mesh1->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    floor_mesh1->hasTexture = false;
    floor_mesh1->textures[0] = "";
    floor_mesh1->textureRatios[0] = 1.0f;
    meshArray.push_back(floor_mesh1);

    cMeshInfo* floor_mesh2 = new cMeshInfo();
    floor_mesh2->meshName = "floor";
    floor_mesh2->friendlyName = "floor2";
    floor_mesh2->isWireframe = wireFrame;
    floor_mesh2->useRGBAColour = true;
    floor_mesh2->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    floor_mesh2->hasTexture = false;
    floor_mesh2->textures[0] = "";
    floor_mesh2->textureRatios[0] = 1.0f;
    meshArray.push_back(floor_mesh2);

    cMeshInfo* floor_mesh3 = new cMeshInfo();
    floor_mesh3->meshName = "floor";
    floor_mesh3->friendlyName = "floor3";
    floor_mesh3->isWireframe = wireFrame;
    floor_mesh3->useRGBAColour = true;
    floor_mesh3->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    floor_mesh3->hasTexture = false;
    floor_mesh3->textures[0] = "";
    floor_mesh3->textureRatios[0] = 1.0f;
    meshArray.push_back(floor_mesh3);

    cMeshInfo* floor_mesh4 = new cMeshInfo();
    floor_mesh4->meshName = "floor";
    floor_mesh4->friendlyName = "floor4";
    floor_mesh4->isWireframe = wireFrame;
    floor_mesh4->useRGBAColour = true;
    floor_mesh4->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    floor_mesh4->hasTexture = false;
    floor_mesh4->textures[0] = "";
    floor_mesh4->textureRatios[0] = 1.0f;
    meshArray.push_back(floor_mesh4);

    cMeshInfo* floor_mesh5 = new cMeshInfo();
    floor_mesh5->meshName = "floor";
    floor_mesh5->friendlyName = "floor5";
    floor_mesh5->isWireframe = wireFrame;
    floor_mesh5->useRGBAColour = true;
    floor_mesh5->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    floor_mesh5->hasTexture = false;
    floor_mesh5->textures[0] = "";
    floor_mesh5->textureRatios[0] = 1.0f;
    meshArray.push_back(floor_mesh5);

    cMeshInfo* floor_mesh6 = new cMeshInfo();
    floor_mesh6->meshName = "floor";
    floor_mesh6->friendlyName = "floor6";
    floor_mesh6->isWireframe = wireFrame;
    floor_mesh6->useRGBAColour = true;
    floor_mesh6->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    floor_mesh6->hasTexture = false;
    floor_mesh6->textures[0] = "";
    floor_mesh6->textureRatios[0] = 1.0f;
    meshArray.push_back(floor_mesh6);

    cMeshInfo* floor_mesh7 = new cMeshInfo();
    floor_mesh7->meshName = "floor";
    floor_mesh7->friendlyName = "floor7";
    floor_mesh7->isWireframe = wireFrame;
    floor_mesh7->useRGBAColour = true;
    floor_mesh7->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    floor_mesh7->hasTexture = false;
    floor_mesh7->textures[0] = "";
    floor_mesh7->textureRatios[0] = 1.0f;
    meshArray.push_back(floor_mesh7);

    cMeshInfo* floor_mesh8 = new cMeshInfo();
    floor_mesh8->meshName = "floor";
    floor_mesh8->friendlyName = "floor8";
    floor_mesh8->isWireframe = wireFrame;
    floor_mesh8->useRGBAColour = true;
    floor_mesh8->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    floor_mesh8->hasTexture = false;
    floor_mesh8->textures[0] = "";
    floor_mesh8->textureRatios[0] = 1.0f;
    meshArray.push_back(floor_mesh8);

    cMeshInfo* floor_mesh9 = new cMeshInfo();
    floor_mesh9->meshName = "floor";
    floor_mesh9->friendlyName = "floor9";
    floor_mesh9->isWireframe = wireFrame;
    floor_mesh9->useRGBAColour = true;
    floor_mesh9->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    floor_mesh9->hasTexture = false;
    floor_mesh9->textures[0] = "";
    floor_mesh9->textureRatios[0] = 1.0f;
    meshArray.push_back(floor_mesh9);

    cMeshInfo* floor_mesh10 = new cMeshInfo();
    floor_mesh10->meshName = "floor";
    floor_mesh10->friendlyName = "floor10";
    floor_mesh10->isWireframe = wireFrame;
    floor_mesh10->useRGBAColour = true;
    floor_mesh10->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    floor_mesh10->hasTexture = false;
    floor_mesh10->textures[0] = "";
    floor_mesh10->textureRatios[0] = 1.0f;
    meshArray.push_back(floor_mesh10);

    cMeshInfo* floor_mesh11 = new cMeshInfo();
    floor_mesh11->meshName = "floor";
    floor_mesh11->friendlyName = "floor11";
    floor_mesh11->isWireframe = wireFrame;
    floor_mesh11->useRGBAColour = true;
    floor_mesh11->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    floor_mesh11->hasTexture = false;
    floor_mesh11->textures[0] = "";
    floor_mesh11->textureRatios[0] = 1.0f;
    meshArray.push_back(floor_mesh11);

    cMeshInfo* floor_mesh12 = new cMeshInfo();
    floor_mesh12->meshName = "floor";
    floor_mesh12->friendlyName = "floor12";
    floor_mesh12->isWireframe = wireFrame;
    floor_mesh12->useRGBAColour = true;
    floor_mesh12->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    floor_mesh12->hasTexture = false;
    floor_mesh12->textures[0] = "";
    floor_mesh12->textureRatios[0] = 1.0f;
    meshArray.push_back(floor_mesh12);

    cMeshInfo* floor_mesh13 = new cMeshInfo();
    floor_mesh13->meshName = "floor";
    floor_mesh13->friendlyName = "floor13";
    floor_mesh13->isWireframe = wireFrame;
    floor_mesh13->useRGBAColour = true;
    floor_mesh13->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    floor_mesh13->hasTexture = false;
    floor_mesh13->textures[0] = "";
    floor_mesh13->textureRatios[0] = 1.0f;
    meshArray.push_back(floor_mesh13);

    cMeshInfo* floor_mesh14 = new cMeshInfo();
    floor_mesh14->meshName = "floor";
    floor_mesh14->friendlyName = "floor14";
    floor_mesh14->isWireframe = wireFrame;
    floor_mesh14->useRGBAColour = true;
    floor_mesh14->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    floor_mesh14->hasTexture = false;
    floor_mesh14->textures[0] = "";
    floor_mesh14->textureRatios[0] = 1.0f;
    meshArray.push_back(floor_mesh14);

    cMeshInfo* floor_mesh15 = new cMeshInfo();
    floor_mesh15->meshName = "floor";
    floor_mesh15->friendlyName = "floor15";
    floor_mesh15->isWireframe = wireFrame;
    floor_mesh15->useRGBAColour = true;
    floor_mesh15->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    floor_mesh15->hasTexture = false;
    floor_mesh15->textures[0] = "";
    floor_mesh15->textureRatios[0] = 1.0f;
    meshArray.push_back(floor_mesh15);

    cMeshInfo* floor_mesh16 = new cMeshInfo();
    floor_mesh16->meshName = "floor";
    floor_mesh16->friendlyName = "floor16";
    floor_mesh16->isWireframe = wireFrame;
    floor_mesh16->useRGBAColour = true;
    floor_mesh16->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    floor_mesh16->hasTexture = false;
    floor_mesh16->textures[0] = "";
    floor_mesh16->textureRatios[0] = 1.0f;
    meshArray.push_back(floor_mesh16);

    cMeshInfo* floor_mesh17 = new cMeshInfo();
    floor_mesh17->meshName = "floor";
    floor_mesh17->friendlyName = "floor17";
    floor_mesh17->isWireframe = wireFrame;
    floor_mesh17->useRGBAColour = true;
    floor_mesh17->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    floor_mesh17->hasTexture = false;
    floor_mesh17->textures[0] = "";
    floor_mesh17->textureRatios[0] = 1.0f;
    meshArray.push_back(floor_mesh17);

    cMeshInfo* floor_mesh18 = new cMeshInfo();
    floor_mesh18->meshName = "floor";
    floor_mesh18->friendlyName = "floor18";
    floor_mesh18->isWireframe = wireFrame;
    floor_mesh18->useRGBAColour = true;
    floor_mesh18->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    floor_mesh18->hasTexture = false;
    floor_mesh18->textures[0] = "";
    floor_mesh18->textureRatios[0] = 1.0f;
    meshArray.push_back(floor_mesh18);

    cMeshInfo* floor_mesh19 = new cMeshInfo();
    floor_mesh19->meshName = "floor";
    floor_mesh19->friendlyName = "floor19";
    floor_mesh19->isWireframe = wireFrame;
    floor_mesh19->useRGBAColour = true;
    floor_mesh19->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    floor_mesh19->hasTexture = false;
    floor_mesh19->textures[0] = "";
    floor_mesh19->textureRatios[0] = 1.0f;
    meshArray.push_back(floor_mesh19);

    cMeshInfo* floor_mesh20 = new cMeshInfo();
    floor_mesh20->meshName = "floor";
    floor_mesh20->friendlyName = "floor20";
    floor_mesh20->isWireframe = wireFrame;
    floor_mesh20->useRGBAColour = true;
    floor_mesh20->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    floor_mesh20->hasTexture = false;
    floor_mesh20->textures[0] = "";
    floor_mesh20->textureRatios[0] = 1.0f;
    meshArray.push_back(floor_mesh20);

    cMeshInfo* floor_mesh21 = new cMeshInfo();
    floor_mesh21->meshName = "floor";
    floor_mesh21->friendlyName = "floor21";
    floor_mesh21->isWireframe = wireFrame;
    floor_mesh21->useRGBAColour = true;
    floor_mesh21->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    floor_mesh21->hasTexture = false;
    floor_mesh21->textures[0] = "";
    floor_mesh21->textureRatios[0] = 1.0f;
    meshArray.push_back(floor_mesh21);

    cMeshInfo* floor_mesh22 = new cMeshInfo();
    floor_mesh22->meshName = "floor";
    floor_mesh22->friendlyName = "floor22";
    floor_mesh22->isWireframe = wireFrame;
    floor_mesh22->useRGBAColour = true;
    floor_mesh22->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    floor_mesh22->hasTexture = false;
    floor_mesh22->textures[0] = "";
    floor_mesh22->textureRatios[0] = 1.0f;
    meshArray.push_back(floor_mesh22);

    cMeshInfo* floor_mesh23 = new cMeshInfo();
    floor_mesh23->meshName = "floor";
    floor_mesh23->friendlyName = "floor23";
    floor_mesh23->isWireframe = wireFrame;
    floor_mesh23->useRGBAColour = true;
    floor_mesh23->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    floor_mesh23->hasTexture = false;
    floor_mesh23->textures[0] = "";
    floor_mesh23->textureRatios[0] = 1.0f;
    meshArray.push_back(floor_mesh23);

    cMeshInfo* floor_mesh24 = new cMeshInfo();
    floor_mesh24->meshName = "floor";
    floor_mesh24->friendlyName = "floor24";
    floor_mesh24->isWireframe = wireFrame;
    floor_mesh24->useRGBAColour = true;
    floor_mesh24->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    floor_mesh24->hasTexture = false;
    floor_mesh24->textures[0] = "";
    floor_mesh24->textureRatios[0] = 1.0f;
    meshArray.push_back(floor_mesh24);

    cMeshInfo* floor_mesh25 = new cMeshInfo();
    floor_mesh25->meshName = "floor";
    floor_mesh25->friendlyName = "floor25";
    floor_mesh25->isWireframe = wireFrame;
    floor_mesh25->useRGBAColour = true;
    floor_mesh25->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    floor_mesh25->hasTexture = false;
    floor_mesh25->textures[0] = "";
    floor_mesh25->textureRatios[0] = 1.0f;
    meshArray.push_back(floor_mesh25);

    cMeshInfo* floor_mesh26 = new cMeshInfo();
    floor_mesh26->meshName = "floor";
    floor_mesh26->friendlyName = "floor26";
    floor_mesh26->isWireframe = wireFrame;
    floor_mesh26->useRGBAColour = true;
    floor_mesh26->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    floor_mesh26->hasTexture = false;
    floor_mesh26->textures[0] = "";
    floor_mesh26->textureRatios[0] = 1.0f;
    meshArray.push_back(floor_mesh26);

    cMeshInfo* floor_mesh27 = new cMeshInfo();
    floor_mesh27->meshName = "floor";
    floor_mesh27->friendlyName = "floor27";
    floor_mesh27->isWireframe = wireFrame;
    floor_mesh27->useRGBAColour = true;
    floor_mesh27->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    floor_mesh27->hasTexture = false;
    floor_mesh27->textures[0] = "";
    floor_mesh27->textureRatios[0] = 1.0f;
    meshArray.push_back(floor_mesh27);

    cMeshInfo* floor_mesh28 = new cMeshInfo();
    floor_mesh28->meshName = "floor";
    floor_mesh28->friendlyName = "floor28";
    floor_mesh28->isWireframe = wireFrame;
    floor_mesh28->useRGBAColour = true;
    floor_mesh28->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    floor_mesh28->hasTexture = false;
    floor_mesh28->textures[0] = "";
    floor_mesh28->textureRatios[0] = 1.0f;
    meshArray.push_back(floor_mesh28);

    cMeshInfo* floor_mesh29 = new cMeshInfo();
    floor_mesh29->meshName = "floor";
    floor_mesh29->friendlyName = "floor29";
    floor_mesh29->isWireframe = wireFrame;
    floor_mesh29->useRGBAColour = true;
    floor_mesh29->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    floor_mesh29->hasTexture = false;
    floor_mesh29->textures[0] = "";
    floor_mesh29->textureRatios[0] = 1.0f;
    meshArray.push_back(floor_mesh29);

    cMeshInfo* floor_mesh30 = new cMeshInfo();
    floor_mesh30->meshName = "floor";
    floor_mesh30->friendlyName = "floor30";
    floor_mesh30->isWireframe = wireFrame;
    floor_mesh30->useRGBAColour = true;
    floor_mesh30->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    floor_mesh30->hasTexture = false;
    floor_mesh30->textures[0] = "";
    floor_mesh30->textureRatios[0] = 1.0f;
    meshArray.push_back(floor_mesh30);

    cMeshInfo* floor_mesh31 = new cMeshInfo();
    floor_mesh31->meshName = "floor";
    floor_mesh31->friendlyName = "floor31";
    floor_mesh31->isWireframe = wireFrame;
    floor_mesh31->useRGBAColour = true;
    floor_mesh31->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    floor_mesh31->hasTexture = false;
    floor_mesh31->textures[0] = "";
    floor_mesh31->textureRatios[0] = 1.0f;
    meshArray.push_back(floor_mesh31);

    cMeshInfo* floor_mesh32 = new cMeshInfo();
    floor_mesh32->meshName = "floor";
    floor_mesh32->friendlyName = "floor32";
    floor_mesh32->isWireframe = wireFrame;
    floor_mesh32->useRGBAColour = true;
    floor_mesh32->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    floor_mesh32->hasTexture = false;
    floor_mesh32->textures[0] = "";
    floor_mesh32->textureRatios[0] = 1.0f;
    meshArray.push_back(floor_mesh32);

    cMeshInfo* floor_mesh33 = new cMeshInfo();
    floor_mesh33->meshName = "floor";
    floor_mesh33->friendlyName = "floor33";
    floor_mesh33->isWireframe = wireFrame;
    floor_mesh33->useRGBAColour = true;
    floor_mesh33->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    floor_mesh33->hasTexture = false;
    floor_mesh33->textures[0] = "";
    floor_mesh33->textureRatios[0] = 1.0f;
    meshArray.push_back(floor_mesh33);

    cMeshInfo* floor_mesh34 = new cMeshInfo();
    floor_mesh34->meshName = "floor";
    floor_mesh34->friendlyName = "floor34";
    floor_mesh34->isWireframe = wireFrame;
    floor_mesh34->useRGBAColour = true;
    floor_mesh34->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    floor_mesh34->hasTexture = false;
    floor_mesh34->textures[0] = "";
    floor_mesh34->textureRatios[0] = 1.0f;
    meshArray.push_back(floor_mesh34);

    cMeshInfo* floor_mesh35 = new cMeshInfo();
    floor_mesh35->meshName = "floor";
    floor_mesh35->friendlyName = "floor35";
    floor_mesh35->isWireframe = wireFrame;
    floor_mesh35->useRGBAColour = true;
    floor_mesh35->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    floor_mesh35->hasTexture = false;
    floor_mesh35->textures[0] = "";
    floor_mesh35->textureRatios[0] = 1.0f;
    meshArray.push_back(floor_mesh35);

    cMeshInfo* floor_mesh36 = new cMeshInfo();
    floor_mesh36->meshName = "floor";
    floor_mesh36->friendlyName = "floor36";
    floor_mesh36->isWireframe = wireFrame;
    floor_mesh36->useRGBAColour = true;
    floor_mesh36->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    floor_mesh36->hasTexture = false;
    floor_mesh36->textures[0] = "";
    floor_mesh36->textureRatios[0] = 1.0f;
    meshArray.push_back(floor_mesh36);

    cMeshInfo* floor_mesh37 = new cMeshInfo();
    floor_mesh37->meshName = "floor";
    floor_mesh37->friendlyName = "floor37";
    floor_mesh37->isWireframe = wireFrame;
    floor_mesh37->useRGBAColour = true;
    floor_mesh37->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    floor_mesh37->hasTexture = false;
    floor_mesh37->textures[0] = "";
    floor_mesh37->textureRatios[0] = 1.0f;
    meshArray.push_back(floor_mesh37);

    cMeshInfo* floor_mesh38 = new cMeshInfo();
    floor_mesh38->meshName = "floor";
    floor_mesh38->friendlyName = "floor38";
    floor_mesh38->isWireframe = wireFrame;
    floor_mesh38->useRGBAColour = true;
    floor_mesh38->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    floor_mesh38->hasTexture = false;
    floor_mesh38->textures[0] = "";
    floor_mesh38->textureRatios[0] = 1.0f;
    meshArray.push_back(floor_mesh38);

    cMeshInfo* floor_mesh39 = new cMeshInfo();
    floor_mesh39->meshName = "floor";
    floor_mesh39->friendlyName = "floor39";
    floor_mesh39->isWireframe = wireFrame;
    floor_mesh39->useRGBAColour = true;
    floor_mesh39->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    floor_mesh39->hasTexture = false;
    floor_mesh39->textures[0] = "";
    floor_mesh39->textureRatios[0] = 1.0f;
    meshArray.push_back(floor_mesh39);

    cMeshInfo* floor_mesh40 = new cMeshInfo();
    floor_mesh40->meshName = "floor";
    floor_mesh40->friendlyName = "floor40";
    floor_mesh40->isWireframe = wireFrame;
    floor_mesh40->useRGBAColour = true;
    floor_mesh40->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    floor_mesh40->hasTexture = false;
    floor_mesh40->textures[0] = "";
    floor_mesh40->textureRatios[0] = 1.0f;
    meshArray.push_back(floor_mesh40);

    cMeshInfo* floor_mesh41 = new cMeshInfo();
    floor_mesh41->meshName = "floor";
    floor_mesh41->friendlyName = "floor41";
    floor_mesh41->isWireframe = wireFrame;
    floor_mesh41->useRGBAColour = true;
    floor_mesh41->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    floor_mesh41->hasTexture = false;
    floor_mesh41->textures[0] = "";
    floor_mesh41->textureRatios[0] = 1.0f;
    meshArray.push_back(floor_mesh41);

    cMeshInfo* floor_mesh42 = new cMeshInfo();
    floor_mesh42->meshName = "floor";
    floor_mesh42->friendlyName = "floor42";
    floor_mesh42->isWireframe = wireFrame;
    floor_mesh42->useRGBAColour = true;
    floor_mesh42->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    floor_mesh42->hasTexture = false;
    floor_mesh42->textures[0] = "";
    floor_mesh42->textureRatios[0] = 1.0f;
    meshArray.push_back(floor_mesh42);

    cMeshInfo* floor_mesh43 = new cMeshInfo();
    floor_mesh43->meshName = "floor";
    floor_mesh43->friendlyName = "floor43";
    floor_mesh43->isWireframe = wireFrame;
    floor_mesh43->useRGBAColour = true;
    floor_mesh43->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    floor_mesh43->hasTexture = false;
    floor_mesh43->textures[0] = "";
    floor_mesh43->textureRatios[0] = 1.0f;
    meshArray.push_back(floor_mesh43);

    cMeshInfo* floor_mesh44 = new cMeshInfo();
    floor_mesh44->meshName = "floor";
    floor_mesh44->friendlyName = "floor44";
    floor_mesh44->isWireframe = wireFrame;
    floor_mesh44->useRGBAColour = true;
    floor_mesh44->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    floor_mesh44->hasTexture = false;
    floor_mesh44->textures[0] = "";
    floor_mesh44->textureRatios[0] = 1.0f;
    meshArray.push_back(floor_mesh44);

    cMeshInfo* floor_mesh45 = new cMeshInfo();
    floor_mesh45->meshName = "floor";
    floor_mesh45->friendlyName = "floor45";
    floor_mesh45->isWireframe = wireFrame;
    floor_mesh45->useRGBAColour = true;
    floor_mesh45->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    floor_mesh45->hasTexture = false;
    floor_mesh45->textures[0] = "";
    floor_mesh45->textureRatios[0] = 1.0f;
    meshArray.push_back(floor_mesh45);

    cMeshInfo* floor_mesh46 = new cMeshInfo();
    floor_mesh46->meshName = "floor";
    floor_mesh46->friendlyName = "floor46";
    floor_mesh46->isWireframe = wireFrame;
    floor_mesh46->useRGBAColour = true;
    floor_mesh46->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    floor_mesh46->hasTexture = false;
    floor_mesh46->textures[0] = "";
    floor_mesh46->textureRatios[0] = 1.0f;
    meshArray.push_back(floor_mesh46);

    cMeshInfo* floor_mesh47 = new cMeshInfo();
    floor_mesh47->meshName = "floor";
    floor_mesh47->friendlyName = "floor47";
    floor_mesh47->isWireframe = wireFrame;
    floor_mesh47->useRGBAColour = true;
    floor_mesh47->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    floor_mesh47->hasTexture = false;
    floor_mesh47->textures[0] = "";
    floor_mesh47->textureRatios[0] = 1.0f;
    meshArray.push_back(floor_mesh47);

    cMeshInfo* floor_mesh48 = new cMeshInfo();
    floor_mesh48->meshName = "floor";
    floor_mesh48->friendlyName = "floor48";
    floor_mesh48->isWireframe = wireFrame;
    floor_mesh48->useRGBAColour = true;
    floor_mesh48->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    floor_mesh48->hasTexture = false;
    floor_mesh48->textures[0] = "";
    floor_mesh48->textureRatios[0] = 1.0f;
    meshArray.push_back(floor_mesh48);

    cMeshInfo* floor_mesh49 = new cMeshInfo();
    floor_mesh49->meshName = "floor";
    floor_mesh49->friendlyName = "floor49";
    floor_mesh49->isWireframe = wireFrame;
    floor_mesh49->useRGBAColour = true;
    floor_mesh49->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    floor_mesh49->hasTexture = false;
    floor_mesh49->textures[0] = "";
    floor_mesh49->textureRatios[0] = 1.0f;
    meshArray.push_back(floor_mesh49);

    cMeshInfo* floor_mesh50 = new cMeshInfo();
    floor_mesh50->meshName = "floor";
    floor_mesh50->friendlyName = "floor50";
    floor_mesh50->isWireframe = wireFrame;
    floor_mesh50->useRGBAColour = true;
    floor_mesh50->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    floor_mesh50->hasTexture = false;
    floor_mesh50->textures[0] = "";
    floor_mesh50->textureRatios[0] = 1.0f;
    meshArray.push_back(floor_mesh17);

    cMeshInfo* floor_mesh51 = new cMeshInfo();
    floor_mesh51->meshName = "floor";
    floor_mesh51->friendlyName = "floor51";
    floor_mesh51->isWireframe = wireFrame;
    floor_mesh51->useRGBAColour = true;
    floor_mesh51->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    floor_mesh51->hasTexture = false;
    floor_mesh51->textures[0] = "";
    floor_mesh51->textureRatios[0] = 1.0f;
    meshArray.push_back(floor_mesh51);

    cMeshInfo* floor_mesh52 = new cMeshInfo();
    floor_mesh52->meshName = "floor";
    floor_mesh52->friendlyName = "floor52";
    floor_mesh52->isWireframe = wireFrame;
    floor_mesh52->useRGBAColour = true;
    floor_mesh52->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    floor_mesh52->hasTexture = false;
    floor_mesh52->textures[0] = "";
    floor_mesh52->textureRatios[0] = 1.0f;
    meshArray.push_back(floor_mesh52);

    cMeshInfo* floor_mesh53 = new cMeshInfo();
    floor_mesh53->meshName = "floor";
    floor_mesh53->friendlyName = "floor53";
    floor_mesh53->isWireframe = wireFrame;
    floor_mesh53->useRGBAColour = true;
    floor_mesh53->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    floor_mesh53->hasTexture = false;
    floor_mesh53->textures[0] = "";
    floor_mesh53->textureRatios[0] = 1.0f;
    meshArray.push_back(floor_mesh53);

    cMeshInfo* floor_mesh54 = new cMeshInfo();
    floor_mesh54->meshName = "floor";
    floor_mesh54->friendlyName = "floor54";
    floor_mesh54->isWireframe = wireFrame;
    floor_mesh54->useRGBAColour = true;
    floor_mesh54->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    floor_mesh54->hasTexture = false;
    floor_mesh54->textures[0] = "";
    floor_mesh54->textureRatios[0] = 1.0f;
    meshArray.push_back(floor_mesh54);

    cMeshInfo* floor_mesh55 = new cMeshInfo();
    floor_mesh55->meshName = "floor";
    floor_mesh55->friendlyName = "floor55";
    floor_mesh55->isWireframe = wireFrame;
    floor_mesh55->useRGBAColour = true;
    floor_mesh55->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    floor_mesh55->hasTexture = false;
    floor_mesh55->textures[0] = "";
    floor_mesh55->textureRatios[0] = 1.0f;
    meshArray.push_back(floor_mesh55);

    cMeshInfo* floor_mesh56 = new cMeshInfo();
    floor_mesh56->meshName = "floor";
    floor_mesh56->friendlyName = "floor56";
    floor_mesh56->isWireframe = wireFrame;
    floor_mesh56->useRGBAColour = true;
    floor_mesh56->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    floor_mesh56->hasTexture = false;
    floor_mesh56->textures[0] = "";
    floor_mesh56->textureRatios[0] = 1.0f;
    meshArray.push_back(floor_mesh56);

    cMeshInfo* floor_mesh57 = new cMeshInfo();
    floor_mesh57->meshName = "floor";
    floor_mesh57->friendlyName = "floor57";
    floor_mesh57->isWireframe = wireFrame;
    floor_mesh57->useRGBAColour = true;
    floor_mesh57->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    floor_mesh57->hasTexture = false;
    floor_mesh57->textures[0] = "";
    floor_mesh57->textureRatios[0] = 1.0f;
    meshArray.push_back(floor_mesh57);

    cMeshInfo* floor_mesh58 = new cMeshInfo();
    floor_mesh58->meshName = "floor";
    floor_mesh58->friendlyName = "floor58";
    floor_mesh58->isWireframe = wireFrame;
    floor_mesh58->useRGBAColour = true;
    floor_mesh58->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    floor_mesh58->hasTexture = false;
    floor_mesh58->textures[0] = "";
    floor_mesh58->textureRatios[0] = 1.0f;
    meshArray.push_back(floor_mesh58);

    cMeshInfo* floor_mesh59 = new cMeshInfo();
    floor_mesh59->meshName = "floor";
    floor_mesh59->friendlyName = "floor59";
    floor_mesh59->isWireframe = wireFrame;
    floor_mesh59->useRGBAColour = true;
    floor_mesh59->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    floor_mesh59->hasTexture = false;
    floor_mesh59->textures[0] = "";
    floor_mesh59->textureRatios[0] = 1.0f;
    meshArray.push_back(floor_mesh59);

    cMeshInfo* floor_mesh60 = new cMeshInfo();
    floor_mesh60->meshName = "floor";
    floor_mesh60->friendlyName = "floor60";
    floor_mesh60->isWireframe = wireFrame;
    floor_mesh60->useRGBAColour = true;
    floor_mesh60->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    floor_mesh60->hasTexture = false;
    floor_mesh60->textures[0] = "";
    floor_mesh60->textureRatios[0] = 1.0f;
    meshArray.push_back(floor_mesh60);

    cMeshInfo* floor_mesh61 = new cMeshInfo();
    floor_mesh61->meshName = "floor";
    floor_mesh61->friendlyName = "floor61";
    floor_mesh61->isWireframe = wireFrame;
    floor_mesh61->useRGBAColour = true;
    floor_mesh61->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    floor_mesh61->hasTexture = false;
    floor_mesh61->textures[0] = "";
    floor_mesh61->textureRatios[0] = 1.0f;
    meshArray.push_back(floor_mesh61);

    cMeshInfo* floor_mesh62 = new cMeshInfo();
    floor_mesh62->meshName = "floor";
    floor_mesh62->friendlyName = "floor62";
    floor_mesh62->isWireframe = wireFrame;
    floor_mesh62->useRGBAColour = true;
    floor_mesh62->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    floor_mesh62->hasTexture = false;
    floor_mesh62->textures[0] = "";
    floor_mesh62->textureRatios[0] = 1.0f;
    meshArray.push_back(floor_mesh62);

    cMeshInfo* floor_mesh63 = new cMeshInfo();
    floor_mesh63->meshName = "floor";
    floor_mesh63->friendlyName = "floor63";
    floor_mesh63->isWireframe = wireFrame;
    floor_mesh63->useRGBAColour = true;
    floor_mesh63->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    floor_mesh63->hasTexture = false;
    floor_mesh63->textures[0] = "";
    floor_mesh63->textureRatios[0] = 1.0f;
    meshArray.push_back(floor_mesh63);

    cMeshInfo* floor_mesh64 = new cMeshInfo();
    floor_mesh64->meshName = "floor";
    floor_mesh64->friendlyName = "floor64";
    floor_mesh64->isWireframe = wireFrame;
    floor_mesh64->useRGBAColour = true;
    floor_mesh64->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    floor_mesh64->hasTexture = false;
    floor_mesh64->textures[0] = "";
    floor_mesh64->textureRatios[0] = 1.0f;
    meshArray.push_back(floor_mesh64);

    cMeshInfo* floor_mesh65 = new cMeshInfo();
    floor_mesh65->meshName = "floor";
    floor_mesh65->friendlyName = "floor65";
    floor_mesh65->isWireframe = wireFrame;
    floor_mesh65->useRGBAColour = true;
    floor_mesh65->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    floor_mesh65->hasTexture = false;
    floor_mesh65->textures[0] = "";
    floor_mesh65->textureRatios[0] = 1.0f;
    meshArray.push_back(floor_mesh65);

    cMeshInfo* floor_mesh66 = new cMeshInfo();
    floor_mesh66->meshName = "floor";
    floor_mesh66->friendlyName = "floor66";
    floor_mesh66->isWireframe = wireFrame;
    floor_mesh66->useRGBAColour = true;
    floor_mesh66->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    floor_mesh66->hasTexture = false;
    floor_mesh66->textures[0] = "";
    floor_mesh66->textureRatios[0] = 1.0f;
    meshArray.push_back(floor_mesh66);

    cMeshInfo* floor_mesh67 = new cMeshInfo();
    floor_mesh67->meshName = "floor";
    floor_mesh67->friendlyName = "floor67";
    floor_mesh67->isWireframe = wireFrame;
    floor_mesh67->useRGBAColour = true;
    floor_mesh67->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    floor_mesh67->hasTexture = false;
    floor_mesh67->textures[0] = "";
    floor_mesh67->textureRatios[0] = 1.0f;
    meshArray.push_back(floor_mesh67);

    cMeshInfo* floor_mesh68 = new cMeshInfo();
    floor_mesh68->meshName = "floor";
    floor_mesh68->friendlyName = "floor68";
    floor_mesh68->isWireframe = wireFrame;
    floor_mesh68->useRGBAColour = true;
    floor_mesh68->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    floor_mesh68->hasTexture = false;
    floor_mesh68->textures[0] = "";
    floor_mesh68->textureRatios[0] = 1.0f;
    meshArray.push_back(floor_mesh68);

    cMeshInfo* floor_mesh69 = new cMeshInfo();
    floor_mesh69->meshName = "floor";
    floor_mesh69->friendlyName = "floor69";
    floor_mesh69->isWireframe = wireFrame;
    floor_mesh69->useRGBAColour = true;
    floor_mesh69->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    floor_mesh69->hasTexture = false;
    floor_mesh69->textures[0] = "";
    floor_mesh69->textureRatios[0] = 1.0f;
    meshArray.push_back(floor_mesh69);

    cMeshInfo* floor_mesh70 = new cMeshInfo();
    floor_mesh70->meshName = "floor";
    floor_mesh70->friendlyName = "floor70";
    floor_mesh70->isWireframe = wireFrame;
    floor_mesh70->useRGBAColour = true;
    floor_mesh70->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    floor_mesh70->hasTexture = false;
    floor_mesh70->textures[0] = "";
    floor_mesh70->textureRatios[0] = 1.0f;
    meshArray.push_back(floor_mesh70);

    cMeshInfo* floor_mesh71 = new cMeshInfo();
    floor_mesh71->meshName = "floor";
    floor_mesh71->friendlyName = "floor71";
    floor_mesh71->isWireframe = wireFrame;
    floor_mesh71->useRGBAColour = true;
    floor_mesh71->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    floor_mesh71->hasTexture = false;
    floor_mesh71->textures[0] = "";
    floor_mesh71->textureRatios[0] = 1.0f;
    meshArray.push_back(floor_mesh71);

    cMeshInfo* floor_mesh72 = new cMeshInfo();
    floor_mesh72->meshName = "floor";
    floor_mesh72->friendlyName = "floor72";
    floor_mesh72->isWireframe = wireFrame;
    floor_mesh72->useRGBAColour = true;
    floor_mesh72->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    floor_mesh72->hasTexture = false;
    floor_mesh72->textures[0] = "";
    floor_mesh72->textureRatios[0] = 1.0f;
    meshArray.push_back(floor_mesh72);

    cMeshInfo* floor_mesh73 = new cMeshInfo();
    floor_mesh73->meshName = "floor";
    floor_mesh73->friendlyName = "floor73";
    floor_mesh73->isWireframe = wireFrame;
    floor_mesh73->useRGBAColour = true;
    floor_mesh73->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    floor_mesh73->hasTexture = false;
    floor_mesh73->textures[0] = "";
    floor_mesh73->textureRatios[0] = 1.0f;
    meshArray.push_back(floor_mesh73);

    cMeshInfo* floor_mesh74 = new cMeshInfo();
    floor_mesh74->meshName = "floor";
    floor_mesh74->friendlyName = "floor74";
    floor_mesh74->isWireframe = wireFrame;
    floor_mesh74->useRGBAColour = true;
    floor_mesh74->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    floor_mesh74->hasTexture = false;
    floor_mesh74->textures[0] = "";
    floor_mesh74->textureRatios[0] = 1.0f;
    meshArray.push_back(floor_mesh74);
    
    cMeshInfo* floor_mesh75 = new cMeshInfo();
    floor_mesh75->meshName = "floor";
    floor_mesh75->friendlyName = "floor75";
    floor_mesh75->isWireframe = wireFrame;
    floor_mesh75->useRGBAColour = true;
    floor_mesh75->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    floor_mesh75->hasTexture = false;
    floor_mesh75->textures[0] = "";
    floor_mesh75->textureRatios[0] = 1.0f;
    meshArray.push_back(floor_mesh75);

    cMeshInfo* floor_mesh76 = new cMeshInfo();
    floor_mesh76->meshName = "floor";
    floor_mesh76->friendlyName = "floor76";
    floor_mesh76->isWireframe = wireFrame;
    floor_mesh76->useRGBAColour = true;
    floor_mesh76->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    floor_mesh76->hasTexture = false;
    floor_mesh76->textures[0] = "";
    floor_mesh76->textureRatios[0] = 1.0f;
    meshArray.push_back(floor_mesh76);

    cMeshInfo* floor_mesh77 = new cMeshInfo();
    floor_mesh77->meshName = "floor";
    floor_mesh77->friendlyName = "floor77";
    floor_mesh77->isWireframe = wireFrame;
    floor_mesh77->useRGBAColour = true;
    floor_mesh77->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    floor_mesh77->hasTexture = false;
    floor_mesh77->textures[0] = "";
    floor_mesh77->textureRatios[0] = 1.0f;
    meshArray.push_back(floor_mesh77);

    cMeshInfo* floor_mesh78 = new cMeshInfo();
    floor_mesh78->meshName = "floor";
    floor_mesh78->friendlyName = "floor78";
    floor_mesh78->isWireframe = wireFrame;
    floor_mesh78->useRGBAColour = true;
    floor_mesh78->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    floor_mesh78->hasTexture = false;
    floor_mesh78->textures[0] = "";
    floor_mesh78->textureRatios[0] = 1.0f;
    meshArray.push_back(floor_mesh78);

    cMeshInfo* floor_mesh79 = new cMeshInfo();
    floor_mesh79->meshName = "floor";
    floor_mesh79->friendlyName = "floor79";
    floor_mesh79->isWireframe = wireFrame;
    floor_mesh79->useRGBAColour = true;
    floor_mesh79->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    floor_mesh79->hasTexture = false;
    floor_mesh79->textures[0] = "";
    floor_mesh79->textureRatios[0] = 1.0f;
    meshArray.push_back(floor_mesh79);

    cMeshInfo* floor_mesh80 = new cMeshInfo();
    floor_mesh80->meshName = "floor";
    floor_mesh80->friendlyName = "floor80";
    floor_mesh80->isWireframe = wireFrame;
    floor_mesh80->useRGBAColour = true;
    floor_mesh80->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    floor_mesh80->hasTexture = false;
    floor_mesh80->textures[0] = "";
    floor_mesh80->textureRatios[0] = 1.0f;
    meshArray.push_back(floor_mesh80);

    cMeshInfo* floor_mesh81 = new cMeshInfo();
    floor_mesh81->meshName = "floor";
    floor_mesh81->friendlyName = "floor81";
    floor_mesh81->isWireframe = wireFrame;
    floor_mesh81->useRGBAColour = true;
    floor_mesh81->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    floor_mesh81->hasTexture = false;
    floor_mesh81->textures[0] = "";
    floor_mesh81->textureRatios[0] = 1.0f;
    meshArray.push_back(floor_mesh81);

    cMeshInfo* floor_mesh82 = new cMeshInfo();
    floor_mesh82->meshName = "floor";
    floor_mesh82->friendlyName = "floor82";
    floor_mesh82->isWireframe = wireFrame;
    floor_mesh82->useRGBAColour = true;
    floor_mesh82->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    floor_mesh82->hasTexture = false;
    floor_mesh82->textures[0] = "";
    floor_mesh82->textureRatios[0] = 1.0f;
    meshArray.push_back(floor_mesh82);

    cMeshInfo* floor_mesh83 = new cMeshInfo();
    floor_mesh83->meshName = "floor";
    floor_mesh83->friendlyName = "floor83";
    floor_mesh83->isWireframe = wireFrame;
    floor_mesh83->useRGBAColour = true;
    floor_mesh83->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    floor_mesh83->hasTexture = false;
    floor_mesh83->textures[0] = "";
    floor_mesh83->textureRatios[0] = 1.0f;
    meshArray.push_back(floor_mesh83);

    cMeshInfo* floor_mesh84 = new cMeshInfo();
    floor_mesh84->meshName = "floor";
    floor_mesh84->friendlyName = "floor84";
    floor_mesh84->isWireframe = wireFrame;
    floor_mesh84->useRGBAColour = true;
    floor_mesh84->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    floor_mesh84->hasTexture = false;
    floor_mesh84->textures[0] = "";
    floor_mesh84->textureRatios[0] = 1.0f;
    meshArray.push_back(floor_mesh84);

    cMeshInfo* floor_mesh85 = new cMeshInfo();
    floor_mesh85->meshName = "floor";
    floor_mesh85->friendlyName = "floor85";
    floor_mesh85->isWireframe = wireFrame;
    floor_mesh85->useRGBAColour = true;
    floor_mesh85->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    floor_mesh85->hasTexture = false;
    floor_mesh85->textures[0] = "";
    floor_mesh85->textureRatios[0] = 1.0f;
    meshArray.push_back(floor_mesh85);

    cMeshInfo* floor_mesh86 = new cMeshInfo();
    floor_mesh86->meshName = "floor";
    floor_mesh86->friendlyName = "floor86";
    floor_mesh86->isWireframe = wireFrame;
    floor_mesh86->useRGBAColour = true;
    floor_mesh86->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    floor_mesh86->hasTexture = false;
    floor_mesh86->textures[0] = "";
    floor_mesh86->textureRatios[0] = 1.0f;
    meshArray.push_back(floor_mesh86);

    cMeshInfo* floor_mesh87 = new cMeshInfo();
    floor_mesh87->meshName = "floor";
    floor_mesh87->friendlyName = "floor87";
    floor_mesh87->isWireframe = wireFrame;
    floor_mesh87->useRGBAColour = true;
    floor_mesh87->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    floor_mesh87->hasTexture = false;
    floor_mesh87->textures[0] = "";
    floor_mesh87->textureRatios[0] = 1.0f;
    meshArray.push_back(floor_mesh87);
    
    cMeshInfo* floor_mesh88 = new cMeshInfo();
    floor_mesh88->meshName = "floor";
    floor_mesh88->friendlyName = "floor88";
    floor_mesh88->isWireframe = wireFrame;
    floor_mesh88->useRGBAColour = true;
    floor_mesh88->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    floor_mesh88->hasTexture = false;
    floor_mesh88->textures[0] = "";
    floor_mesh88->textureRatios[0] = 1.0f;
    meshArray.push_back(floor_mesh88);

    cMeshInfo* floor_mesh89 = new cMeshInfo();
    floor_mesh89->meshName = "floor";
    floor_mesh89->friendlyName = "floor89";
    floor_mesh89->isWireframe = wireFrame;
    floor_mesh89->useRGBAColour = true;
    floor_mesh89->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    floor_mesh89->hasTexture = false;
    floor_mesh89->textures[0] = "";
    floor_mesh89->textureRatios[0] = 1.0f;
    meshArray.push_back(floor_mesh89);

    cMeshInfo* floor_mesh90 = new cMeshInfo();
    floor_mesh90->meshName = "floor";
    floor_mesh90->friendlyName = "floor90";
    floor_mesh90->isWireframe = wireFrame;
    floor_mesh90->useRGBAColour = true;
    floor_mesh90->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    floor_mesh90->hasTexture = false;
    floor_mesh90->textures[0] = "";
    floor_mesh90->textureRatios[0] = 1.0f;
    meshArray.push_back(floor_mesh90);

    cMeshInfo* floor_mesh91 = new cMeshInfo();
    floor_mesh91->meshName = "floor";
    floor_mesh91->friendlyName = "floor91";
    floor_mesh91->isWireframe = wireFrame;
    floor_mesh91->useRGBAColour = true;
    floor_mesh91->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    floor_mesh91->hasTexture = false;
    floor_mesh91->textures[0] = "";
    floor_mesh91->textureRatios[0] = 1.0f;
    meshArray.push_back(floor_mesh91);

    cMeshInfo* floor_mesh92 = new cMeshInfo();
    floor_mesh92->meshName = "floor";
    floor_mesh92->friendlyName = "floor92";
    floor_mesh92->isWireframe = wireFrame;
    floor_mesh92->useRGBAColour = true;
    floor_mesh92->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    floor_mesh92->hasTexture = false;
    floor_mesh92->textures[0] = "";
    floor_mesh92->textureRatios[0] = 1.0f;
    meshArray.push_back(floor_mesh92);

    cMeshInfo* floor_mesh93 = new cMeshInfo();
    floor_mesh93->meshName = "floor";
    floor_mesh93->friendlyName = "floor93";
    floor_mesh93->isWireframe = wireFrame;
    floor_mesh93->useRGBAColour = true;
    floor_mesh93->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    floor_mesh93->hasTexture = false;
    floor_mesh93->textures[0] = "";
    floor_mesh93->textureRatios[0] = 1.0f;
    meshArray.push_back(floor_mesh93);

    cMeshInfo* floor_mesh94 = new cMeshInfo();
    floor_mesh94->meshName = "floor";
    floor_mesh94->friendlyName = "floor94";
    floor_mesh94->isWireframe = wireFrame;
    floor_mesh94->useRGBAColour = true;
    floor_mesh94->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    floor_mesh94->hasTexture = false;
    floor_mesh94->textures[0] = "";
    floor_mesh94->textureRatios[0] = 1.0f;
    meshArray.push_back(floor_mesh94);

    cMeshInfo* floor_mesh95 = new cMeshInfo();
    floor_mesh95->meshName = "floor";
    floor_mesh95->friendlyName = "floor95";
    floor_mesh95->isWireframe = wireFrame;
    floor_mesh95->useRGBAColour = true;
    floor_mesh95->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    floor_mesh95->hasTexture = false;
    floor_mesh95->textures[0] = "";
    floor_mesh95->textureRatios[0] = 1.0f;
    meshArray.push_back(floor_mesh95);

    cMeshInfo* floor_mesh96 = new cMeshInfo();
    floor_mesh96->meshName = "floor";
    floor_mesh96->friendlyName = "floor96";
    floor_mesh96->isWireframe = wireFrame;
    floor_mesh96->useRGBAColour = true;
    floor_mesh96->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    floor_mesh96->hasTexture = false;
    floor_mesh96->textures[0] = "";
    floor_mesh96->textureRatios[0] = 1.0f;
    meshArray.push_back(floor_mesh96);

    cMeshInfo* floor_mesh97 = new cMeshInfo();
    floor_mesh97->meshName = "floor";
    floor_mesh97->friendlyName = "floor97";
    floor_mesh97->isWireframe = wireFrame;
    floor_mesh97->useRGBAColour = true;
    floor_mesh97->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    floor_mesh97->hasTexture = false;
    floor_mesh97->textures[0] = "";
    floor_mesh97->textureRatios[0] = 1.0f;
    meshArray.push_back(floor_mesh97);

    cMeshInfo* floor_mesh98 = new cMeshInfo();
    floor_mesh98->meshName = "floor";
    floor_mesh98->friendlyName = "floor98";
    floor_mesh98->isWireframe = wireFrame;
    floor_mesh98->useRGBAColour = true;
    floor_mesh98->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    floor_mesh98->hasTexture = false;
    floor_mesh98->textures[0] = "";
    floor_mesh98->textureRatios[0] = 1.0f;
    meshArray.push_back(floor_mesh98);

    cMeshInfo* floor_mesh99 = new cMeshInfo();
    floor_mesh99->meshName = "floor";
    floor_mesh99->friendlyName = "floor99";
    floor_mesh99->isWireframe = wireFrame;
    floor_mesh99->useRGBAColour = true;
    floor_mesh99->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    floor_mesh99->hasTexture = false;
    floor_mesh99->textures[0] = "";
    floor_mesh99->textureRatios[0] = 1.0f;
    meshArray.push_back(floor_mesh99);

    cMeshInfo* floor_mesh100 = new cMeshInfo();
    floor_mesh100->meshName = "floor";
    floor_mesh100->friendlyName = "floor100";
    floor_mesh100->isWireframe = wireFrame;
    floor_mesh100->useRGBAColour = true;
    floor_mesh100->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    floor_mesh100->hasTexture = false;
    floor_mesh100->textures[0] = "";
    floor_mesh100->textureRatios[0] = 1.0f;
    meshArray.push_back(floor_mesh100);

    cMeshInfo* floor_mesh101 = new cMeshInfo();
    floor_mesh101->meshName = "floor";
    floor_mesh101->friendlyName = "floor101";
    floor_mesh101->isWireframe = wireFrame;
    floor_mesh101->useRGBAColour = true;
    floor_mesh101->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    floor_mesh101->hasTexture = false;
    floor_mesh101->textures[0] = "";
    floor_mesh101->textureRatios[0] = 1.0f;
    meshArray.push_back(floor_mesh101);

    cMeshInfo* floor_mesh102 = new cMeshInfo();
    floor_mesh102->meshName = "floor";
    floor_mesh102->friendlyName = "floor102";
    floor_mesh102->isWireframe = wireFrame;
    floor_mesh102->useRGBAColour = true;
    floor_mesh102->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    floor_mesh102->hasTexture = false;
    floor_mesh102->textures[0] = "";
    floor_mesh102->textureRatios[0] = 1.0f;
    meshArray.push_back(floor_mesh102);

    cMeshInfo* floor_mesh103 = new cMeshInfo();
    floor_mesh103->meshName = "floor";
    floor_mesh103->friendlyName = "floor103";
    floor_mesh103->isWireframe = wireFrame;
    floor_mesh103->useRGBAColour = true;
    floor_mesh103->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    floor_mesh103->hasTexture = false;
    floor_mesh103->textures[0] = "";
    floor_mesh103->textureRatios[0] = 1.0f;
    meshArray.push_back(floor_mesh103);

    cMeshInfo* floor_mesh104 = new cMeshInfo();
    floor_mesh104->meshName = "floor";
    floor_mesh104->friendlyName = "floor104";
    floor_mesh104->isWireframe = wireFrame;
    floor_mesh104->useRGBAColour = true;
    floor_mesh104->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    floor_mesh104->hasTexture = false;
    floor_mesh104->textures[0] = "";
    floor_mesh104->textureRatios[0] = 1.0f;
    meshArray.push_back(floor_mesh104);

    cMeshInfo* floor_mesh105 = new cMeshInfo();
    floor_mesh105->meshName = "floor";
    floor_mesh105->friendlyName = "floor105";
    floor_mesh105->isWireframe = wireFrame;
    floor_mesh105->useRGBAColour = true;
    floor_mesh105->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    floor_mesh105->hasTexture = false;
    floor_mesh105->textures[0] = "";
    floor_mesh105->textureRatios[0] = 1.0f;
    meshArray.push_back(floor_mesh105);

    cMeshInfo* floor_mesh106 = new cMeshInfo();
    floor_mesh106->meshName = "floor";
    floor_mesh106->friendlyName = "floor106";
    floor_mesh106->isWireframe = wireFrame;
    floor_mesh106->useRGBAColour = true;
    floor_mesh106->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    floor_mesh106->hasTexture = false;
    floor_mesh106->textures[0] = "";
    floor_mesh106->textureRatios[0] = 1.0f;
    meshArray.push_back(floor_mesh106);

    cMeshInfo* floor_mesh107 = new cMeshInfo();
    floor_mesh107->meshName = "floor";
    floor_mesh107->friendlyName = "floor107";
    floor_mesh107->isWireframe = wireFrame;
    floor_mesh107->useRGBAColour = true;
    floor_mesh107->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    floor_mesh107->hasTexture = false;
    floor_mesh107->textures[0] = "";
    floor_mesh107->textureRatios[0] = 1.0f;
    meshArray.push_back(floor_mesh107);

    cMeshInfo* floor_mesh108 = new cMeshInfo();
    floor_mesh108->meshName = "floor";
    floor_mesh108->friendlyName = "floor108";
    floor_mesh108->isWireframe = wireFrame;
    floor_mesh108->useRGBAColour = true;
    floor_mesh108->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    floor_mesh108->hasTexture = false;
    floor_mesh108->textures[0] = "";
    floor_mesh108->textureRatios[0] = 1.0f;
    meshArray.push_back(floor_mesh108);

    cMeshInfo* floor_mesh109 = new cMeshInfo();
    floor_mesh109->meshName = "floor";
    floor_mesh109->friendlyName = "floor109";
    floor_mesh109->isWireframe = wireFrame;
    floor_mesh109->useRGBAColour = true;
    floor_mesh109->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    floor_mesh109->hasTexture = false;
    floor_mesh109->textures[0] = "";
    floor_mesh109->textureRatios[0] = 1.0f;
    meshArray.push_back(floor_mesh109);

    cMeshInfo* floor_mesh110 = new cMeshInfo();
    floor_mesh110->meshName = "floor";
    floor_mesh110->friendlyName = "floor110";
    floor_mesh110->isWireframe = wireFrame;
    floor_mesh110->useRGBAColour = true;
    floor_mesh110->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    floor_mesh110->hasTexture = false;
    floor_mesh110->textures[0] = "";
    floor_mesh110->textureRatios[0] = 1.0f;
    meshArray.push_back(floor_mesh110);

    cMeshInfo* floor_mesh111 = new cMeshInfo();
    floor_mesh111->meshName = "floor";
    floor_mesh111->friendlyName = "floor111";
    floor_mesh111->isWireframe = wireFrame;
    floor_mesh111->useRGBAColour = true;
    floor_mesh111->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    floor_mesh111->hasTexture = false;
    floor_mesh111->textures[0] = "";
    floor_mesh111->textureRatios[0] = 1.0f;
    meshArray.push_back(floor_mesh111);

    cMeshInfo* floor_mesh112 = new cMeshInfo();
    floor_mesh112->meshName = "floor";
    floor_mesh112->friendlyName = "floor112";
    floor_mesh112->isWireframe = wireFrame;
    floor_mesh112->useRGBAColour = true;
    floor_mesh112->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    floor_mesh112->hasTexture = false;
    floor_mesh112->textures[0] = "";
    floor_mesh112->textureRatios[0] = 1.0f;
    meshArray.push_back(floor_mesh112);

    cMeshInfo* floor_mesh113 = new cMeshInfo();
    floor_mesh113->meshName = "floor";
    floor_mesh113->friendlyName = "floor113";
    floor_mesh113->isWireframe = wireFrame;
    floor_mesh113->useRGBAColour = true;
    floor_mesh113->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    floor_mesh113->hasTexture = false;
    floor_mesh113->textures[0] = "";
    floor_mesh113->textureRatios[0] = 1.0f;
    meshArray.push_back(floor_mesh113);

    cMeshInfo* floor_mesh114 = new cMeshInfo();
    floor_mesh114->meshName = "floor";
    floor_mesh114->friendlyName = "floor114";
    floor_mesh114->isWireframe = wireFrame;
    floor_mesh114->useRGBAColour = true;
    floor_mesh114->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    floor_mesh114->hasTexture = false;
    floor_mesh114->textures[0] = "";
    floor_mesh114->textureRatios[0] = 1.0f;
    meshArray.push_back(floor_mesh114);

    cMeshInfo* floor_mesh115 = new cMeshInfo();
    floor_mesh115->meshName = "floor";
    floor_mesh115->friendlyName = "floor115";
    floor_mesh115->isWireframe = wireFrame;
    floor_mesh115->useRGBAColour = true;
    floor_mesh115->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    floor_mesh115->hasTexture = false;
    floor_mesh115->textures[0] = "";
    floor_mesh115->textureRatios[0] = 1.0f;
    meshArray.push_back(floor_mesh115);

    cMeshInfo* floor_mesh116 = new cMeshInfo();
    floor_mesh116->meshName = "floor";
    floor_mesh116->friendlyName = "floor116";
    floor_mesh116->isWireframe = wireFrame;
    floor_mesh116->useRGBAColour = true;
    floor_mesh116->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    floor_mesh116->hasTexture = false;
    floor_mesh116->textures[0] = "";
    floor_mesh116->textureRatios[0] = 1.0f;
    meshArray.push_back(floor_mesh116);

    cMeshInfo* floor_mesh117 = new cMeshInfo();
    floor_mesh117->meshName = "floor";
    floor_mesh117->friendlyName = "floor117";
    floor_mesh117->isWireframe = wireFrame;
    floor_mesh117->useRGBAColour = true;
    floor_mesh117->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    floor_mesh117->hasTexture = false;
    floor_mesh117->textures[0] = "";
    floor_mesh117->textureRatios[0] = 1.0f;
    meshArray.push_back(floor_mesh117);

    cMeshInfo* floor_mesh118 = new cMeshInfo();
    floor_mesh118->meshName = "floor";
    floor_mesh118->friendlyName = "floor118";
    floor_mesh118->isWireframe = wireFrame;
    floor_mesh118->useRGBAColour = true;
    floor_mesh118->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    floor_mesh118->hasTexture = false;
    floor_mesh118->textures[0] = "";
    floor_mesh118->textureRatios[0] = 1.0f;
    meshArray.push_back(floor_mesh118);

    cMeshInfo* floor_mesh119 = new cMeshInfo();
    floor_mesh119->meshName = "floor";
    floor_mesh119->friendlyName = "floor119";
    floor_mesh119->isWireframe = wireFrame;
    floor_mesh119->useRGBAColour = true;
    floor_mesh119->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    floor_mesh119->hasTexture = false;
    floor_mesh119->textures[0] = "";
    floor_mesh119->textureRatios[0] = 1.0f;
    meshArray.push_back(floor_mesh119);

    cMeshInfo* floor_mesh120 = new cMeshInfo();
    floor_mesh120->meshName = "floor";
    floor_mesh120->friendlyName = "floor120";
    floor_mesh120->isWireframe = wireFrame;
    floor_mesh120->useRGBAColour = true;
    floor_mesh120->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    floor_mesh120->hasTexture = false;
    floor_mesh120->textures[0] = "";
    floor_mesh120->textureRatios[0] = 1.0f;
    meshArray.push_back(floor_mesh120);

    cMeshInfo* floor_mesh121 = new cMeshInfo();
    floor_mesh121->meshName = "floor";
    floor_mesh121->friendlyName = "floor121";
    floor_mesh121->isWireframe = wireFrame;
    floor_mesh121->useRGBAColour = true;
    floor_mesh121->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    floor_mesh121->hasTexture = false;
    floor_mesh121->textures[0] = "";
    floor_mesh121->textureRatios[0] = 1.0f;
    meshArray.push_back(floor_mesh121);

    sModelDrawInfo wall_obj;
    LoadModel(meshFiles[14], wall_obj);
    if (!VAOMan->LoadModelIntoVAO("wall", wall_obj, shaderID)) {
        std::cerr << "Could not load model into VAO" << std::endl;
    }
    cMeshInfo* wall_mesh = new cMeshInfo();
    wall_mesh->meshName = "wall";
    wall_mesh->friendlyName = "wall";
    wall_mesh->isWireframe = wireFrame;
    wall_mesh->useRGBAColour = true;
    wall_mesh->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    wall_mesh->hasTexture = false;
    wall_mesh->textures[0] = "";
    wall_mesh->textureRatios[0] = 1.0f;
    meshArray.push_back(wall_mesh);
    
    cMeshInfo* wall_mesh1 = new cMeshInfo();
    wall_mesh1->meshName = "wall";
    wall_mesh1->friendlyName = "wall1";
    wall_mesh1->isWireframe = wireFrame;
    wall_mesh1->useRGBAColour = true;
    wall_mesh1->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    wall_mesh1->hasTexture = false;
    wall_mesh1->textures[0] = "";
    wall_mesh1->textureRatios[0] = 1.0f;
    meshArray.push_back(wall_mesh1);
    
    cMeshInfo* wall_mesh2 = new cMeshInfo();
    wall_mesh2->meshName = "wall";
    wall_mesh2->friendlyName = "wall2";
    wall_mesh2->isWireframe = wireFrame;
    wall_mesh2->useRGBAColour = true;
    wall_mesh2->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    wall_mesh2->hasTexture = false;
    wall_mesh2->textures[0] = "";
    wall_mesh2->textureRatios[0] = 1.0f;
    meshArray.push_back(wall_mesh2);
    
    cMeshInfo* wall_mesh3 = new cMeshInfo();
    wall_mesh3->meshName = "wall";
    wall_mesh3->friendlyName = "wall3";
    wall_mesh3->isWireframe = wireFrame;
    wall_mesh3->useRGBAColour = true;
    wall_mesh3->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    wall_mesh3->hasTexture = false;
    wall_mesh3->textures[0] = "";
    wall_mesh3->textureRatios[0] = 1.0f;
    meshArray.push_back(wall_mesh3);
    
    cMeshInfo* wall_mesh4 = new cMeshInfo();
    wall_mesh4->meshName = "wall";
    wall_mesh4->friendlyName = "wall4";
    wall_mesh4->isWireframe = wireFrame;
    wall_mesh4->useRGBAColour = true;
    wall_mesh4->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    wall_mesh4->hasTexture = false;
    wall_mesh4->textures[0] = "";
    wall_mesh4->textureRatios[0] = 1.0f;
    meshArray.push_back(wall_mesh4);
    
    cMeshInfo* wall_mesh5 = new cMeshInfo();
    wall_mesh5->meshName = "wall";
    wall_mesh5->friendlyName = "wall5";
    wall_mesh5->isWireframe = wireFrame;
    wall_mesh5->useRGBAColour = true;
    wall_mesh5->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    wall_mesh5->hasTexture = false;
    wall_mesh5->textures[0] = "";
    wall_mesh5->textureRatios[0] = 1.0f;
    meshArray.push_back(wall_mesh5);
    
    cMeshInfo* wall_mesh6 = new cMeshInfo();
    wall_mesh6->meshName = "wall";
    wall_mesh6->friendlyName = "wall6";
    wall_mesh6->isWireframe = wireFrame;
    wall_mesh6->useRGBAColour = true;
    wall_mesh6->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    wall_mesh6->hasTexture = false;
    wall_mesh6->textures[0] = "";
    wall_mesh6->textureRatios[0] = 1.0f;
    meshArray.push_back(wall_mesh6);
    
    cMeshInfo* wall_mesh7 = new cMeshInfo();
    wall_mesh7->meshName = "wall";
    wall_mesh7->friendlyName = "wall7";
    wall_mesh7->isWireframe = wireFrame;
    wall_mesh7->useRGBAColour = true;
    wall_mesh7->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    wall_mesh7->hasTexture = false;
    wall_mesh7->textures[0] = "";
    wall_mesh7->textureRatios[0] = 1.0f;
    meshArray.push_back(wall_mesh7);

    cMeshInfo* wall_mesh8 = new cMeshInfo();
    wall_mesh8->meshName = "wall";
    wall_mesh8->friendlyName = "wall8";
    wall_mesh8->isWireframe = wireFrame;
    wall_mesh8->useRGBAColour = true;
    wall_mesh8->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    wall_mesh8->hasTexture = false;
    wall_mesh8->textures[0] = "";
    wall_mesh8->textureRatios[0] = 1.0f;
    meshArray.push_back(wall_mesh8);

    cMeshInfo* wall_mesh9 = new cMeshInfo();
    wall_mesh9->meshName = "wall";
    wall_mesh9->friendlyName = "wall9";
    wall_mesh9->isWireframe = wireFrame;
    wall_mesh9->useRGBAColour = true;
    wall_mesh9->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    wall_mesh9->hasTexture = false;
    wall_mesh9->textures[0] = "";
    wall_mesh9->textureRatios[0] = 1.0f;
    meshArray.push_back(wall_mesh9);

    cMeshInfo* wall_mesh10 = new cMeshInfo();
    wall_mesh10->meshName = "wall";
    wall_mesh10->friendlyName = "wall10";
    wall_mesh10->isWireframe = wireFrame;
    wall_mesh10->useRGBAColour = true;
    wall_mesh10->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    wall_mesh10->hasTexture = false;
    wall_mesh10->textures[0] = "";
    wall_mesh10->textureRatios[0] = 1.0f;
    meshArray.push_back(wall_mesh10);

    cMeshInfo* wall_mesh11 = new cMeshInfo();
    wall_mesh11->meshName = "wall";
    wall_mesh11->friendlyName = "wall11";
    wall_mesh11->isWireframe = wireFrame;
    wall_mesh11->useRGBAColour = true;
    wall_mesh11->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    wall_mesh11->hasTexture = false;
    wall_mesh11->textures[0] = "";
    wall_mesh11->textureRatios[0] = 1.0f;
    meshArray.push_back(wall_mesh11);

    cMeshInfo* wall_mesh12 = new cMeshInfo();
    wall_mesh12->meshName = "wall";
    wall_mesh12->friendlyName = "wall12";
    wall_mesh12->isWireframe = wireFrame;
    wall_mesh12->useRGBAColour = true;
    wall_mesh12->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    wall_mesh12->hasTexture = false;
    wall_mesh12->textures[0] = "";
    wall_mesh12->textureRatios[0] = 1.0f;
    meshArray.push_back(wall_mesh12);

    cMeshInfo* wall_mesh13 = new cMeshInfo();
    wall_mesh13->meshName = "wall";
    wall_mesh13->friendlyName = "wall13";
    wall_mesh13->isWireframe = wireFrame;
    wall_mesh13->useRGBAColour = true;
    wall_mesh13->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    wall_mesh13->hasTexture = false;
    wall_mesh13->textures[0] = "";
    wall_mesh13->textureRatios[0] = 1.0f;
    meshArray.push_back(wall_mesh13);

    cMeshInfo* wall_mesh14 = new cMeshInfo();
    wall_mesh14->meshName = "wall";
    wall_mesh14->friendlyName = "wall14";
    wall_mesh14->isWireframe = wireFrame;
    wall_mesh14->useRGBAColour = true;
    wall_mesh14->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    wall_mesh14->hasTexture = false;
    wall_mesh14->textures[0] = "";
    wall_mesh14->textureRatios[0] = 1.0f;
    meshArray.push_back(wall_mesh14);

    cMeshInfo* wall_mesh15 = new cMeshInfo();
    wall_mesh15->meshName = "wall";
    wall_mesh15->friendlyName = "wall15";
    wall_mesh15->isWireframe = wireFrame;
    wall_mesh15->useRGBAColour = true;
    wall_mesh15->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    wall_mesh15->hasTexture = false;
    wall_mesh15->textures[0] = "";
    wall_mesh15->textureRatios[0] = 1.0f;
    meshArray.push_back(wall_mesh15);

    cMeshInfo* wall_mesh16 = new cMeshInfo();
    wall_mesh16->meshName = "wall";
    wall_mesh16->friendlyName = "wall16";
    wall_mesh16->isWireframe = wireFrame;
    wall_mesh16->useRGBAColour = true;
    wall_mesh16->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    wall_mesh16->hasTexture = false;
    wall_mesh16->textures[0] = "";
    wall_mesh16->textureRatios[0] = 1.0f;
    meshArray.push_back(wall_mesh16);

    cMeshInfo* wall_mesh17 = new cMeshInfo();
    wall_mesh17->meshName = "wall";
    wall_mesh17->friendlyName = "wall17";
    wall_mesh17->isWireframe = wireFrame;
    wall_mesh17->useRGBAColour = true;
    wall_mesh17->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    wall_mesh17->hasTexture = false;
    wall_mesh17->textures[0] = "";
    wall_mesh17->textureRatios[0] = 1.0f;
    meshArray.push_back(wall_mesh17);

    cMeshInfo* wall_mesh18 = new cMeshInfo();
    wall_mesh18->meshName = "wall";
    wall_mesh18->friendlyName = "wall18";
    wall_mesh18->isWireframe = wireFrame;
    wall_mesh18->useRGBAColour = true;
    wall_mesh18->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    wall_mesh18->hasTexture = false;
    wall_mesh18->textures[0] = "";
    wall_mesh18->textureRatios[0] = 1.0f;
    meshArray.push_back(wall_mesh18);
    
    cMeshInfo* wall_mesh19 = new cMeshInfo();
    wall_mesh19->meshName = "wall";
    wall_mesh19->friendlyName = "wall19";
    wall_mesh19->isWireframe = wireFrame;
    wall_mesh19->useRGBAColour = true;
    wall_mesh19->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    wall_mesh19->hasTexture = false;
    wall_mesh19->textures[0] = "";
    wall_mesh19->textureRatios[0] = 1.0f;
    meshArray.push_back(wall_mesh19);
    
    cMeshInfo* wall_mesh20 = new cMeshInfo();
    wall_mesh20->meshName = "wall";
    wall_mesh20->friendlyName = "wall20";
    wall_mesh20->isWireframe = wireFrame;
    wall_mesh20->useRGBAColour = true;
    wall_mesh20->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    wall_mesh20->hasTexture = false;
    wall_mesh20->textures[0] = "";
    wall_mesh20->textureRatios[0] = 1.0f;
    meshArray.push_back(wall_mesh20);
    
    cMeshInfo* wall_mesh21 = new cMeshInfo();
    wall_mesh21->meshName = "wall";
    wall_mesh21->friendlyName = "wall21";
    wall_mesh21->isWireframe = wireFrame;
    wall_mesh21->useRGBAColour = true;
    wall_mesh21->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    wall_mesh21->hasTexture = false;
    wall_mesh21->textures[0] = "";
    wall_mesh21->textureRatios[0] = 1.0f;
    meshArray.push_back(wall_mesh21);
    
    cMeshInfo* wall_mesh22 = new cMeshInfo();
    wall_mesh22->meshName = "wall";
    wall_mesh22->friendlyName = "wall22";
    wall_mesh22->isWireframe = wireFrame;
    wall_mesh22->useRGBAColour = true;
    wall_mesh22->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    wall_mesh22->hasTexture = false;
    wall_mesh22->textures[0] = "";
    wall_mesh22->textureRatios[0] = 1.0f;
    meshArray.push_back(wall_mesh22);
    
    cMeshInfo* wall_mesh23 = new cMeshInfo();
    wall_mesh23->meshName = "wall";
    wall_mesh23->friendlyName = "wall23";
    wall_mesh23->isWireframe = wireFrame;
    wall_mesh23->useRGBAColour = true;
    wall_mesh23->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    wall_mesh23->hasTexture = false;
    wall_mesh23->textures[0] = "";
    wall_mesh23->textureRatios[0] = 1.0f;
    meshArray.push_back(wall_mesh23);
    
    cMeshInfo* wall_mesh24 = new cMeshInfo();
    wall_mesh24->meshName = "wall";
    wall_mesh24->friendlyName = "wall24";
    wall_mesh24->isWireframe = wireFrame;
    wall_mesh24->useRGBAColour = true;
    wall_mesh24->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    wall_mesh24->hasTexture = false;
    wall_mesh24->textures[0] = "";
    wall_mesh24->textureRatios[0] = 1.0f;
    meshArray.push_back(wall_mesh24);
    
    cMeshInfo* wall_mesh25 = new cMeshInfo();
    wall_mesh25->meshName = "wall";
    wall_mesh25->friendlyName = "wall25";
    wall_mesh25->isWireframe = wireFrame;
    wall_mesh25->useRGBAColour = true;
    wall_mesh25->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    wall_mesh25->hasTexture = false;
    wall_mesh25->textures[0] = "";
    wall_mesh25->textureRatios[0] = 1.0f;
    meshArray.push_back(wall_mesh25);
    
    cMeshInfo* wall_mesh26 = new cMeshInfo();
    wall_mesh26->meshName = "wall";
    wall_mesh26->friendlyName = "wall26";
    wall_mesh26->isWireframe = wireFrame;
    wall_mesh26->useRGBAColour = true;
    wall_mesh26->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    wall_mesh26->hasTexture = false;
    wall_mesh26->textures[0] = "";
    wall_mesh26->textureRatios[0] = 1.0f;
    meshArray.push_back(wall_mesh26);

    cMeshInfo* wall_mesh27 = new cMeshInfo();
    wall_mesh27->meshName = "wall";
    wall_mesh27->friendlyName = "wall27";
    wall_mesh27->isWireframe = wireFrame;
    wall_mesh27->useRGBAColour = true;
    wall_mesh27->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    wall_mesh27->hasTexture = false;
    wall_mesh27->textures[0] = "";
    wall_mesh27->textureRatios[0] = 1.0f;
    meshArray.push_back(wall_mesh27);
    
    cMeshInfo* wall_mesh28 = new cMeshInfo();
    wall_mesh28->meshName = "wall";
    wall_mesh28->friendlyName = "wall28";
    wall_mesh28->isWireframe = wireFrame;
    wall_mesh28->useRGBAColour = true;
    wall_mesh28->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    wall_mesh28->hasTexture = false;
    wall_mesh28->textures[0] = "";
    wall_mesh28->textureRatios[0] = 1.0f;
    meshArray.push_back(wall_mesh28);

    cMeshInfo* wall_mesh29 = new cMeshInfo();
    wall_mesh29->meshName = "wall";
    wall_mesh29->friendlyName = "wall29";
    wall_mesh29->isWireframe = wireFrame;
    wall_mesh29->useRGBAColour = true;
    wall_mesh29->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    wall_mesh29->hasTexture = false;
    wall_mesh29->textures[0] = "";
    wall_mesh29->textureRatios[0] = 1.0f;
    meshArray.push_back(wall_mesh29);

    cMeshInfo* wall_mesh30 = new cMeshInfo();
    wall_mesh30->meshName = "wall";
    wall_mesh30->friendlyName = "wall30";
    wall_mesh30->isWireframe = wireFrame;
    wall_mesh30->useRGBAColour = true;
    wall_mesh30->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    wall_mesh30->hasTexture = false;
    wall_mesh30->textures[0] = "";
    wall_mesh30->textureRatios[0] = 1.0f;
    meshArray.push_back(wall_mesh30);
    
    cMeshInfo* wall_mesh31 = new cMeshInfo();
    wall_mesh31->meshName = "wall";
    wall_mesh31->friendlyName = "wall31";
    wall_mesh31->isWireframe = wireFrame;
    wall_mesh31->useRGBAColour = true;
    wall_mesh31->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    wall_mesh31->hasTexture = false;
    wall_mesh31->textures[0] = "";
    wall_mesh31->textureRatios[0] = 1.0f;
    meshArray.push_back(wall_mesh31);

    cMeshInfo* wall_mesh32 = new cMeshInfo();
    wall_mesh32->meshName = "wall";
    wall_mesh32->friendlyName = "wall32";
    wall_mesh32->isWireframe = wireFrame;
    wall_mesh32->useRGBAColour = true;
    wall_mesh32->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    wall_mesh32->hasTexture = false;
    wall_mesh32->textures[0] = "";
    wall_mesh32->textureRatios[0] = 1.0f;
    meshArray.push_back(wall_mesh32);

    cMeshInfo* wall_mesh33 = new cMeshInfo();
    wall_mesh33->meshName = "wall";
    wall_mesh33->friendlyName = "wall33";
    wall_mesh33->isWireframe = wireFrame;
    wall_mesh33->useRGBAColour = true;
    wall_mesh33->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    wall_mesh33->hasTexture = false;
    wall_mesh33->textures[0] = "";
    wall_mesh33->textureRatios[0] = 1.0f;
    meshArray.push_back(wall_mesh33);
    
    cMeshInfo* wall_mesh34 = new cMeshInfo();
    wall_mesh34->meshName = "wall";
    wall_mesh34->friendlyName = "wall34";
    wall_mesh34->isWireframe = wireFrame;
    wall_mesh34->useRGBAColour = true;
    wall_mesh34->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    wall_mesh34->hasTexture = false;
    wall_mesh34->textures[0] = "";
    wall_mesh34->textureRatios[0] = 1.0f;
    meshArray.push_back(wall_mesh34);
    
    cMeshInfo* wall_mesh35 = new cMeshInfo();
    wall_mesh35->meshName = "wall";
    wall_mesh35->friendlyName = "wall35";
    wall_mesh35->isWireframe = wireFrame;
    wall_mesh35->useRGBAColour = true;
    wall_mesh35->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    wall_mesh35->hasTexture = false;
    wall_mesh35->textures[0] = "";
    wall_mesh35->textureRatios[0] = 1.0f;
    meshArray.push_back(wall_mesh35);
    
    cMeshInfo* wall_mesh36 = new cMeshInfo();
    wall_mesh36->meshName = "wall";
    wall_mesh36->friendlyName = "wall36";
    wall_mesh36->isWireframe = wireFrame;
    wall_mesh36->useRGBAColour = true;
    wall_mesh36->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    wall_mesh36->hasTexture = false;
    wall_mesh36->textures[0] = "";
    wall_mesh36->textureRatios[0] = 1.0f;
    meshArray.push_back(wall_mesh36);
    
    cMeshInfo* wall_mesh37 = new cMeshInfo();
    wall_mesh37->meshName = "wall";
    wall_mesh37->friendlyName = "wall37";
    wall_mesh37->isWireframe = wireFrame;
    wall_mesh37->useRGBAColour = true;
    wall_mesh37->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    wall_mesh37->hasTexture = false;
    wall_mesh37->textures[0] = "";
    wall_mesh37->textureRatios[0] = 1.0f;
    meshArray.push_back(wall_mesh37);

    cMeshInfo* wall_mesh38 = new cMeshInfo();
    wall_mesh38->meshName = "wall";
    wall_mesh38->friendlyName = "wall38";
    wall_mesh38->isWireframe = wireFrame;
    wall_mesh38->useRGBAColour = true;
    wall_mesh38->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    wall_mesh38->hasTexture = false;
    wall_mesh38->textures[0] = "";
    wall_mesh38->textureRatios[0] = 1.0f;
    meshArray.push_back(wall_mesh38);

    cMeshInfo* wall_mesh39 = new cMeshInfo();
    wall_mesh39->meshName = "wall";
    wall_mesh39->friendlyName = "wall39";
    wall_mesh39->isWireframe = wireFrame;
    wall_mesh39->useRGBAColour = true;
    wall_mesh39->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    wall_mesh39->hasTexture = false;
    wall_mesh39->textures[0] = "";
    wall_mesh39->textureRatios[0] = 1.0f;
    meshArray.push_back(wall_mesh39);

    cMeshInfo* wall_mesh40 = new cMeshInfo();
    wall_mesh40->meshName = "wall";
    wall_mesh40->friendlyName = "wall40";
    wall_mesh40->isWireframe = wireFrame;
    wall_mesh40->useRGBAColour = true;
    wall_mesh40->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    wall_mesh40->hasTexture = false;
    wall_mesh40->textures[0] = "";
    wall_mesh40->textureRatios[0] = 1.0f;
    meshArray.push_back(wall_mesh40);

    cMeshInfo* wall_mesh41 = new cMeshInfo();
    wall_mesh41->meshName = "wall";
    wall_mesh41->friendlyName = "wall41";
    wall_mesh41->isWireframe = wireFrame;
    wall_mesh41->useRGBAColour = true;
    wall_mesh41->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    wall_mesh41->hasTexture = false;
    wall_mesh41->textures[0] = "";
    wall_mesh41->textureRatios[0] = 1.0f;
    meshArray.push_back(wall_mesh41);

    cMeshInfo* wall_mesh42 = new cMeshInfo();
    wall_mesh42->meshName = "wall";
    wall_mesh42->friendlyName = "wall42";
    wall_mesh42->isWireframe = wireFrame;
    wall_mesh42->useRGBAColour = true;
    wall_mesh42->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    wall_mesh42->hasTexture = false;
    wall_mesh42->textures[0] = "";
    wall_mesh42->textureRatios[0] = 1.0f;
    meshArray.push_back(wall_mesh42);
    
    cMeshInfo* wall_mesh43 = new cMeshInfo();
    wall_mesh43->meshName = "wall";
    wall_mesh43->friendlyName = "wall43";
    wall_mesh43->isWireframe = wireFrame;
    wall_mesh43->useRGBAColour = true;
    wall_mesh43->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    wall_mesh43->hasTexture = false;
    wall_mesh43->textures[0] = "";
    wall_mesh43->textureRatios[0] = 1.0f;
    meshArray.push_back(wall_mesh43);

    cMeshInfo* wall_mesh44 = new cMeshInfo();
    wall_mesh44->meshName = "wall";
    wall_mesh44->friendlyName = "wall44";
    wall_mesh44->isWireframe = wireFrame;
    wall_mesh44->useRGBAColour = true;
    wall_mesh44->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    wall_mesh44->hasTexture = false;
    wall_mesh44->textures[0] = "";
    wall_mesh44->textureRatios[0] = 1.0f;
    meshArray.push_back(wall_mesh44);

    cMeshInfo* wall_mesh45 = new cMeshInfo();
    wall_mesh45->meshName = "wall";
    wall_mesh45->friendlyName = "wall45";
    wall_mesh45->isWireframe = wireFrame;
    wall_mesh45->useRGBAColour = true;
    wall_mesh45->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    wall_mesh45->hasTexture = false;
    wall_mesh45->textures[0] = "";
    wall_mesh45->textureRatios[0] = 1.0f;
    meshArray.push_back(wall_mesh45);

    cMeshInfo* wall_mesh46 = new cMeshInfo();
    wall_mesh46->meshName = "wall";
    wall_mesh46->friendlyName = "wall46";
    wall_mesh46->isWireframe = wireFrame;
    wall_mesh46->useRGBAColour = true;
    wall_mesh46->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    wall_mesh46->hasTexture = false;
    wall_mesh46->textures[0] = "";
    wall_mesh46->textureRatios[0] = 1.0f;
    meshArray.push_back(wall_mesh46);

    cMeshInfo* wall_mesh47 = new cMeshInfo();
    wall_mesh47->meshName = "wall";
    wall_mesh47->friendlyName = "wall47";
    wall_mesh47->isWireframe = wireFrame;
    wall_mesh47->useRGBAColour = true;
    wall_mesh47->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    wall_mesh47->hasTexture = false;
    wall_mesh47->textures[0] = "";
    wall_mesh47->textureRatios[0] = 1.0f;
    meshArray.push_back(wall_mesh47);

    cMeshInfo* wall_mesh48 = new cMeshInfo();
    wall_mesh48->meshName = "wall";
    wall_mesh48->friendlyName = "wall48";
    wall_mesh48->isWireframe = wireFrame;
    wall_mesh48->useRGBAColour = true;
    wall_mesh48->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    wall_mesh48->hasTexture = false;
    wall_mesh48->textures[0] = "";
    wall_mesh48->textureRatios[0] = 1.0f;
    meshArray.push_back(wall_mesh48);

    cMeshInfo* wall_mesh49 = new cMeshInfo();
    wall_mesh49->meshName = "wall";
    wall_mesh49->friendlyName = "wall49";
    wall_mesh49->isWireframe = wireFrame;
    wall_mesh49->useRGBAColour = true;
    wall_mesh49->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    wall_mesh49->hasTexture = false;
    wall_mesh49->textures[0] = "";
    wall_mesh49->textureRatios[0] = 1.0f;
    meshArray.push_back(wall_mesh49);

    cMeshInfo* wall_mesh50 = new cMeshInfo();
    wall_mesh50->meshName = "wall";
    wall_mesh50->friendlyName = "wall50";
    wall_mesh50->isWireframe = wireFrame;
    wall_mesh50->useRGBAColour = true;
    wall_mesh50->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    wall_mesh50->hasTexture = false;
    wall_mesh50->textures[0] = "";
    wall_mesh50->textureRatios[0] = 1.0f;
    meshArray.push_back(wall_mesh50);
    
    cMeshInfo* wall_mesh51 = new cMeshInfo();
    wall_mesh51->meshName = "wall";
    wall_mesh51->friendlyName = "wall51";
    wall_mesh51->isWireframe = wireFrame;
    wall_mesh51->useRGBAColour = true;
    wall_mesh51->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    wall_mesh51->hasTexture = false;
    wall_mesh51->textures[0] = "";
    wall_mesh51->textureRatios[0] = 1.0f;
    meshArray.push_back(wall_mesh51);

    cMeshInfo* wall_mesh52 = new cMeshInfo();
    wall_mesh52->meshName = "wall";
    wall_mesh52->friendlyName = "wall52";
    wall_mesh52->isWireframe = wireFrame;
    wall_mesh52->useRGBAColour = true;
    wall_mesh52->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    wall_mesh52->hasTexture = false;
    wall_mesh52->textures[0] = "";
    wall_mesh52->textureRatios[0] = 1.0f;
    meshArray.push_back(wall_mesh52);

    cMeshInfo* wall_mesh53 = new cMeshInfo();
    wall_mesh53->meshName = "wall";
    wall_mesh53->friendlyName = "wall53";
    wall_mesh53->isWireframe = wireFrame;
    wall_mesh53->useRGBAColour = true;
    wall_mesh53->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    wall_mesh53->hasTexture = false;
    wall_mesh53->textures[0] = "";
    wall_mesh53->textureRatios[0] = 1.0f;
    meshArray.push_back(wall_mesh53);

    cMeshInfo* wall_mesh54 = new cMeshInfo();
    wall_mesh54->meshName = "wall";
    wall_mesh54->friendlyName = "wall54";
    wall_mesh54->isWireframe = wireFrame;
    wall_mesh54->useRGBAColour = true;
    wall_mesh54->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    wall_mesh54->hasTexture = false;
    wall_mesh54->textures[0] = "";
    wall_mesh54->textureRatios[0] = 1.0f;
    meshArray.push_back(wall_mesh54);
    
    cMeshInfo* wall_mesh55 = new cMeshInfo();
    wall_mesh55->meshName = "wall";
    wall_mesh55->friendlyName = "wall55";
    wall_mesh55->isWireframe = wireFrame;
    wall_mesh55->useRGBAColour = true;
    wall_mesh55->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    wall_mesh55->hasTexture = false;
    wall_mesh55->textures[0] = "";
    wall_mesh55->textureRatios[0] = 1.0f;
    meshArray.push_back(wall_mesh55);

    cMeshInfo* wall_mesh56 = new cMeshInfo();
    wall_mesh56->meshName = "wall";
    wall_mesh56->friendlyName = "wall56";
    wall_mesh56->isWireframe = wireFrame;
    wall_mesh56->useRGBAColour = true;
    wall_mesh56->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    wall_mesh56->hasTexture = false;
    wall_mesh56->textures[0] = "";
    wall_mesh56->textureRatios[0] = 1.0f;
    meshArray.push_back(wall_mesh56);

    cMeshInfo* wall_mesh57 = new cMeshInfo();
    wall_mesh57->meshName = "wall";
    wall_mesh57->friendlyName = "wall57";
    wall_mesh57->isWireframe = wireFrame;
    wall_mesh57->useRGBAColour = true;
    wall_mesh57->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    wall_mesh57->hasTexture = false;
    wall_mesh57->textures[0] = "";
    wall_mesh57->textureRatios[0] = 1.0f;
    meshArray.push_back(wall_mesh57);

    cMeshInfo* wall_mesh58 = new cMeshInfo();
    wall_mesh58->meshName = "wall";
    wall_mesh58->friendlyName = "wall58";
    wall_mesh58->isWireframe = wireFrame;
    wall_mesh58->useRGBAColour = true;
    wall_mesh58->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    wall_mesh58->hasTexture = false;
    wall_mesh58->textures[0] = "";
    wall_mesh58->textureRatios[0] = 1.0f;
    meshArray.push_back(wall_mesh58);

    cMeshInfo* wall_mesh59 = new cMeshInfo();
    wall_mesh59->meshName = "wall";
    wall_mesh59->friendlyName = "wall59";
    wall_mesh59->isWireframe = wireFrame;
    wall_mesh59->useRGBAColour = true;
    wall_mesh59->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    wall_mesh59->hasTexture = false;
    wall_mesh59->textures[0] = "";
    wall_mesh59->textureRatios[0] = 1.0f;
    meshArray.push_back(wall_mesh59);

    cMeshInfo* wall_mesh60 = new cMeshInfo();
    wall_mesh60->meshName = "wall";
    wall_mesh60->friendlyName = "wall60";
    wall_mesh60->isWireframe = wireFrame;
    wall_mesh60->useRGBAColour = true;
    wall_mesh60->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    wall_mesh60->hasTexture = false;
    wall_mesh60->textures[0] = "";
    wall_mesh60->textureRatios[0] = 1.0f;
    meshArray.push_back(wall_mesh60);

    cMeshInfo* wall_mesh61 = new cMeshInfo();
    wall_mesh61->meshName = "wall";
    wall_mesh61->friendlyName = "wall61";
    wall_mesh61->isWireframe = wireFrame;
    wall_mesh61->useRGBAColour = true;
    wall_mesh61->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    wall_mesh61->hasTexture = false;
    wall_mesh61->textures[0] = "";
    wall_mesh61->textureRatios[0] = 1.0f;
    meshArray.push_back(wall_mesh61);

    cMeshInfo* wall_mesh62 = new cMeshInfo();
    wall_mesh62->meshName = "wall";
    wall_mesh62->friendlyName = "wall62";
    wall_mesh62->isWireframe = wireFrame;
    wall_mesh62->useRGBAColour = true;
    wall_mesh62->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    wall_mesh62->hasTexture = false;
    wall_mesh62->textures[0] = "";
    wall_mesh62->textureRatios[0] = 1.0f;
    meshArray.push_back(wall_mesh62);

    cMeshInfo* wall_mesh63 = new cMeshInfo();
    wall_mesh63->meshName = "wall";
    wall_mesh63->friendlyName = "wall63";
    wall_mesh63->isWireframe = wireFrame;
    wall_mesh63->useRGBAColour = true;
    wall_mesh63->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    wall_mesh63->hasTexture = false;
    wall_mesh63->textures[0] = "";
    wall_mesh63->textureRatios[0] = 1.0f;
    meshArray.push_back(wall_mesh63);

    cMeshInfo* wall_mesh64 = new cMeshInfo();
    wall_mesh64->meshName = "wall";
    wall_mesh64->friendlyName = "wall64";
    wall_mesh64->isWireframe = wireFrame;
    wall_mesh64->useRGBAColour = true;
    wall_mesh64->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    wall_mesh64->hasTexture = false;
    wall_mesh64->textures[0] = "";
    wall_mesh64->textureRatios[0] = 1.0f;
    meshArray.push_back(wall_mesh64);

    cMeshInfo* wall_mesh65 = new cMeshInfo();
    wall_mesh65->meshName = "wall";
    wall_mesh65->friendlyName = "wall65";
    wall_mesh65->isWireframe = wireFrame;
    wall_mesh65->useRGBAColour = true;
    wall_mesh65->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    wall_mesh65->hasTexture = false;
    wall_mesh65->textures[0] = "";
    wall_mesh65->textureRatios[0] = 1.0f;
    meshArray.push_back(wall_mesh65);
    
    cMeshInfo* wall_mesh66 = new cMeshInfo();
    wall_mesh66->meshName = "wall";
    wall_mesh66->friendlyName = "wall66";
    wall_mesh66->isWireframe = wireFrame;
    wall_mesh66->useRGBAColour = true;
    wall_mesh66->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    wall_mesh66->hasTexture = false;
    wall_mesh66->textures[0] = "";
    wall_mesh66->textureRatios[0] = 1.0f;
    meshArray.push_back(wall_mesh66);

    cMeshInfo* wall_mesh67 = new cMeshInfo();
    wall_mesh67->meshName = "wall";
    wall_mesh67->friendlyName = "wall67";
    wall_mesh67->isWireframe = wireFrame;
    wall_mesh67->useRGBAColour = true;
    wall_mesh67->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    wall_mesh67->hasTexture = false;
    wall_mesh67->textures[0] = "";
    wall_mesh67->textureRatios[0] = 1.0f;
    meshArray.push_back(wall_mesh67);

    cMeshInfo* wall_mesh68 = new cMeshInfo();
    wall_mesh68->meshName = "wall";
    wall_mesh68->friendlyName = "wall68";
    wall_mesh68->isWireframe = wireFrame;
    wall_mesh68->useRGBAColour = true;
    wall_mesh68->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    wall_mesh68->hasTexture = false;
    wall_mesh68->textures[0] = "";
    wall_mesh68->textureRatios[0] = 1.0f;
    meshArray.push_back(wall_mesh68);

    cMeshInfo* wall_mesh69 = new cMeshInfo();
    wall_mesh69->meshName = "wall";
    wall_mesh69->friendlyName = "wall69";
    wall_mesh69->isWireframe = wireFrame;
    wall_mesh69->useRGBAColour = true;
    wall_mesh69->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    wall_mesh69->hasTexture = false;
    wall_mesh69->textures[0] = "";
    wall_mesh69->textureRatios[0] = 1.0f;
    meshArray.push_back(wall_mesh69);

    cMeshInfo* wall_mesh70 = new cMeshInfo();
    wall_mesh70->meshName = "wall";
    wall_mesh70->friendlyName = "wall70";
    wall_mesh70->isWireframe = wireFrame;
    wall_mesh70->useRGBAColour = true;
    wall_mesh70->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    wall_mesh70->hasTexture = false;
    wall_mesh70->textures[0] = "";
    wall_mesh70->textureRatios[0] = 1.0f;
    meshArray.push_back(wall_mesh70);

    cMeshInfo* wall_mesh71 = new cMeshInfo();
    wall_mesh71->meshName = "wall";
    wall_mesh71->friendlyName = "wall71";
    wall_mesh71->isWireframe = wireFrame;
    wall_mesh71->useRGBAColour = true;
    wall_mesh71->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    wall_mesh71->hasTexture = false;
    wall_mesh71->textures[0] = "";
    wall_mesh71->textureRatios[0] = 1.0f;
    meshArray.push_back(wall_mesh71);

    cMeshInfo* wall_mesh72 = new cMeshInfo();
    wall_mesh72->meshName = "wall";
    wall_mesh72->friendlyName = "wall72";
    wall_mesh72->isWireframe = wireFrame;
    wall_mesh72->useRGBAColour = true;
    wall_mesh72->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    wall_mesh72->hasTexture = false;
    wall_mesh72->textures[0] = "";
    wall_mesh72->textureRatios[0] = 1.0f;
    meshArray.push_back(wall_mesh72);
    
    cMeshInfo* wall_mesh73 = new cMeshInfo();
    wall_mesh73->meshName = "wall";
    wall_mesh73->friendlyName = "wall73";
    wall_mesh73->isWireframe = wireFrame;
    wall_mesh73->useRGBAColour = true;
    wall_mesh73->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    wall_mesh73->hasTexture = false;
    wall_mesh73->textures[0] = "";
    wall_mesh73->textureRatios[0] = 1.0f;
    meshArray.push_back(wall_mesh73);

    cMeshInfo* wall_mesh74 = new cMeshInfo();
    wall_mesh74->meshName = "wall";
    wall_mesh74->friendlyName = "wall74";
    wall_mesh74->isWireframe = wireFrame;
    wall_mesh74->useRGBAColour = true;
    wall_mesh74->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    wall_mesh74->hasTexture = false;
    wall_mesh74->textures[0] = "";
    wall_mesh74->textureRatios[0] = 1.0f;
    meshArray.push_back(wall_mesh74);

    cMeshInfo* wall_mesh75 = new cMeshInfo();
    wall_mesh75->meshName = "wall";
    wall_mesh75->friendlyName = "wall75";
    wall_mesh75->isWireframe = wireFrame;
    wall_mesh75->useRGBAColour = true;
    wall_mesh75->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    wall_mesh75->hasTexture = false;
    wall_mesh75->textures[0] = "";
    wall_mesh75->textureRatios[0] = 1.0f;
    meshArray.push_back(wall_mesh75);

    cMeshInfo* wall_mesh76 = new cMeshInfo();
    wall_mesh76->meshName = "wall";
    wall_mesh76->friendlyName = "wall76";
    wall_mesh76->isWireframe = wireFrame;
    wall_mesh76->useRGBAColour = true;
    wall_mesh76->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    wall_mesh76->hasTexture = false;
    wall_mesh76->textures[0] = "";
    wall_mesh76->textureRatios[0] = 1.0f;
    meshArray.push_back(wall_mesh76);

    cMeshInfo* wall_mesh77 = new cMeshInfo();
    wall_mesh77->meshName = "wall";
    wall_mesh77->friendlyName = "wall77";
    wall_mesh77->isWireframe = wireFrame;
    wall_mesh77->useRGBAColour = true;
    wall_mesh77->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    wall_mesh77->hasTexture = false;
    wall_mesh77->textures[0] = "";
    wall_mesh77->textureRatios[0] = 1.0f;
    meshArray.push_back(wall_mesh77);

    cMeshInfo* wall_mesh78 = new cMeshInfo();
    wall_mesh78->meshName = "wall";
    wall_mesh78->friendlyName = "wall78";
    wall_mesh78->isWireframe = wireFrame;
    wall_mesh78->useRGBAColour = true;
    wall_mesh78->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    wall_mesh78->hasTexture = false;
    wall_mesh78->textures[0] = "";
    wall_mesh78->textureRatios[0] = 1.0f;
    meshArray.push_back(wall_mesh78);

    cMeshInfo* wall_mesh79 = new cMeshInfo();
    wall_mesh79->meshName = "wall";
    wall_mesh79->friendlyName = "wall79";
    wall_mesh79->isWireframe = wireFrame;
    wall_mesh79->useRGBAColour = true;
    wall_mesh79->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    wall_mesh79->hasTexture = false;
    wall_mesh79->textures[0] = "";
    wall_mesh79->textureRatios[0] = 1.0f;
    meshArray.push_back(wall_mesh79);

    cMeshInfo* wall_mesh80 = new cMeshInfo();
    wall_mesh80->meshName = "wall";
    wall_mesh80->friendlyName = "wall80";
    wall_mesh80->isWireframe = wireFrame;
    wall_mesh80->useRGBAColour = true;
    wall_mesh80->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    wall_mesh80->hasTexture = false;
    wall_mesh80->textures[0] = "";
    wall_mesh80->textureRatios[0] = 1.0f;
    meshArray.push_back(wall_mesh80);

    cMeshInfo* wall_mesh81 = new cMeshInfo();
    wall_mesh81->meshName = "wall";
    wall_mesh81->friendlyName = "wall81";
    wall_mesh81->isWireframe = wireFrame;
    wall_mesh81->useRGBAColour = true;
    wall_mesh81->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    wall_mesh81->hasTexture = false;
    wall_mesh81->textures[0] = "";
    wall_mesh81->textureRatios[0] = 1.0f;
    meshArray.push_back(wall_mesh81);

    cMeshInfo* wall_mesh82 = new cMeshInfo();
    wall_mesh82->meshName = "wall";
    wall_mesh82->friendlyName = "wall82";
    wall_mesh82->isWireframe = wireFrame;
    wall_mesh82->useRGBAColour = true;
    wall_mesh82->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    wall_mesh82->hasTexture = false;
    wall_mesh82->textures[0] = "";
    wall_mesh82->textureRatios[0] = 1.0f;
    meshArray.push_back(wall_mesh82);

    cMeshInfo* wall_mesh83 = new cMeshInfo();
    wall_mesh83->meshName = "wall";
    wall_mesh83->friendlyName = "wall83";
    wall_mesh83->isWireframe = wireFrame;
    wall_mesh83->useRGBAColour = true;
    wall_mesh83->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    wall_mesh83->hasTexture = false;
    wall_mesh83->textures[0] = "";
    wall_mesh83->textureRatios[0] = 1.0f;
    meshArray.push_back(wall_mesh83);

    cMeshInfo* wall_mesh84 = new cMeshInfo();
    wall_mesh84->meshName = "wall";
    wall_mesh84->friendlyName = "wall84";
    wall_mesh84->isWireframe = wireFrame;
    wall_mesh84->useRGBAColour = true;
    wall_mesh84->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    wall_mesh84->hasTexture = false;
    wall_mesh84->textures[0] = "";
    wall_mesh84->textureRatios[0] = 1.0f;
    meshArray.push_back(wall_mesh84);

    cMeshInfo* wall_mesh85 = new cMeshInfo();
    wall_mesh85->meshName = "wall";
    wall_mesh85->friendlyName = "wall85";
    wall_mesh85->isWireframe = wireFrame;
    wall_mesh85->useRGBAColour = true;
    wall_mesh85->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    wall_mesh85->hasTexture = false;
    wall_mesh85->textures[0] = "";
    wall_mesh85->textureRatios[0] = 1.0f;
    meshArray.push_back(wall_mesh85);

    cMeshInfo* wall_mesh86 = new cMeshInfo();
    wall_mesh86->meshName = "wall";
    wall_mesh86->friendlyName = "wall86";
    wall_mesh86->isWireframe = wireFrame;
    wall_mesh86->useRGBAColour = true;
    wall_mesh86->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    wall_mesh86->hasTexture = false;
    wall_mesh86->textures[0] = "";
    wall_mesh86->textureRatios[0] = 1.0f;
    meshArray.push_back(wall_mesh86);
    
    cMeshInfo* wall_mesh87 = new cMeshInfo();
    wall_mesh87->meshName = "wall";
    wall_mesh87->friendlyName = "wall87";
    wall_mesh87->isWireframe = wireFrame;
    wall_mesh87->useRGBAColour = true;
    wall_mesh87->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    wall_mesh87->hasTexture = false;
    wall_mesh87->textures[0] = "";
    wall_mesh87->textureRatios[0] = 1.0f;
    meshArray.push_back(wall_mesh87);

    cMeshInfo* wall_mesh88 = new cMeshInfo();
    wall_mesh88->meshName = "wall";
    wall_mesh88->friendlyName = "wall88";
    wall_mesh88->isWireframe = wireFrame;
    wall_mesh88->useRGBAColour = true;
    wall_mesh88->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    wall_mesh88->hasTexture = false;
    wall_mesh88->textures[0] = "";
    wall_mesh88->textureRatios[0] = 1.0f;
    meshArray.push_back(wall_mesh88);

    cMeshInfo* wall_mesh89 = new cMeshInfo();
    wall_mesh89->meshName = "wall";
    wall_mesh89->friendlyName = "wall89";
    wall_mesh89->isWireframe = wireFrame;
    wall_mesh89->useRGBAColour = true;
    wall_mesh89->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    wall_mesh89->hasTexture = false;
    wall_mesh89->textures[0] = "";
    wall_mesh89->textureRatios[0] = 1.0f;
    meshArray.push_back(wall_mesh89);

    cMeshInfo* wall_mesh90 = new cMeshInfo();
    wall_mesh90->meshName = "wall";
    wall_mesh90->friendlyName = "wall90";
    wall_mesh90->isWireframe = wireFrame;
    wall_mesh90->useRGBAColour = true;
    wall_mesh90->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    wall_mesh90->hasTexture = false;
    wall_mesh90->textures[0] = "";
    wall_mesh90->textureRatios[0] = 1.0f;
    meshArray.push_back(wall_mesh90);

    cMeshInfo* wall_mesh91 = new cMeshInfo();
    wall_mesh91->meshName = "wall";
    wall_mesh91->friendlyName = "wall91";
    wall_mesh91->isWireframe = wireFrame;
    wall_mesh91->useRGBAColour = true;
    wall_mesh91->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    wall_mesh91->hasTexture = false;
    wall_mesh91->textures[0] = "";
    wall_mesh91->textureRatios[0] = 1.0f;
    meshArray.push_back(wall_mesh91);

    cMeshInfo* wall_mesh92 = new cMeshInfo();
    wall_mesh92->meshName = "wall";
    wall_mesh92->friendlyName = "wall92";
    wall_mesh92->isWireframe = wireFrame;
    wall_mesh92->useRGBAColour = true;
    wall_mesh92->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    wall_mesh92->hasTexture = false;
    wall_mesh92->textures[0] = "";
    wall_mesh92->textureRatios[0] = 1.0f;
    meshArray.push_back(wall_mesh92);

    cMeshInfo* wall_mesh93 = new cMeshInfo();
    wall_mesh93->meshName = "wall";
    wall_mesh93->friendlyName = "wall93";
    wall_mesh93->isWireframe = wireFrame;
    wall_mesh93->useRGBAColour = true;
    wall_mesh93->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    wall_mesh93->hasTexture = false;
    wall_mesh93->textures[0] = "";
    wall_mesh93->textureRatios[0] = 1.0f;
    meshArray.push_back(wall_mesh93);

    cMeshInfo* wall_mesh94 = new cMeshInfo();
    wall_mesh94->meshName = "wall";
    wall_mesh94->friendlyName = "wall94";
    wall_mesh94->isWireframe = wireFrame;
    wall_mesh94->useRGBAColour = true;
    wall_mesh94->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    wall_mesh94->hasTexture = false;
    wall_mesh94->textures[0] = "";
    wall_mesh94->textureRatios[0] = 1.0f;
    meshArray.push_back(wall_mesh94);

    cMeshInfo* wall_mesh95 = new cMeshInfo();
    wall_mesh95->meshName = "wall";
    wall_mesh95->friendlyName = "wall95";
    wall_mesh95->isWireframe = wireFrame;
    wall_mesh95->useRGBAColour = true;
    wall_mesh95->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    wall_mesh95->hasTexture = false;
    wall_mesh95->textures[0] = "";
    wall_mesh95->textureRatios[0] = 1.0f;
    meshArray.push_back(wall_mesh95);

    cMeshInfo* wall_mesh96 = new cMeshInfo();
    wall_mesh96->meshName = "wall";
    wall_mesh96->friendlyName = "wall96";
    wall_mesh96->isWireframe = wireFrame;
    wall_mesh96->useRGBAColour = true;
    wall_mesh96->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    wall_mesh96->hasTexture = false;
    wall_mesh96->textures[0] = "";
    wall_mesh96->textureRatios[0] = 1.0f;
    meshArray.push_back(wall_mesh96);

    cMeshInfo* wall_mesh97 = new cMeshInfo();
    wall_mesh97->meshName = "wall";
    wall_mesh97->friendlyName = "wall97";
    wall_mesh97->isWireframe = wireFrame;
    wall_mesh97->useRGBAColour = true;
    wall_mesh97->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    wall_mesh97->hasTexture = false;
    wall_mesh97->textures[0] = "";
    wall_mesh97->textureRatios[0] = 1.0f;
    meshArray.push_back(wall_mesh97);

    cMeshInfo* wall_mesh98 = new cMeshInfo();
    wall_mesh98->meshName = "wall";
    wall_mesh98->friendlyName = "wall98";
    wall_mesh98->isWireframe = wireFrame;
    wall_mesh98->useRGBAColour = true;
    wall_mesh98->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    wall_mesh98->hasTexture = false;
    wall_mesh98->textures[0] = "";
    wall_mesh98->textureRatios[0] = 1.0f;
    meshArray.push_back(wall_mesh98);

    cMeshInfo* wall_mesh99 = new cMeshInfo();
    wall_mesh99->meshName = "wall";
    wall_mesh99->friendlyName = "wall99";
    wall_mesh99->isWireframe = wireFrame;
    wall_mesh99->useRGBAColour = true;
    wall_mesh99->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    wall_mesh99->hasTexture = false;
    wall_mesh99->textures[0] = "";
    wall_mesh99->textureRatios[0] = 1.0f;
    meshArray.push_back(wall_mesh99);

    cMeshInfo* wall_mesh100 = new cMeshInfo();
    wall_mesh100->meshName = "wall";
    wall_mesh100->friendlyName = "wall100";
    wall_mesh100->isWireframe = wireFrame;
    wall_mesh100->useRGBAColour = true;
    wall_mesh100->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    wall_mesh100->hasTexture = false;
    wall_mesh100->textures[0] = "";
    wall_mesh100->textureRatios[0] = 1.0f;
    meshArray.push_back(wall_mesh100);

    cMeshInfo* wall_mesh101 = new cMeshInfo();
    wall_mesh101->meshName = "wall";
    wall_mesh101->friendlyName = "wall101";
    wall_mesh101->isWireframe = wireFrame;
    wall_mesh101->useRGBAColour = true;
    wall_mesh101->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    wall_mesh101->hasTexture = false;
    wall_mesh101->textures[0] = "";
    wall_mesh101->textureRatios[0] = 1.0f;
    meshArray.push_back(wall_mesh101);

    cMeshInfo* wall_mesh102 = new cMeshInfo();
    wall_mesh102->meshName = "wall";
    wall_mesh102->friendlyName = "wall102";
    wall_mesh102->isWireframe = wireFrame;
    wall_mesh102->useRGBAColour = true;
    wall_mesh102->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    wall_mesh102->hasTexture = false;
    wall_mesh102->textures[0] = "";
    wall_mesh102->textureRatios[0] = 1.0f;
    meshArray.push_back(wall_mesh102);

    cMeshInfo* wall_mesh103 = new cMeshInfo();
    wall_mesh103->meshName = "wall";
    wall_mesh103->friendlyName = "wall103";
    wall_mesh103->isWireframe = wireFrame;
    wall_mesh103->useRGBAColour = true;
    wall_mesh103->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    wall_mesh103->hasTexture = false;
    wall_mesh103->textures[0] = "";
    wall_mesh103->textureRatios[0] = 1.0f;
    meshArray.push_back(wall_mesh103);

    cMeshInfo* wall_mesh104 = new cMeshInfo();
    wall_mesh104->meshName = "wall";
    wall_mesh104->friendlyName = "wall104";
    wall_mesh104->isWireframe = wireFrame;
    wall_mesh104->useRGBAColour = true;
    wall_mesh104->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    wall_mesh104->hasTexture = false;
    wall_mesh104->textures[0] = "";
    wall_mesh104->textureRatios[0] = 1.0f;
    meshArray.push_back(wall_mesh104);

    cMeshInfo* wall_mesh105 = new cMeshInfo();
    wall_mesh105->meshName = "wall";
    wall_mesh105->friendlyName = "wall105";
    wall_mesh105->isWireframe = wireFrame;
    wall_mesh105->useRGBAColour = true;
    wall_mesh105->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    wall_mesh105->hasTexture = false;
    wall_mesh105->textures[0] = "";
    wall_mesh105->textureRatios[0] = 1.0f;
    meshArray.push_back(wall_mesh105);

    cMeshInfo* wall_mesh106 = new cMeshInfo();
    wall_mesh106->meshName = "wall";
    wall_mesh106->friendlyName = "wall106";
    wall_mesh106->isWireframe = wireFrame;
    wall_mesh106->useRGBAColour = true;
    wall_mesh106->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    wall_mesh106->hasTexture = false;
    wall_mesh106->textures[0] = "";
    wall_mesh106->textureRatios[0] = 1.0f;
    meshArray.push_back(wall_mesh106);

    cMeshInfo* wall_mesh107 = new cMeshInfo();
    wall_mesh107->meshName = "wall";
    wall_mesh107->friendlyName = "wall107";
    wall_mesh107->isWireframe = wireFrame;
    wall_mesh107->useRGBAColour = true;
    wall_mesh107->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    wall_mesh107->hasTexture = false;
    wall_mesh107->textures[0] = "";
    wall_mesh107->textureRatios[0] = 1.0f;
    meshArray.push_back(wall_mesh107);

    cMeshInfo* wall_mesh108 = new cMeshInfo();
    wall_mesh108->meshName = "wall";
    wall_mesh108->friendlyName = "wall108";
    wall_mesh108->isWireframe = wireFrame;
    wall_mesh108->useRGBAColour = true;
    wall_mesh108->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    wall_mesh108->hasTexture = false;
    wall_mesh108->textures[0] = "";
    wall_mesh108->textureRatios[0] = 1.0f;
    meshArray.push_back(wall_mesh108);

    cMeshInfo* wall_mesh109 = new cMeshInfo();
    wall_mesh109->meshName = "wall";
    wall_mesh109->friendlyName = "wall109";
    wall_mesh109->isWireframe = wireFrame;
    wall_mesh109->useRGBAColour = true;
    wall_mesh109->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    wall_mesh109->hasTexture = false;
    wall_mesh109->textures[0] = "";
    wall_mesh109->textureRatios[0] = 1.0f;
    meshArray.push_back(wall_mesh109);

    cMeshInfo* wall_mesh110 = new cMeshInfo();
    wall_mesh110->meshName = "wall";
    wall_mesh110->friendlyName = "wall110";
    wall_mesh110->isWireframe = wireFrame;
    wall_mesh110->useRGBAColour = true;
    wall_mesh110->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    wall_mesh110->hasTexture = false;
    wall_mesh110->textures[0] = "";
    wall_mesh110->textureRatios[0] = 1.0f;
    meshArray.push_back(wall_mesh110);

    cMeshInfo* wall_mesh111 = new cMeshInfo();
    wall_mesh111->meshName = "wall";
    wall_mesh111->friendlyName = "wall111";
    wall_mesh111->isWireframe = wireFrame;
    wall_mesh111->useRGBAColour = true;
    wall_mesh111->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    wall_mesh111->hasTexture = false;
    wall_mesh111->textures[0] = "";
    wall_mesh111->textureRatios[0] = 1.0f;
    meshArray.push_back(wall_mesh111);

    cMeshInfo* wall_mesh112 = new cMeshInfo();
    wall_mesh112->meshName = "wall";
    wall_mesh112->friendlyName = "wall112";
    wall_mesh112->isWireframe = wireFrame;
    wall_mesh112->useRGBAColour = true;
    wall_mesh112->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    wall_mesh112->hasTexture = false;
    wall_mesh112->textures[0] = "";
    wall_mesh112->textureRatios[0] = 1.0f;
    meshArray.push_back(wall_mesh112);

    sModelDrawInfo arched_doorway;
    LoadModel(meshFiles[15], arched_doorway);
    if (!VAOMan->LoadModelIntoVAO("arched_doorway", arched_doorway, shaderID)) {
        std::cerr << "Could not load model into VAO" << std::endl;
    }
    cMeshInfo* arched_doorway_mesh1 = new cMeshInfo();
    arched_doorway_mesh1->meshName = "arched_doorway";
    arched_doorway_mesh1->friendlyName = "arched_doorway1";
    arched_doorway_mesh1->isWireframe = wireFrame;
    arched_doorway_mesh1->useRGBAColour = true;
    arched_doorway_mesh1->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    arched_doorway_mesh1->hasTexture = false;
    arched_doorway_mesh1->textures[0] = "";
    arched_doorway_mesh1->textureRatios[0] = 1.0f;
    meshArray.push_back(arched_doorway_mesh1);
    
    cMeshInfo* arched_doorway_mesh2 = new cMeshInfo();
    arched_doorway_mesh2->meshName = "arched_doorway";
    arched_doorway_mesh2->friendlyName = "arched_doorway2";
    arched_doorway_mesh2->isWireframe = wireFrame;
    arched_doorway_mesh2->useRGBAColour = true;
    arched_doorway_mesh2->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    arched_doorway_mesh2->hasTexture = false;
    arched_doorway_mesh2->textures[0] = "";
    arched_doorway_mesh2->textureRatios[0] = 1.0f;
    meshArray.push_back(arched_doorway_mesh2);

    sModelDrawInfo torch1_obj;
    LoadModel(meshFiles[16], torch1_obj);
    if (!VAOMan->LoadModelIntoVAO("torch1", torch1_obj, shaderID)) {
        std::cerr << "Could not load model into VAO" << std::endl;
    }
    cMeshInfo* torch1_mesh = new cMeshInfo();
    torch1_mesh->meshName = "torch1";
    torch1_mesh->friendlyName = "torch1-0";
    torch1_mesh->isWireframe = wireFrame;
    torch1_mesh->useRGBAColour = true;
    torch1_mesh->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    torch1_mesh->hasTexture = false;
    torch1_mesh->textures[0] = "";
    torch1_mesh->textureRatios[0] = 1.0f;
    meshArray.push_back(torch1_mesh);
    
    cMeshInfo* torch1_mesh1 = new cMeshInfo();
    torch1_mesh1->meshName = "torch1";
    torch1_mesh1->friendlyName = "torch1-1";
    torch1_mesh1->isWireframe = wireFrame;
    torch1_mesh1->useRGBAColour = true;
    torch1_mesh1->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    torch1_mesh1->hasTexture = false;
    torch1_mesh1->textures[0] = "";
    torch1_mesh1->textureRatios[0] = 1.0f;
    meshArray.push_back(torch1_mesh1);
    
    cMeshInfo* torch1_mesh2 = new cMeshInfo();
    torch1_mesh2->meshName = "torch1";
    torch1_mesh2->friendlyName = "torch1-2";
    torch1_mesh2->isWireframe = wireFrame;
    torch1_mesh2->useRGBAColour = true;
    torch1_mesh2->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    torch1_mesh2->hasTexture = false;
    torch1_mesh2->textures[0] = "";
    torch1_mesh2->textureRatios[0] = 1.0f;
    meshArray.push_back(torch1_mesh2);
    
    cMeshInfo* torch1_mesh3 = new cMeshInfo();
    torch1_mesh3->meshName = "torch1";
    torch1_mesh3->friendlyName = "torch1-3";
    torch1_mesh3->isWireframe = wireFrame;
    torch1_mesh3->useRGBAColour = true;
    torch1_mesh3->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    torch1_mesh3->hasTexture = false;
    torch1_mesh3->textures[0] = "";
    torch1_mesh3->textureRatios[0] = 1.0f;
    meshArray.push_back(torch1_mesh3);
    
    cMeshInfo* torch1_mesh4 = new cMeshInfo();
    torch1_mesh4->meshName = "torch1";
    torch1_mesh4->friendlyName = "torch1-4";
    torch1_mesh4->isWireframe = wireFrame;
    torch1_mesh4->useRGBAColour = true;
    torch1_mesh4->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    torch1_mesh4->hasTexture = false;
    torch1_mesh4->textures[0] = "";
    torch1_mesh4->textureRatios[0] = 1.0f;
    meshArray.push_back(torch1_mesh4);

    cMeshInfo* torch1_mesh5 = new cMeshInfo();
    torch1_mesh5->meshName = "torch1";
    torch1_mesh5->friendlyName = "torch1-5";
    torch1_mesh5->isWireframe = wireFrame;
    torch1_mesh5->useRGBAColour = true;
    torch1_mesh5->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    torch1_mesh5->hasTexture = false;
    torch1_mesh5->textures[0] = "";
    torch1_mesh5->textureRatios[0] = 1.0f;
    meshArray.push_back(torch1_mesh5);

    cMeshInfo* torch1_mesh6 = new cMeshInfo();
    torch1_mesh6->meshName = "torch1";
    torch1_mesh6->friendlyName = "torch1-6";
    torch1_mesh6->isWireframe = wireFrame;
    torch1_mesh6->useRGBAColour = true;
    torch1_mesh6->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    torch1_mesh6->hasTexture = false;
    torch1_mesh6->textures[0] = "";
    torch1_mesh6->textureRatios[0] = 1.0f;
    meshArray.push_back(torch1_mesh6);

    cMeshInfo* torch1_mesh7 = new cMeshInfo();
    torch1_mesh7->meshName = "torch1";
    torch1_mesh7->friendlyName = "torch1-7";
    torch1_mesh7->isWireframe = wireFrame;
    torch1_mesh7->useRGBAColour = true;
    torch1_mesh7->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    torch1_mesh7->hasTexture = false;
    torch1_mesh7->textures[0] = "";
    torch1_mesh7->textureRatios[0] = 1.0f;
    meshArray.push_back(torch1_mesh7);

    sModelDrawInfo torch2_obj;
    LoadModel(meshFiles[17], torch2_obj);
    if (!VAOMan->LoadModelIntoVAO("torch2", torch2_obj, shaderID)) {
        std::cerr << "Could not load model into VAO" << std::endl;
    }
    cMeshInfo* torch2_mesh = new cMeshInfo();
    torch2_mesh->meshName = "torch2";
    torch2_mesh->friendlyName = "torch2-0";
    torch2_mesh->isWireframe = wireFrame;
    torch2_mesh->useRGBAColour = true;
    torch2_mesh->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    torch2_mesh->hasTexture = false;
    torch2_mesh->textures[0] = "";
    torch2_mesh->textureRatios[0] = 1.0f;
    meshArray.push_back(torch2_mesh);

    cMeshInfo* torch2_mesh1 = new cMeshInfo();
    torch2_mesh1->meshName = "torch2";
    torch2_mesh1->friendlyName = "torch2-1";
    torch2_mesh1->isWireframe = wireFrame;
    torch2_mesh1->useRGBAColour = true;
    torch2_mesh1->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    torch2_mesh1->hasTexture = false;
    torch2_mesh1->textures[0] = "";
    torch2_mesh1->textureRatios[0] = 1.0f;
    meshArray.push_back(torch2_mesh1);

    cMeshInfo* torch2_mesh2 = new cMeshInfo();
    torch2_mesh2->meshName = "torch2";
    torch2_mesh2->friendlyName = "torch2-2";
    torch2_mesh2->isWireframe = wireFrame;
    torch2_mesh2->useRGBAColour = true;
    torch2_mesh2->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    torch2_mesh2->hasTexture = false;
    torch2_mesh2->textures[0] = "";
    torch2_mesh2->textureRatios[0] = 1.0f;
    meshArray.push_back(torch2_mesh2);

    cMeshInfo* torch2_mesh3 = new cMeshInfo();
    torch2_mesh3->meshName = "torch2";
    torch2_mesh3->friendlyName = "torch2-3";
    torch2_mesh3->isWireframe = wireFrame;
    torch2_mesh3->useRGBAColour = true;
    torch2_mesh3->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    torch2_mesh3->hasTexture = false;
    torch2_mesh3->textures[0] = "";
    torch2_mesh3->textureRatios[0] = 1.0f;
    meshArray.push_back(torch2_mesh3);

    cMeshInfo* torch2_mesh4 = new cMeshInfo();
    torch2_mesh4->meshName = "torch2";
    torch2_mesh4->friendlyName = "torch2-4";
    torch2_mesh4->isWireframe = wireFrame;
    torch2_mesh4->useRGBAColour = true;
    torch2_mesh4->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    torch2_mesh4->hasTexture = false;
    torch2_mesh4->textures[0] = "";
    torch2_mesh4->textureRatios[0] = 1.0f;
    meshArray.push_back(torch2_mesh4);

    cMeshInfo* torch2_mesh5 = new cMeshInfo();
    torch2_mesh5->meshName = "torch2";
    torch2_mesh5->friendlyName = "torch2-5";
    torch2_mesh5->isWireframe = wireFrame;
    torch2_mesh5->useRGBAColour = true;
    torch2_mesh5->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    torch2_mesh5->hasTexture = false;
    torch2_mesh5->textures[0] = "";
    torch2_mesh5->textureRatios[0] = 1.0f;
    meshArray.push_back(torch2_mesh5);

    cMeshInfo* torch2_mesh6 = new cMeshInfo();
    torch2_mesh6->meshName = "torch2";
    torch2_mesh6->friendlyName = "torch2-6";
    torch2_mesh6->isWireframe = wireFrame;
    torch2_mesh6->useRGBAColour = true;
    torch2_mesh6->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    torch2_mesh6->hasTexture = false;
    torch2_mesh6->textures[0] = "";
    torch2_mesh6->textureRatios[0] = 1.0f;
    meshArray.push_back(torch2_mesh6);

    cMeshInfo* torch2_mesh7 = new cMeshInfo();
    torch2_mesh7->meshName = "torch2";
    torch2_mesh7->friendlyName = "torch2-7";
    torch2_mesh7->isWireframe = wireFrame;
    torch2_mesh7->useRGBAColour = true;
    torch2_mesh7->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    torch2_mesh7->hasTexture = false;
    torch2_mesh7->textures[0] = "";
    torch2_mesh7->textureRatios[0] = 1.0f;
    meshArray.push_back(torch2_mesh7);

    cMeshInfo* torch2_mesh8 = new cMeshInfo();
    torch2_mesh8->meshName = "torch2";
    torch2_mesh8->friendlyName = "torch2-8";
    torch2_mesh8->isWireframe = wireFrame;
    torch2_mesh8->useRGBAColour = true;
    torch2_mesh8->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    torch2_mesh8->hasTexture = false;
    torch2_mesh8->textures[0] = "";
    torch2_mesh8->textureRatios[0] = 1.0f;
    meshArray.push_back(torch2_mesh8);

    cMeshInfo* torch2_mesh9 = new cMeshInfo();
    torch2_mesh9->meshName = "torch2";
    torch2_mesh9->friendlyName = "torch2-9";
    torch2_mesh9->isWireframe = wireFrame;
    torch2_mesh9->useRGBAColour = true;
    torch2_mesh9->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    torch2_mesh9->hasTexture = false;
    torch2_mesh9->textures[0] = "";
    torch2_mesh9->textureRatios[0] = 1.0f;
    meshArray.push_back(torch2_mesh9);

    cMeshInfo* torch2_mesh10 = new cMeshInfo();
    torch2_mesh10->meshName = "torch2";
    torch2_mesh10->friendlyName = "torch2-10";
    torch2_mesh10->isWireframe = wireFrame;
    torch2_mesh10->useRGBAColour = true;
    torch2_mesh10->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    torch2_mesh10->hasTexture = false;
    torch2_mesh10->textures[0] = "";
    torch2_mesh10->textureRatios[0] = 1.0f;
    meshArray.push_back(torch2_mesh10);

    cMeshInfo* torch2_mesh11 = new cMeshInfo();
    torch2_mesh11->meshName = "torch2";
    torch2_mesh11->friendlyName = "torch2-11";
    torch2_mesh11->isWireframe = wireFrame;
    torch2_mesh11->useRGBAColour = true;
    torch2_mesh11->RGBAColour = glm::vec4(50.f, 50.f, 50.f, 1.f);
    torch2_mesh11->hasTexture = false;
    torch2_mesh11->textures[0] = "";
    torch2_mesh11->textureRatios[0] = 1.0f;
    meshArray.push_back(torch2_mesh11);

    cMeshInfo* bulb_mesh13 = new cMeshInfo();
    bulb_mesh13->meshName = "bulb";
    bulb_mesh13->friendlyName = "bulb13";
    bulb_mesh13->isWireframe = wireFrame;
    meshArray.push_back(bulb_mesh13);

    cMeshInfo* bulb_mesh14 = new cMeshInfo();
    bulb_mesh14->meshName = "bulb";
    bulb_mesh14->friendlyName = "bulb14";
    bulb_mesh14->isWireframe = wireFrame;
    meshArray.push_back(bulb_mesh14);

    cMeshInfo* bulb_mesh15 = new cMeshInfo();
    bulb_mesh15->meshName = "bulb";
    bulb_mesh15->friendlyName = "bulb15";
    bulb_mesh15->isWireframe = wireFrame;
    meshArray.push_back(bulb_mesh15);

    cMeshInfo* bulb_mesh16 = new cMeshInfo();
    bulb_mesh16->meshName = "bulb";
    bulb_mesh16->friendlyName = "bulb16";
    bulb_mesh16->isWireframe = wireFrame;
    meshArray.push_back(bulb_mesh16);

    cMeshInfo* bulb_mesh17 = new cMeshInfo();
    bulb_mesh17->meshName = "bulb";
    bulb_mesh17->friendlyName = "bulb17";
    bulb_mesh17->isWireframe = wireFrame;
    meshArray.push_back(bulb_mesh17);

    cMeshInfo* bulb_mesh18 = new cMeshInfo();
    bulb_mesh18->meshName = "bulb";
    bulb_mesh18->friendlyName = "bulb18";
    bulb_mesh18->isWireframe = wireFrame;
    meshArray.push_back(bulb_mesh18);

    cMeshInfo* bulb_mesh19 = new cMeshInfo();
    bulb_mesh19->meshName = "bulb";
    bulb_mesh19->friendlyName = "bulb19";
    bulb_mesh19->isWireframe = wireFrame;
    meshArray.push_back(bulb_mesh19);

    cMeshInfo* bulb_mesh20 = new cMeshInfo();
    bulb_mesh20->meshName = "bulb";
    bulb_mesh20->friendlyName = "bulb20";
    bulb_mesh20->isWireframe = wireFrame;
    meshArray.push_back(bulb_mesh20);
}

// All lights managed here
void ManageLights() {
    
    GLint PositionLocation = glGetUniformLocation(shaderID, "sLightsArray[0].position");
    GLint DiffuseLocation = glGetUniformLocation(shaderID, "sLightsArray[0].diffuse");
    GLint SpecularLocation = glGetUniformLocation(shaderID, "sLightsArray[0].specular");
    GLint AttenLocation = glGetUniformLocation(shaderID, "sLightsArray[0].atten");
    GLint DirectionLocation = glGetUniformLocation(shaderID, "sLightsArray[0].direction");
    GLint Param1Location = glGetUniformLocation(shaderID, "sLightsArray[0].param1");
    GLint Param2Location = glGetUniformLocation(shaderID, "sLightsArray[0].param2");

    //glm::vec3 lightPosition0 = meshArray[1]->position;
    glm::vec3 lightPosition0 = meshArray[1]->position;
    glUniform4f(PositionLocation, lightPosition0.x, lightPosition0.y, lightPosition0.z, 1.0f);
    //glUniform4f(PositionLocation, 0.f, 0.f, 0.f, 1.0f);
    glUniform4f(DiffuseLocation, 1.f, 1.f, 1.f, 1.f);
    glUniform4f(SpecularLocation, 1.f, 1.f, 1.f, 1.f);
    glUniform4f(AttenLocation, 0.1f, 0.5f, 0.0f, 1.f);
    glUniform4f(DirectionLocation, 1.f, 1.f, 1.f, 1.f);
    glUniform4f(Param1Location, 0.f, 0.f, 0.f, 1.f); //x = Light Type
    glUniform4f(Param2Location, l, 0.f, 0.f, 1.f); //x = Light on/off

    GLint PositionLocation1 = glGetUniformLocation(shaderID, "sLightsArray[1].position");
    GLint DiffuseLocation1 = glGetUniformLocation(shaderID, "sLightsArray[1].diffuse");
    GLint SpecularLocation1 = glGetUniformLocation(shaderID, "sLightsArray[1].specular");
    GLint AttenLocation1 = glGetUniformLocation(shaderID, "sLightsArray[1].atten");
    GLint DirectionLocation1 = glGetUniformLocation(shaderID, "sLightsArray[1].direction");
    GLint Param1Location1 = glGetUniformLocation(shaderID, "sLightsArray[1].param1");
    GLint Param2Location1 = glGetUniformLocation(shaderID, "sLightsArray[1].param2");

    glm::vec3 lightPosition1 = meshArray[2]->position;
    glUniform4f(PositionLocation1, lightPosition1.x, lightPosition1.y, lightPosition1.z, 1.0f);
    //glUniform4f(PositionLocation, 0.f, 0.f, 0.f, 1.0f);
    glUniform4f(DiffuseLocation1, 70.f, 50.f, 1.f, 1.f);
    glUniform4f(SpecularLocation1, 1.f, 1.f, 1.f, 1.f);
    glUniform4f(AttenLocation1, x, y, z, 1.f);
    glUniform4f(DirectionLocation1, 1.f, 1.f, 1.f, 1.f);
    glUniform4f(Param1Location1, 0.f, 0.f, 0.f, 1.f); //x = Light Type
    glUniform4f(Param2Location1, 1.f, 0.f, 0.f, 1.f); //x = Light on/off
    
    GLint PositionLocation2 = glGetUniformLocation(shaderID, "sLightsArray[2].position");
    GLint DiffuseLocation2 = glGetUniformLocation(shaderID, "sLightsArray[2].diffuse");
    GLint SpecularLocation2 = glGetUniformLocation(shaderID, "sLightsArray[2].specular");
    GLint AttenLocation2 = glGetUniformLocation(shaderID, "sLightsArray[2].atten");
    GLint DirectionLocation2 = glGetUniformLocation(shaderID, "sLightsArray[2].direction");
    GLint Param1Location2 = glGetUniformLocation(shaderID, "sLightsArray[2].param1");
    GLint Param2Location2 = glGetUniformLocation(shaderID, "sLightsArray[2].param2");

    glm::vec3 lightPosition2 = meshArray[3]->position;
    glUniform4f(PositionLocation2, lightPosition2.x, lightPosition2.y, lightPosition2.z, 1.0f);
    //glUniform4f(PositionLocation2, -23.f, 75.f, 58.f, 1.0f);
    glUniform4f(DiffuseLocation2, 70.f, 50.f, 1.f, 1.f);
    glUniform4f(SpecularLocation2, 1.f, 1.f, 1.f, 1.f);
    glUniform4f(AttenLocation2, x, y, z, 1.f);
    glUniform4f(DirectionLocation2, 1.f, 1.f, 1.f, 1.f);
    glUniform4f(Param1Location2, 0.f, 0.f, 0.f, 1.f); //x = Light Type
    glUniform4f(Param2Location2, 1.f, 0.f, 0.f, 1.f); //x = Light on/off
    
    GLint PositionLocation3 = glGetUniformLocation(shaderID, "sLightsArray[3].position");
    GLint DiffuseLocation3 = glGetUniformLocation(shaderID, "sLightsArray[3].diffuse");
    GLint SpecularLocation3 = glGetUniformLocation(shaderID, "sLightsArray[3].specular");
    GLint AttenLocation3 = glGetUniformLocation(shaderID, "sLightsArray[3].atten");
    GLint DirectionLocation3 = glGetUniformLocation(shaderID, "sLightsArray[3].direction");
    GLint Param1Location3 = glGetUniformLocation(shaderID, "sLightsArray[3].param1");
    GLint Param2Location3 = glGetUniformLocation(shaderID, "sLightsArray[3].param2");

    glm::vec3 lightPosition3 = meshArray[4]->position;
    glUniform4f(PositionLocation3, lightPosition3.x, lightPosition3.y, lightPosition3.z, 1.0f);
    //glUniform4f(PositionLocation, 0.f, 0.f, 0.f, 1.0f);
    glUniform4f(DiffuseLocation3, 70.f, 50.f, 1.f, 1.f);
    glUniform4f(SpecularLocation3, 1.f, 1.f, 1.f, 1.f);
    glUniform4f(AttenLocation3, x, y, z, 1.f);
    glUniform4f(DirectionLocation3, 1.f, 1.f, 1.f, 1.f);
    glUniform4f(Param1Location3, 0.f, 0.f, 0.f, 1.f); //x = Light Type
    glUniform4f(Param2Location3, 1.f, 0.f, 0.f, 1.f); //x = Light on/off
    
    GLint PositionLocation4 = glGetUniformLocation(shaderID, "sLightsArray[4].position");
    GLint DiffuseLocation4 = glGetUniformLocation(shaderID, "sLightsArray[4].diffuse");
    GLint SpecularLocation4 = glGetUniformLocation(shaderID, "sLightsArray[4].specular");
    GLint AttenLocation4 = glGetUniformLocation(shaderID, "sLightsArray[4].atten");
    GLint DirectionLocation4 = glGetUniformLocation(shaderID, "sLightsArray[4].direction");
    GLint Param1Location4 = glGetUniformLocation(shaderID, "sLightsArray[4].param1");
    GLint Param2Location4 = glGetUniformLocation(shaderID, "sLightsArray[4].param2");

    glm::vec3 lightPosition4 = meshArray[5]->position;
    glUniform4f(PositionLocation4, lightPosition4.x, lightPosition4.y, lightPosition4.z, 1.0f);
    //glUniform4f(PositionLocation, 0.f, 0.f, 0.f, 1.0f);
    glUniform4f(DiffuseLocation4, 70.f, 50.f, 1.f, 1.f);
    glUniform4f(SpecularLocation4, 1.f, 1.f, 1.f, 1.f);
    glUniform4f(AttenLocation4, x, y, z, 1.f);
    glUniform4f(DirectionLocation4, 1.f, 1.f, 1.f, 1.f);
    glUniform4f(Param1Location4, 0.f, 0.f, 0.f, 1.f); //x = Light Type
    glUniform4f(Param2Location4, 1.f, 0.f, 0.f, 1.f); //x = Light on/off
    
    GLint PositionLocation5 = glGetUniformLocation(shaderID, "sLightsArray[5].position");
    GLint DiffuseLocation5 = glGetUniformLocation(shaderID, "sLightsArray[5].diffuse");
    GLint SpecularLocation5 = glGetUniformLocation(shaderID, "sLightsArray[5].specular");
    GLint AttenLocation5 = glGetUniformLocation(shaderID, "sLightsArray[5].atten");
    GLint DirectionLocation5 = glGetUniformLocation(shaderID, "sLightsArray[5].direction");
    GLint Param1Location5 = glGetUniformLocation(shaderID, "sLightsArray[5].param1");
    GLint Param2Location5 = glGetUniformLocation(shaderID, "sLightsArray[5].param2");

    glm::vec3 lightPosition5 = meshArray[6]->position;
    glUniform4f(PositionLocation5, lightPosition5.x, lightPosition5.y, lightPosition5.z, 1.0f);
    //glUniform4f(PositionLocation, 0.f, 0.f, 0.f, 1.0f);
    glUniform4f(DiffuseLocation5, 70.f, 50.f, 1.f, 1.f);
    glUniform4f(SpecularLocation5, 1.f, 1.f, 1.f, 1.f);
    glUniform4f(AttenLocation5, x, y, z, 1.f);
    glUniform4f(DirectionLocation5, 1.f, 1.f, 1.f, 1.f);
    glUniform4f(Param1Location5, 0.f, 0.f, 0.f, 1.f); //x = Light Type
    glUniform4f(Param2Location5, 1.f, 0.f, 0.f, 1.f); //x = Light on/off
    
    GLint PositionLocation6 = glGetUniformLocation(shaderID, "sLightsArray[6].position");
    GLint DiffuseLocation6 = glGetUniformLocation(shaderID, "sLightsArray[6].diffuse");
    GLint SpecularLocation6 = glGetUniformLocation(shaderID, "sLightsArray[6].specular");
    GLint AttenLocation6 = glGetUniformLocation(shaderID, "sLightsArray[6].atten");
    GLint DirectionLocation6 = glGetUniformLocation(shaderID, "sLightsArray[6].direction");
    GLint Param1Location6 = glGetUniformLocation(shaderID, "sLightsArray[6].param1");
    GLint Param2Location6 = glGetUniformLocation(shaderID, "sLightsArray[6].param2");

    glm::vec3 lightPosition6 = meshArray[7]->position;
    glUniform4f(PositionLocation6, lightPosition6.x, lightPosition6.y, lightPosition6.z, 1.0f);
    //glUniform4f(PositionLocation, 0.f, 0.f, 0.f, 1.0f);
    glUniform4f(DiffuseLocation6, 70.f, 50.f, 1.f, 1.f);
    glUniform4f(SpecularLocation6, 1.f, 1.f, 1.f, 1.f);
    glUniform4f(AttenLocation6, x, y, z, 1.f);
    glUniform4f(DirectionLocation6, 1.f, 1.f, 1.f, 1.f);
    glUniform4f(Param1Location6, 0.f, 0.f, 0.f, 1.f); //x = Light Type
    glUniform4f(Param2Location6, 1.f, 0.f, 0.f, 1.f); //x = Light on/off
    
    GLint PositionLocation7 = glGetUniformLocation(shaderID, "sLightsArray[7].position");
    GLint DiffuseLocation7 = glGetUniformLocation(shaderID, "sLightsArray[7].diffuse");
    GLint SpecularLocation7 = glGetUniformLocation(shaderID, "sLightsArray[7].specular");
    GLint AttenLocation7 = glGetUniformLocation(shaderID, "sLightsArray[7].atten");
    GLint DirectionLocation7 = glGetUniformLocation(shaderID, "sLightsArray[7].direction");
    GLint Param1Location7 = glGetUniformLocation(shaderID, "sLightsArray[7].param1");
    GLint Param2Location7 = glGetUniformLocation(shaderID, "sLightsArray[7].param2");

    glm::vec3 lightPosition7 = meshArray[8]->position;
    glUniform4f(PositionLocation7, lightPosition7.x, lightPosition7.y, lightPosition7.z, 1.0f);
    //glUniform4f(PositionLocation, 0.f, 0.f, 0.f, 1.0f);
    glUniform4f(DiffuseLocation7, 70.f, 50.f, 1.f, 1.f);
    glUniform4f(SpecularLocation7, 1.f, 1.f, 1.f, 1.f);
    glUniform4f(AttenLocation7, x, y, z, 1.f);
    glUniform4f(DirectionLocation7, 1.f, 1.f, 1.f, 1.f);
    glUniform4f(Param1Location7, 0.f, 0.f, 0.f, 1.f); //x = Light Type
    glUniform4f(Param2Location7, 1.f, 0.f, 0.f, 1.f); //x = Light on/off
    
    GLint PositionLocation8 = glGetUniformLocation(shaderID, "sLightsArray[8].position");
    GLint DiffuseLocation8 = glGetUniformLocation(shaderID, "sLightsArray[8].diffuse");
    GLint SpecularLocation8 = glGetUniformLocation(shaderID, "sLightsArray[8].specular");
    GLint AttenLocation8 = glGetUniformLocation(shaderID, "sLightsArray[8].atten");
    GLint DirectionLocation8 = glGetUniformLocation(shaderID, "sLightsArray[8].direction");
    GLint Param1Location8 = glGetUniformLocation(shaderID, "sLightsArray[8].param1");
    GLint Param2Location8 = glGetUniformLocation(shaderID, "sLightsArray[8].param2");

    glm::vec3 lightPosition8 = meshArray[9]->position;
    glUniform4f(PositionLocation8, lightPosition8.x, lightPosition8.y, lightPosition8.z, 1.0f);
    //glUniform4f(PositionLocation, 0.f, 0.f, 0.f, 1.0f);
    glUniform4f(DiffuseLocation8, 70.f, 50.f, 1.f, 1.f);
    glUniform4f(SpecularLocation8, 1.f, 1.f, 1.f, 1.f);
    glUniform4f(AttenLocation8, x, y, z, 1.f);
    glUniform4f(DirectionLocation8, 1.f, 1.f, 1.f, 1.f);
    glUniform4f(Param1Location8, 0.f, 0.f, 0.f, 1.f); //x = Light Type
    glUniform4f(Param2Location8, 1.f, 0.f, 0.f, 1.f); //x = Light on/off
    
    GLint PositionLocation9 = glGetUniformLocation(shaderID, "sLightsArray[9].position");
    GLint DiffuseLocation9 = glGetUniformLocation(shaderID, "sLightsArray[9].diffuse");
    GLint SpecularLocation9 = glGetUniformLocation(shaderID, "sLightsArray[9].specular");
    GLint AttenLocation9 = glGetUniformLocation(shaderID, "sLightsArray[9].atten");
    GLint DirectionLocation9 = glGetUniformLocation(shaderID, "sLightsArray[9].direction");
    GLint Param1Location9 = glGetUniformLocation(shaderID, "sLightsArray[9].param1");
    GLint Param2Location9 = glGetUniformLocation(shaderID, "sLightsArray[9].param2");

    glm::vec3 lightPosition9 = meshArray[10]->position;
    glUniform4f(PositionLocation9, lightPosition9.x, lightPosition9.y, lightPosition9.z, 1.0f);
    //glUniform4f(PositionLocation, 0.f, 0.f, 0.f, 1.0f);
    glUniform4f(DiffuseLocation9, 70.f, 50.f, 1.f, 1.f);
    glUniform4f(SpecularLocation9, 1.f, 1.f, 1.f, 1.f);
    glUniform4f(AttenLocation9, x, y, z, 1.f);
    glUniform4f(DirectionLocation9, 1.f, 1.f, 1.f, 1.f);
    glUniform4f(Param1Location9, 0.f, 0.f, 0.f, 1.f); //x = Light Type
    glUniform4f(Param2Location9, 1.f, 0.f, 0.f, 1.f); //x = Light on/off
    
    GLint PositionLocation10 = glGetUniformLocation(shaderID, "sLightsArray[10].position");
    GLint DiffuseLocation10 = glGetUniformLocation(shaderID, "sLightsArray[10].diffuse");
    GLint SpecularLocation10 = glGetUniformLocation(shaderID, "sLightsArray[10].specular");
    GLint AttenLocation10 = glGetUniformLocation(shaderID, "sLightsArray[10].atten");
    GLint DirectionLocation10 = glGetUniformLocation(shaderID, "sLightsArray[10].direction");
    GLint Param1Location10 = glGetUniformLocation(shaderID, "sLightsArray[10].param1");
    GLint Param2Location10 = glGetUniformLocation(shaderID, "sLightsArray[10].param2");

    glm::vec3 lightPosition10 = meshArray[11]->position;
    glUniform4f(PositionLocation10, lightPosition10.x, lightPosition10.y, lightPosition10.z, 1.0f);
    //glUniform4f(PositionLocation, 0.f, 0.f, 0.f, 1.0f);
    glUniform4f(DiffuseLocation10, 70.f, 50.f, 1.f, 1.f);
    glUniform4f(SpecularLocation10, 1.f, 1.f, 1.f, 1.f);
    glUniform4f(AttenLocation10, x, y, z, 1.f);
    glUniform4f(DirectionLocation10, 1.f, 1.f, 1.f, 1.f);
    glUniform4f(Param1Location10, 0.f, 0.f, 0.f, 1.f); //x = Light Type
    glUniform4f(Param2Location10, 1.f, 0.f, 0.f, 1.f); //x = Light on/off
    
    GLint PositionLocation11 = glGetUniformLocation(shaderID, "sLightsArray[11].position");
    GLint DiffuseLocation11 = glGetUniformLocation(shaderID, "sLightsArray[11].diffuse");
    GLint SpecularLocation11 = glGetUniformLocation(shaderID, "sLightsArray[11].specular");
    GLint AttenLocation11 = glGetUniformLocation(shaderID, "sLightsArray[11].atten");
    GLint DirectionLocation11 = glGetUniformLocation(shaderID, "sLightsArray[11].direction");
    GLint Param1Location11 = glGetUniformLocation(shaderID, "sLightsArray[11].param1");
    GLint Param2Location11 = glGetUniformLocation(shaderID, "sLightsArray[11].param2");

    glm::vec3 lightPosition11 = meshArray[12]->position;
    glUniform4f(PositionLocation11, lightPosition11.x, lightPosition11.y, lightPosition11.z, 1.0f);
    //glUniform4f(PositionLocation, 0.f, 0.f, 0.f, 1.0f);
    glUniform4f(DiffuseLocation11, 70.f, 50.f, 1.f, 1.f);
    glUniform4f(SpecularLocation11, 1.f, 1.f, 1.f, 1.f);
    glUniform4f(AttenLocation11, x, y, z, 1.f);
    glUniform4f(DirectionLocation11, 1.f, 1.f, 1.f, 1.f);
    glUniform4f(Param1Location11, 0.f, 0.f, 0.f, 1.f); //x = Light Type
    glUniform4f(Param2Location11, 1.f, 0.f, 0.f, 1.f); //x = Light on/off
    
    GLint PositionLocation12 = glGetUniformLocation(shaderID, "sLightsArray[12].position");
    GLint DiffuseLocation12 = glGetUniformLocation(shaderID, "sLightsArray[12].diffuse");
    GLint SpecularLocation12 = glGetUniformLocation(shaderID, "sLightsArray[12].specular");
    GLint AttenLocation12 = glGetUniformLocation(shaderID, "sLightsArray[12].atten");
    GLint DirectionLocation12 = glGetUniformLocation(shaderID, "sLightsArray[12].direction");
    GLint Param1Location12 = glGetUniformLocation(shaderID, "sLightsArray[12].param1");
    GLint Param2Location12 = glGetUniformLocation(shaderID, "sLightsArray[12].param2");

    glm::vec3 lightPosition12 = meshArray[13]->position;
    glUniform4f(PositionLocation12, lightPosition12.x, lightPosition12.y, lightPosition12.z, 1.0f);
    //glUniform4f(PositionLocation, 0.f, 0.f, 0.f, 1.0f);
    glUniform4f(DiffuseLocation12, 70.f, 50.f, 1.f, 1.f);
    glUniform4f(SpecularLocation12, 1.f, 1.f, 1.f, 1.f);
    glUniform4f(AttenLocation12, x, y, z, 1.f);
    glUniform4f(DirectionLocation12, 1.f, 1.f, 1.f, 1.f);
    glUniform4f(Param1Location12, 0.f, 0.f, 0.f, 1.f); //x = Light Type
    glUniform4f(Param2Location12, 1.f, 0.f, 0.f, 1.f); //x = Light on/off
    
    GLint PositionLocation13 = glGetUniformLocation(shaderID, "sLightsArray[13].position");
    GLint DiffuseLocation13 = glGetUniformLocation(shaderID, "sLightsArray[13].diffuse");
    GLint SpecularLocation13 = glGetUniformLocation(shaderID, "sLightsArray[13].specular");
    GLint AttenLocation13 = glGetUniformLocation(shaderID, "sLightsArray[13].atten");
    GLint DirectionLocation13 = glGetUniformLocation(shaderID, "sLightsArray[13].direction");
    GLint Param1Location13 = glGetUniformLocation(shaderID, "sLightsArray[13].param1");
    GLint Param2Location13 = glGetUniformLocation(shaderID, "sLightsArray[12].param2");

    glm::vec3 lightPosition13 = meshArray[278]->position;
    glUniform4f(PositionLocation13, lightPosition13.x, lightPosition13.y, lightPosition13.z, 1.0f);
    //glUniform4f(PositionLocation, 0.f, 0.f, 0.f, 1.0f);
    glUniform4f(DiffuseLocation13, 70.f, 50.f, 1.f, 1.f);
    glUniform4f(SpecularLocation13, 1.f, 1.f, 1.f, 1.f);
    glUniform4f(AttenLocation13, x, y, z, 1.f);
    glUniform4f(DirectionLocation13, 1.f, 1.f, 1.f, 1.f);
    glUniform4f(Param1Location13, 0.f, 0.f, 0.f, 1.f); //x = Light Type
    glUniform4f(Param2Location13, 1.f, 0.f, 0.f, 1.f); //x = Light on/off
    
    GLint PositionLocation14 = glGetUniformLocation(shaderID, "sLightsArray[14].position");
    GLint DiffuseLocation14 = glGetUniformLocation(shaderID, "sLightsArray[14].diffuse");
    GLint SpecularLocation14 = glGetUniformLocation(shaderID, "sLightsArray[14].specular");
    GLint AttenLocation14 = glGetUniformLocation(shaderID, "sLightsArray[14].atten");
    GLint DirectionLocation14 = glGetUniformLocation(shaderID, "sLightsArray[14].direction");
    GLint Param1Location14 = glGetUniformLocation(shaderID, "sLightsArray[14].param1");
    GLint Param2Location14 = glGetUniformLocation(shaderID, "sLightsArray[14].param2");

    glm::vec3 lightPosition14 = meshArray[279]->position;
    glUniform4f(PositionLocation14, lightPosition14.x, lightPosition14.y, lightPosition14.z, 1.0f);
    //glUniform4f(PositionLocation, 0.f, 0.f, 0.f, 1.0f);
    glUniform4f(DiffuseLocation14, 70.f, 50.f, 1.f, 1.f);
    glUniform4f(SpecularLocation14, 1.f, 1.f, 1.f, 1.f);
    glUniform4f(AttenLocation14, x, y, z, 1.f);
    glUniform4f(DirectionLocation14, 1.f, 1.f, 1.f, 1.f);
    glUniform4f(Param1Location14, 0.f, 0.f, 0.f, 1.f); //x = Light Type
    glUniform4f(Param2Location14, 1.f, 0.f, 0.f, 1.f); //x = Light on/off

    GLint PositionLocation15 = glGetUniformLocation(shaderID, "sLightsArray[15].position");
    GLint DiffuseLocation15 = glGetUniformLocation(shaderID, "sLightsArray[15].diffuse");
    GLint SpecularLocation15 = glGetUniformLocation(shaderID, "sLightsArray[15].specular");
    GLint AttenLocation15 = glGetUniformLocation(shaderID, "sLightsArray[15].atten");
    GLint DirectionLocation15 = glGetUniformLocation(shaderID, "sLightsArray[15].direction");
    GLint Param1Location15 = glGetUniformLocation(shaderID, "sLightsArray[15].param1");
    GLint Param2Location15 = glGetUniformLocation(shaderID, "sLightsArray[15].param2");

    glm::vec3 lightPosition15 = meshArray[280]->position;
    glUniform4f(PositionLocation15, lightPosition15.x, lightPosition15.y, lightPosition15.z, 1.0f);
    //glUniform4f(PositionLocation, 0.f, 0.f, 0.f, 1.0f);
    glUniform4f(DiffuseLocation15, 70.f, 50.f, 1.f, 1.f);
    glUniform4f(SpecularLocation15, 1.f, 1.f, 1.f, 1.f);
    glUniform4f(AttenLocation15, x, y, z, 1.f);
    glUniform4f(DirectionLocation15, 1.f, 1.f, 1.f, 1.f);
    glUniform4f(Param1Location15, 0.f, 0.f, 0.f, 1.f); //x = Light Type
    glUniform4f(Param2Location15, 1.f, 0.f, 0.f, 1.f); //x = Light on/off

    GLint PositionLocation16 = glGetUniformLocation(shaderID, "sLightsArray[16].position");
    GLint DiffuseLocation16 = glGetUniformLocation(shaderID, "sLightsArray[16].diffuse");
    GLint SpecularLocation16 = glGetUniformLocation(shaderID, "sLightsArray[16].specular");
    GLint AttenLocation16 = glGetUniformLocation(shaderID, "sLightsArray[16].atten");
    GLint DirectionLocation16 = glGetUniformLocation(shaderID, "sLightsArray[16].direction");
    GLint Param1Location16 = glGetUniformLocation(shaderID, "sLightsArray[16].param1");
    GLint Param2Location16 = glGetUniformLocation(shaderID, "sLightsArray[16].param2");

    glm::vec3 lightPosition16 = meshArray[281]->position;
    glUniform4f(PositionLocation16, lightPosition16.x, lightPosition16.y, lightPosition16.z, 1.0f);
    //glUniform4f(PositionLocation, 0.f, 0.f, 0.f, 1.0f);
    glUniform4f(DiffuseLocation16, 70.f, 50.f, 1.f, 1.f);
    glUniform4f(SpecularLocation16, 1.f, 1.f, 1.f, 1.f);
    glUniform4f(AttenLocation16, x, y, z, 1.f);
    glUniform4f(DirectionLocation16, 1.f, 1.f, 1.f, 1.f);
    glUniform4f(Param1Location16, 0.f, 0.f, 0.f, 1.f); //x = Light Type
    glUniform4f(Param2Location16, 1.f, 0.f, 0.f, 1.f); //x = Light on/off

    GLint PositionLocation17 = glGetUniformLocation(shaderID, "sLightsArray[17].position");
    GLint DiffuseLocation17 = glGetUniformLocation(shaderID, "sLightsArray[17].diffuse");
    GLint SpecularLocation17 = glGetUniformLocation(shaderID, "sLightsArray[17].specular");
    GLint AttenLocation17 = glGetUniformLocation(shaderID, "sLightsArray[17].atten");
    GLint DirectionLocation17 = glGetUniformLocation(shaderID, "sLightsArray[17].direction");
    GLint Param1Location17 = glGetUniformLocation(shaderID, "sLightsArray[17].param1");
    GLint Param2Location17 = glGetUniformLocation(shaderID, "sLightsArray[17].param2");

    glm::vec3 lightPosition17 = meshArray[282]->position;
    glUniform4f(PositionLocation17, lightPosition17.x, lightPosition17.y, lightPosition17.z, 1.0f);
    //glUniform4f(PositionLocation, 0.f, 0.f, 0.f, 1.0f);
    glUniform4f(DiffuseLocation17, 70.f, 50.f, 1.f, 1.f);
    glUniform4f(SpecularLocation17, 1.f, 1.f, 1.f, 1.f);
    glUniform4f(AttenLocation17, x, y, z, 1.f);
    glUniform4f(DirectionLocation17, 1.f, 1.f, 1.f, 1.f);
    glUniform4f(Param1Location17, 0.f, 0.f, 0.f, 1.f); //x = Light Type
    glUniform4f(Param2Location17, 1.f, 0.f, 0.f, 1.f); //x = Light on/off

    GLint PositionLocation18 = glGetUniformLocation(shaderID, "sLightsArray[18].position");
    GLint DiffuseLocation18 = glGetUniformLocation(shaderID, "sLightsArray[18].diffuse");
    GLint SpecularLocation18 = glGetUniformLocation(shaderID, "sLightsArray[18].specular");
    GLint AttenLocation18 = glGetUniformLocation(shaderID, "sLightsArray[18].atten");
    GLint DirectionLocation18 = glGetUniformLocation(shaderID, "sLightsArray[18].direction");
    GLint Param1Location18 = glGetUniformLocation(shaderID, "sLightsArray[18].param1");
    GLint Param2Location18 = glGetUniformLocation(shaderID, "sLightsArray[18].param2");

    glm::vec3 lightPosition18 = meshArray[283]->position;
    glUniform4f(PositionLocation18, lightPosition18.x, lightPosition18.y, lightPosition18.z, 1.0f);
    //glUniform4f(PositionLocation, 0.f, 0.f, 0.f, 1.0f);
    glUniform4f(DiffuseLocation18, 70.f, 50.f, 1.f, 1.f);
    glUniform4f(SpecularLocation18, 1.f, 1.f, 1.f, 1.f);
    glUniform4f(AttenLocation18, x, y, z, 1.f);
    glUniform4f(DirectionLocation18, 1.f, 1.f, 1.f, 1.f);
    glUniform4f(Param1Location18, 0.f, 0.f, 0.f, 1.f); //x = Light Type
    glUniform4f(Param2Location18, 1.f, 0.f, 0.f, 1.f); //x = Light on/off

    GLint PositionLocation19 = glGetUniformLocation(shaderID, "sLightsArray[19].position");
    GLint DiffuseLocation19 = glGetUniformLocation(shaderID, "sLightsArray[19].diffuse");
    GLint SpecularLocation19 = glGetUniformLocation(shaderID, "sLightsArray[19].specular");
    GLint AttenLocation19 = glGetUniformLocation(shaderID, "sLightsArray[19].atten");
    GLint DirectionLocation19 = glGetUniformLocation(shaderID, "sLightsArray[19].direction");
    GLint Param1Location19 = glGetUniformLocation(shaderID, "sLightsArray[19].param1");
    GLint Param2Location19 = glGetUniformLocation(shaderID, "sLightsArray[19].param2");

    glm::vec3 lightPosition19 = meshArray[284]->position;
    glUniform4f(PositionLocation19, lightPosition19.x, lightPosition19.y, lightPosition19.z, 1.0f);
    //glUniform4f(PositionLocation, 0.f, 0.f, 0.f, 1.0f);
    glUniform4f(DiffuseLocation19, 70.f, 50.f, 1.f, 1.f);
    glUniform4f(SpecularLocation19, 1.f, 1.f, 1.f, 1.f);
    glUniform4f(AttenLocation19, x, y, z, 1.f);
    glUniform4f(DirectionLocation19, 1.f, 1.f, 1.f, 1.f);
    glUniform4f(Param1Location19, 0.f, 0.f, 0.f, 1.f); //x = Light Type
    glUniform4f(Param2Location19, 1.f, 0.f, 0.f, 1.f); //x = Light on/off

    GLint PositionLocation20 = glGetUniformLocation(shaderID, "sLightsArray[20].position");
    GLint DiffuseLocation20 = glGetUniformLocation(shaderID, "sLightsArray[20].diffuse");
    GLint SpecularLocation20 = glGetUniformLocation(shaderID, "sLightsArray[20].specular");
    GLint AttenLocation20 = glGetUniformLocation(shaderID, "sLightsArray[20].atten");
    GLint DirectionLocation20 = glGetUniformLocation(shaderID, "sLightsArray[20].direction");
    GLint Param1Location20 = glGetUniformLocation(shaderID, "sLightsArray[20].param1");
    GLint Param2Location20 = glGetUniformLocation(shaderID, "sLightsArray[20].param2");

    glm::vec3 lightPosition20 = meshArray[285]->position;
    glUniform4f(PositionLocation20, lightPosition20.x, lightPosition20.y, lightPosition20.z, 1.0f);
    //glUniform4f(PositionLocation, 0.f, 0.f, 0.f, 1.0f);
    glUniform4f(DiffuseLocation20, 70.f, 50.f, 1.f, 1.f);
    glUniform4f(SpecularLocation20, 1.f, 1.f, 1.f, 1.f);
    glUniform4f(AttenLocation20, x, y, z, 1.f);
    glUniform4f(DirectionLocation20, 1.f, 1.f, 1.f, 1.f);
    glUniform4f(Param1Location20, 0.f, 0.f, 0.f, 1.f); //x = Light Type
    glUniform4f(Param2Location20, 1.f, 0.f, 0.f, 1.f); //x = Light on/off

}

//read scene description files
void ReadSceneDescription() {
    std::ifstream File("sceneDescription.txt");
    if (!File.is_open()) {
        std::cerr << "Could not load file." << std::endl;
        return;
    }
    
    int number = 0;
    std::string input0;
    std::string input1;
    std::string input2;
    std::string input3;

    std::string temp;

    glm::vec3 position;
    glm::vec3 rotation;
    glm::vec3 scale;

    File >> number;

    for (int i = 0; i < number; i++) {
        File >> input0                                                         
             >> input1 >> position.x >> position.y >> position.z 
             >> input2 >> rotation.x >> rotation.y >> rotation.z
             >> input3 >> scale.x >> scale.y >> scale.z;

        /*  long_highway
            position 0.0 -1.0 0.0
            rotation 0.0 0.0 0.0
            scale 1.0 1.0 1.0
        */

        temp = input0;

        if (input1 == "position") {
            meshArray[i]->position.x = position.x;
            meshArray[i]->position.y = position.y;
            meshArray[i]->position.z = position.z;
        }
        if (input2 == "rotation") {
            meshArray[i]->rotation.x = rotation.x;
            meshArray[i]->rotation.y = rotation.y;
            meshArray[i]->rotation.z = rotation.z;
        }
        if (input3 == "scale") {
            meshArray[i]->scale.x = scale.x;             
            meshArray[i]->scale.y = scale.y;             
            meshArray[i]->scale.z = scale.z;             
        }
    }
    File.close();

    std::string yes;
    float x, y, z;
    std::ifstream File1("cameraEye.txt");
    if (!File1.is_open()) {
        std::cerr << "Could not load file." << std::endl;
        return;
    }
    while (File1 >> yes >> x >> y >> z) {
        ::cameraEye.x = x;
        ::cameraEye.y = y;
        ::cameraEye.z = z;
    }
    File1.close();
}

float RandomFloat(float a, float b) {
    float random = ((float)rand()) / (float)RAND_MAX;
    float diff = b - a;
    float r = random * diff;
    return a + r;
}

bool RandomizePositions(cMeshInfo* mesh) {

    int i = 0;
    float x, y, z, w;

    x = RandomFloat(-500, 500);
    y = mesh->position.y;
    z = RandomFloat(-200, 200);

    mesh->position = glm::vec3(x, y, z);
    
    return true;
}

int main(int argc, char** argv) {

    Initialize();
    Render();

    while (!glfwWindowShouldClose(window)) {
        Update();
    }

    Shutdown();

    return 0;
}
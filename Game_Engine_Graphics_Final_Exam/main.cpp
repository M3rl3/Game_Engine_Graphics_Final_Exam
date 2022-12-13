#include "OpenGL.h"
#include "MeshInfo.h"
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
ParticleAccelerator partAcc;
cRenderReticle crosshair;

sModelDrawInfo player_obj;
sModelDrawInfo cube_obj;

MeshInfo* player_mesh;
MeshInfo* cube_mesh;
MeshInfo* bulb_mesh;

unsigned int readIndex = 0;
int object_index = 0;
int elapsed_frames = 0;
int mouse_hover = 0;
float x, y, z, l = 1.f;
float speed = 0.f;
double seconds = 0.0;

bool wireFrame = false;
bool doOnce = true;
bool mouseClick = false;

std::vector <std::string> meshFiles;
std::vector <MeshInfo*> meshArray;
std::vector <MeshInfo*> teleport;

void ReadFromFile();
void ReadSceneDescription();
void LoadModel(std::string fileName, sModelDrawInfo& plyModel);
void ManageLights();
float RandomFloat(float a, float b);
bool RandomizePositions(MeshInfo* mesh);

enum eEditMode
{
    MOVING_CAMERA,
    MOVING_LIGHT,
    MOVING_SELECTED_OBJECT,
    TAKE_CONTROL
};

glm::vec3 cameraEye; //loaded from external file
//glm::vec3 cameraTarget = glm::vec3(-75.0f, 2.0f, 0.0f);

// now controlled by mouse!
glm::vec3 cameraTarget = glm::vec3(0.f, 0.f, -1.f);
eEditMode theEditMode = MOVING_CAMERA;

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
                partAcc.UpdateStep(glm::vec3(-1, 0, 0), 0.1f);
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
    cameraTarget = glm::normalize(front);
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
    MeshInfo* long_highway_mesh = new MeshInfo();
    long_highway_mesh->meshName = "long_highway";
    long_highway_mesh->isWireframe = wireFrame;
    long_highway_mesh->RGBAColour = glm::vec4(15.f, 18.f, 13.f, 1.f);
    long_highway_mesh->useRGBAColour = true;
    long_highway_mesh->drawBBox = true;
    meshArray.push_back(long_highway_mesh);
    //long_highway_mesh->CopyVertices(long_highway);
    
    sModelDrawInfo bulb;
    LoadModel(meshFiles[0], bulb);
    if (!VAOMan->LoadModelIntoVAO("bulb", bulb, shaderID)) {
        std::cerr << "Could not load model into VAO" << std::endl;
    }
    bulb_mesh = new MeshInfo();
    bulb_mesh->meshName = "bulb";
    bulb_mesh->isWireframe = wireFrame;
    meshArray.push_back(bulb_mesh);
    
    if (!VAOMan->LoadModelIntoVAO("bulb1", bulb, shaderID)) {
        std::cerr << "Could not load model into VAO" << std::endl;
    }
    MeshInfo* bulb_mesh1 = new MeshInfo();
    bulb_mesh1->meshName = "bulb1";
    bulb_mesh1->isWireframe = wireFrame;
    meshArray.push_back(bulb_mesh1);
    
    if (!VAOMan->LoadModelIntoVAO("bulb2", bulb, shaderID)) {
        std::cerr << "Could not load model into VAO" << std::endl;
    }
    MeshInfo* bulb_mesh2 = new MeshInfo();
    bulb_mesh2->meshName = "bulb2";
    bulb_mesh2->isWireframe = wireFrame;
    meshArray.push_back(bulb_mesh2); 
    
    if (!VAOMan->LoadModelIntoVAO("bulb3", bulb, shaderID)) {
        std::cerr << "Could not load model into VAO" << std::endl;
    }
    MeshInfo* bulb_mesh3 = new MeshInfo();
    bulb_mesh3->meshName = "bulb3";
    bulb_mesh3->isWireframe = wireFrame;
    meshArray.push_back(bulb_mesh3);
    
    if (!VAOMan->LoadModelIntoVAO("bulb4", bulb, shaderID)) {
        std::cerr << "Could not load model into VAO" << std::endl;
    }
    MeshInfo* bulb_mesh4 = new MeshInfo();
    bulb_mesh4->meshName = "bulb4";
    bulb_mesh4->isWireframe = wireFrame;
    meshArray.push_back(bulb_mesh4);
    
    if (!VAOMan->LoadModelIntoVAO("bulb5", bulb, shaderID)) {
        std::cerr << "Could not load model into VAO" << std::endl;
    }
    MeshInfo* bulb_mesh5 = new MeshInfo();
    bulb_mesh5->meshName = "bulb5";
    bulb_mesh5->isWireframe = wireFrame;
    meshArray.push_back(bulb_mesh5);
    
    if (!VAOMan->LoadModelIntoVAO("bulb6", bulb, shaderID)) {
        std::cerr << "Could not load model into VAO" << std::endl;
    }
    MeshInfo* bulb_mesh6 = new MeshInfo();
    bulb_mesh6->meshName = "bulb6";
    bulb_mesh6->isWireframe = wireFrame;
    meshArray.push_back(bulb_mesh6);
    
    if (!VAOMan->LoadModelIntoVAO("bulb7", bulb, shaderID)) {
        std::cerr << "Could not load model into VAO" << std::endl;
    }
    MeshInfo* bulb_mesh7 = new MeshInfo();
    bulb_mesh7->meshName = "bulb7";
    bulb_mesh7->isWireframe = wireFrame;
    meshArray.push_back(bulb_mesh7);
    
    if (!VAOMan->LoadModelIntoVAO("bulb8", bulb, shaderID)) {
        std::cerr << "Could not load model into VAO" << std::endl;
    }
    MeshInfo* bulb_mesh8 = new MeshInfo();
    bulb_mesh8->meshName = "bulb8";
    bulb_mesh8->isWireframe = wireFrame;
    meshArray.push_back(bulb_mesh8);
    
    if (!VAOMan->LoadModelIntoVAO("bulb9", bulb, shaderID)) {
        std::cerr << "Could not load model into VAO" << std::endl;
    }
    MeshInfo* bulb_mesh9 = new MeshInfo();
    bulb_mesh9->meshName = "bulb9";
    bulb_mesh9->isWireframe = wireFrame;
    meshArray.push_back(bulb_mesh9);
    
    if (!VAOMan->LoadModelIntoVAO("bulb10", bulb, shaderID)) {
        std::cerr << "Could not load model into VAO" << std::endl;
    }
    MeshInfo* bulb_mesh10 = new MeshInfo();
    bulb_mesh10->meshName = "bulb10";
    bulb_mesh10->isWireframe = wireFrame;
    meshArray.push_back(bulb_mesh10);
    
    if (!VAOMan->LoadModelIntoVAO("bulb11", bulb, shaderID)) {
        std::cerr << "Could not load model into VAO" << std::endl;
    }
    MeshInfo* bulb_mesh11 = new MeshInfo();
    bulb_mesh11->meshName = "bulb11";
    bulb_mesh11->isWireframe = wireFrame;
    meshArray.push_back(bulb_mesh11);
    
    if (!VAOMan->LoadModelIntoVAO("bulb12", bulb, shaderID)) {
        std::cerr << "Could not load model into VAO" << std::endl;
    }
    MeshInfo* bulb_mesh12 = new MeshInfo();
    bulb_mesh12->meshName = "bulb12";
    bulb_mesh12->isWireframe = wireFrame;
    meshArray.push_back(bulb_mesh12);

    sModelDrawInfo long_sidewalk;
    LoadModel(meshFiles[3], long_sidewalk);
    if (!VAOMan->LoadModelIntoVAO("long_sidewalk", long_sidewalk, shaderID)) {
        std::cerr << "Could not load model into VAO" << std::endl;
    }
    MeshInfo* long_sidewalk_mesh = new MeshInfo();
    long_sidewalk_mesh->meshName = "long_sidewalk";
    long_sidewalk_mesh->isWireframe = wireFrame;
    long_sidewalk_mesh->RGBAColour = glm::vec4(1.f, 5.f, 1.f, 1.f);
    long_sidewalk_mesh->useRGBAColour = true;
    meshArray.push_back(long_sidewalk_mesh);
    
    if (!VAOMan->LoadModelIntoVAO("long_sidewalk1", long_sidewalk, shaderID)) {
        std::cerr << "Could not load model into VAO" << std::endl;
    }
    MeshInfo* long_sidewalk_mesh1 = new MeshInfo();
    long_sidewalk_mesh1->meshName = "long_sidewalk1";
    long_sidewalk_mesh1->isWireframe = wireFrame;
    long_sidewalk_mesh1->RGBAColour = glm::vec4(1.f, 5.f, 1.f, 1.f);
    long_sidewalk_mesh1->useRGBAColour = true;
    meshArray.push_back(long_sidewalk_mesh1);
    //long_sidewalk_mesh1->CopyVertices(long_sidewalk);
    
    LoadModel(meshFiles[9], player_obj);
    if (!VAOMan->LoadModelIntoVAO("player", player_obj, shaderID)) {
        std::cerr << "Could not load model into VAO" << std::endl;
    }
    player_mesh = new MeshInfo();
    player_mesh->meshName = "player";
    player_mesh->isWireframe = wireFrame;
    player_mesh->RGBAColour = glm::vec4(255.f, 255.f, 255.f, 1.f);
    player_mesh->useRGBAColour = false;
    player_mesh->drawBBox = true;
    meshArray.push_back(player_mesh);
    player_mesh->CopyVertices(player_obj);

    // reads scene descripion files for positioning and other info
    ReadSceneDescription();

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

    view = glm::lookAt(cameraEye, cameraEye + cameraTarget, upVector);
    //projection = glm::perspective(0.6f, ratio, 0.1f, 10000.f);
    projection = glm::perspective(glm::radians(fov), ratio, 0.1f, 100000.f);

    glm::vec4 viewport = glm::vec4(0, 0, width, height);

    GLint eyeLocationLocation = glGetUniformLocation(shaderID, "eyeLocation");
    glUniform4f(eyeLocationLocation, cameraEye.x, cameraEye.y, cameraEye.z, 1.f);

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

        MeshInfo* currentMesh = meshArray[i];
        model = glm::mat4x4(1.f);

        if (currentMesh->isVisible == false) {
            continue;
        }

        glm::mat4 translationMatrix = glm::translate(glm::mat4(1.f), currentMesh->position);
        float scale = currentMesh->scale;
        glm::mat4 scaling = glm::scale(glm::mat4(1.f), glm::vec3(scale));

        glm::mat4 rotationX = glm::rotate(glm::mat4(1.f), currentMesh->rotation.x, glm::vec3(1.f, 0.f, 0.f));
        glm::mat4 rotationY = glm::rotate(glm::mat4(1.f), currentMesh->rotation.y, glm::vec3(0.f, 1.f, 0.f));
        glm::mat4 rotationZ = glm::rotate(glm::mat4(1.f), currentMesh->rotation.z, glm::vec3(0.f, 0.f, 1.f));
  
        model *= translationMatrix;
        model *= rotationX;
        model *= rotationY;
        model *= rotationZ;
        model *= scaling;

        glUniformMatrix4fv(modelLocaction, 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(viewLocation, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projectionLocation, 1, GL_FALSE, glm::value_ptr(projection));

        glm::mat4 modelInverse = glm::inverse(glm::transpose(model));
        glUniformMatrix4fv(modelInverseLocation, 1, GL_FALSE, glm::value_ptr(modelInverse));

        if (currentMesh->isWireframe) {
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        }
        else
        {
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
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

        // Randomize cube positions post every x amount of frames
        /*elapsed_frames++;
        if (elapsed_frames > 5000) {
            for (int j = 0; j < cubeMeshes.size(); j++) {
                auto theMesh = cubeMeshes[j];
                RandomizePositions(theMesh);
            }
            elapsed_frames = 0;
        }*/

        glm::vec3 cursorPos;
        // Division is expensive
        cursorPos.x = width * 0.5;
        cursorPos.y = height * 0.5;

        glm::vec3 worldSpaceCoordinates = glm::unProject(cursorPos, view, projection, viewport);
        
        glm::normalize(worldSpaceCoordinates);
        
        if (mouseClick) {
            
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

        // Only draw bounding box around meshes with this boolean value set to true
        if (currentMesh->drawBBox) {
            draw_bbox(currentMesh, shaderID, model);
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

    currentTime = glfwGetTime();
    timeDiff = currentTime - beginTime;
    frameCount++;

    if (timeDiff >= 1.f / 30.f) {
        std::string frameRate = std::to_string((1.f / timeDiff) * frameCount);
        std::string frameTime = std::to_string((timeDiff / frameCount) * 1000);

        std::stringstream ss;
        ss << " Camera: " << "(" << cameraEye.x << ", " << cameraEye.y << ", " << cameraEye.z << ")"
           << " Target: Index = " << object_index << ", MeshName: " << meshArray[object_index]->meshName << ", Position: (" << meshArray[object_index]->position.x << ", " << meshArray[object_index]->position.y << ", " << meshArray[object_index]->position.z << ")"
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
    float scale;

    File >> number;

    for (int i = 0; i < number; i++) {
        File >> input0                                                         
             >> input1 >> position.x >> position.y >> position.z 
             >> input2 >> rotation.x >> rotation.y >> rotation.z
             >> input3 >> scale;

        /*long_highway
        position 0.0 -1.0 0.0
        rotation 0.0 0.0 0.0
        scale 1.0*/

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
            meshArray[i]->scale = scale;             
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

bool RandomizePositions(MeshInfo* mesh) {

    int i = 0;
    float x, y, z, w;

    x = RandomFloat(-80, 80);
    y = mesh->position.y;
    z = RandomFloat(-20, 20);

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
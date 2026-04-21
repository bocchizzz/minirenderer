#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <iostream>
#include <vector>
#include <string>
#include <filesystem>

#include "Camera.h"
#include "Model.h"
#include "Light.h"
#include "Controller.h"
#include "SceneSerializer.h"
#include "Renderer.h"

const unsigned int SCR_WIDTH  = 1920;
const unsigned int SCR_HEIGHT = 1080;

Camera camera(glm::vec3(0.0f, 5.0f, 10.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;
float deltaTime = 0.0f;
float lastFrame = 0.0f;

bool isDraggingCamera = false;
bool isDraggingModel  = false;
bool isRotatingModel  = false;
Model* selectedModel  = nullptr;
std::vector<Model*>     models;
std::vector<PointLight> lights;

LightingModel currentLightingModel = LightingModel::BLINN_PHONG;

// 函数声明
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);
void loadModelsFromDirectory();
glm::vec3 getRayFromMouse(double mouseX, double mouseY);

// Renderer 实例（全局，用于 resize 回调）
Renderer* renderer = nullptr;

int main() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 4);

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Deferred Renderer", NULL, NULL);
    if (!window) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetScrollCallback(window, scroll_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 430");

    // 创建 Renderer 实例
    renderer = new Renderer(SCR_WIDTH, SCR_HEIGHT);

    Controller GuiController(currentLightingModel, selectedModel, models, lights, SCR_WIDTH, SCR_HEIGHT, renderer);

    // 设置场景保存/加载回调
    GuiController.onSaveScene = [&](const std::string& path) {
        bool success = SceneSerializer::SaveScene(path, models, lights, camera, (int)currentLightingModel);
        if (success) {
            std::cout << "Scene saved successfully to: " << path << std::endl;
        } else {
            std::cout << "Failed to save scene to: " << path << std::endl;
        }
    };

    GuiController.onLoadScene = [&](const std::string& path) {
        int loadedLightingModel = 0;
        bool success = SceneSerializer::LoadScene(path, models, lights, camera, loadedLightingModel);
        if (success) {
            currentLightingModel = (LightingModel)loadedLightingModel;
            selectedModel = nullptr;
            std::cout << "Scene loaded successfully from: " << path << std::endl;
        } else {
            std::cout << "Failed to load scene from: " << path << std::endl;
        }
    };

    glEnable(GL_DEPTH_TEST);


    // 主渲染循环
    while (!glfwWindowShouldClose(window)) {
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);

        // 使用 Renderer 渲染场景
        renderer->renderScene(models, lights, camera, currentLightingModel, selectedModel);

        // ImGui UI
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        GuiController.run();

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
        glfwPollEvents();
    }


    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    for (auto m : models)
        delete m;

    glfwTerminate();
    return 0;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
    if (renderer) {
        renderer->resize(width, height);
    }
}

void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (!ImGui::GetIO().WantCaptureKeyboard) {
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) camera.ProcessKeyboard(FORWARD,  deltaTime);
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) camera.ProcessKeyboard(BACKWARD, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) camera.ProcessKeyboard(LEFT,     deltaTime);
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) camera.ProcessKeyboard(RIGHT,    deltaTime);
    }
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = (float)xpos;
        lastY = (float)ypos;
        firstMouse = false;
    }
    float xoffset = (float)xpos - lastX;
    float yoffset = lastY - (float)ypos;
    lastX = (float)xpos;
    lastY = (float)ypos;

    if (isDraggingCamera && !ImGui::GetIO().WantCaptureMouse)
        camera.ProcessMouseMovement(xoffset, yoffset);

    if (isDraggingModel && selectedModel) {
        glm::vec3 right = camera.Right;
        glm::vec3 up    = glm::vec3(0, 1, 0);
        selectedModel->position += right * xoffset * 0.01f;
        selectedModel->position += up    * yoffset * 0.01f;
    }

    if (isRotatingModel && selectedModel) {
        selectedModel->rotation.y += xoffset * 0.5f;
        selectedModel->rotation.x += yoffset * 0.5f;
    }
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    if (ImGui::GetIO().WantCaptureMouse) return;

    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            double xpos, ypos;
            glfwGetCursorPos(window, &xpos, &ypos);
            glm::vec3 ray = getRayFromMouse(xpos, ypos);

            float  minDist = FLT_MAX;
            Model* hitPtr  = nullptr;

            for (auto m : models) {
                float dist;
                if (m->RayIntersect(camera.Position, ray, dist) && dist < minDist) {
                    minDist = dist;
                    hitPtr  = m;
                }
            }

            if (hitPtr) {
                for (auto m : models) m->isSelected = false;
                hitPtr->isSelected = true;
                selectedModel      = hitPtr;
                isDraggingModel    = true;
            } else {
                for (auto m : models) m->isSelected = false;
                selectedModel   = nullptr;
                isDraggingCamera = true;
            }
        } else if (action == GLFW_RELEASE) {
            isDraggingCamera = false;
            isDraggingModel  = false;
        }
    }

    if (button == GLFW_MOUSE_BUTTON_RIGHT) {
        if (action == GLFW_PRESS && selectedModel)
            isRotatingModel = true;
        else if (action == GLFW_RELEASE)
            isRotatingModel = false;
    }
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    camera.ProcessMouseScroll(static_cast<float>(yoffset));
}

glm::vec3 getRayFromMouse(double mouseX, double mouseY) {
    float x = (2.0f * (float)mouseX) / SCR_WIDTH  - 1.0f;
    float y = 1.0f - (2.0f * (float)mouseY) / SCR_HEIGHT;
    glm::vec4 rayClip = glm::vec4(x, y, -1.0f, 1.0f);

    glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom),
                                             (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 200.0f);
    glm::vec4 rayEye = glm::inverse(projection) * rayClip;
    rayEye = glm::vec4(rayEye.x, rayEye.y, -1.0f, 0.0f);

    glm::vec3 rayWorld = glm::vec3(glm::inverse(camera.GetViewMatrix()) * rayEye);
    return glm::normalize(rayWorld);
}

void loadModelsFromDirectory() {
    namespace fs = std::filesystem;
    std::string modelsDir = "models";
    if (!fs::exists(modelsDir) || !fs::is_directory(modelsDir)) return;

    for (const auto& entry : fs::directory_iterator(modelsDir)) {
        if (!fs::is_directory(entry.path())) continue;

        std::string modelFile;
        for (const auto& file : fs::directory_iterator(entry.path())) {
            std::string ext = file.path().extension().string();
            if (ext == ".obj" || ext == ".fbx" || ext == ".dae" ||
                ext == ".gltf" || ext == ".blend") {
                modelFile = file.path().string();
                break;
            }
        }
        if (!modelFile.empty()) {
            std::string name = entry.path().filename().string();
            Model* model = new Model(modelFile, name);

            // 只有加载成功的模型才添加到场景中
            if (model->loadSuccess) {
                models.push_back(model);
            } else {
                delete model;  // 释放失败的模型
            }
        }
    }
}

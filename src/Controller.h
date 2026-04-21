#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <functional>
#include "Model.h"
#include "Light.h"
#include "Camera.h"
#include "Renderer.h"

class Controller
{
    public:
    Controller(LightingModel& CLM, Model*& SM, std::vector<Model*>& models, std::vector<PointLight>& lights,
        const unsigned int SCR_WIDTH, const unsigned int SCR_HEIGHT, Renderer* renderer)
    :currentLightingModel(CLM), selectedModel(SM), models(models), lights(lights),
    SCR_WIDTH(SCR_WIDTH), SCR_HEIGHT(SCR_HEIGHT), renderer(renderer)
    {};

    void run();
    std::string openFileDialog();
    std::string saveFileDialog();  // 新增：保存文件对话框
    std::string openHDRFileDialog();

    Renderer* renderer;

    // 场景管理回调
    std::function<void(const std::string&)> onSaveScene;
    std::function<void(const std::string&)> onLoadScene;


    bool showLightManagerWindow  = false;
    bool showAddLightWindow      = false;
    bool showEditLightWindow     = false;
    bool showLightingModelWindow = false;
    int  editingLightIndex       = -1;
    int  nextLightID             = 1;
    LightingModel& currentLightingModel;

    Model*& selectedModel;
    std::vector<Model*>&     models;
    std::vector<PointLight>& lights;

    const unsigned int SCR_WIDTH;
    const unsigned int SCR_HEIGHT;

    float newLightPos[3]   = { 0.0f, 10.0f, 0.0f };
    float newLightIntensity = 50.0f;  // 默认50，适合物理正确的平方反比衰减
    float newLightColor[3] = { 1.0f, 1.0f, 1.0f };



private:
    // 新增：材质编辑窗口状态
    bool showMaterialEditorWindow = false;
    int editingMaterialIndex = -1;
    int editingMeshIndex = -1;

    // 新增：场景管理窗口状态
    bool showSceneManagerWindow = false;
};
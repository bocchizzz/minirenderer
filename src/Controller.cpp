#include "Controller.h"
#include <string>
#include <windows.h>
#include <commdlg.h>
#include <filesystem>
#include <iostream>

void Controller::run()
{
    // 控制面板
    ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(250, 220), ImGuiCond_Always);
    ImGui::Begin("Control Panel", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

    if (ImGui::Button("Light Manager", ImVec2(230, 25)))
        showLightManagerWindow = true;
    if (ImGui::Button("Change Lighting Model", ImVec2(230, 25)))
        showLightingModelWindow = true;
    if (ImGui::Button("Add Model", ImVec2(230, 25))) {
        std::string path = openFileDialog();
        if (!path.empty()) {
            namespace fs = std::filesystem;
            std::string name = fs::path(path).stem().string();
            Model* model = new Model(path, name);

            // 只有加载成功的模型才添加到场景中
            if (model->loadSuccess) {
                models.push_back(model);
                std::cout << "Model loaded successfully: " << name << std::endl;
            } else {
                std::cout << "Model loading failed, discarded: " << name << std::endl;
                delete model;  // 释放失败的模型
            }
        }
    }
    if (ImGui::Button("Load Environment Map", ImVec2(230, 25))) {
    std::string path = openHDRFileDialog();
    if (!path.empty() && renderer) {
        if (renderer->loadHDREnvironment(path)) {
            std::cout << "Environment map loaded successfully!" << std::endl;
        } else {
            std::cout << "Failed to load environment map!" << std::endl;
        }
    }
    else {
        renderer->cleanupIBL();
    }
    }

    // IBL强度滑块（仅在有环境贴图时显示）
    if (renderer && renderer->hasEnvironmentMap()) {
        ImGui::SliderFloat("IBL Intensity", &renderer->iblIntensity, 0.0f, 5.0f, "%.2f");
    }

    // 全局曝光滑块
    ImGui::SliderFloat("Exposure", &renderer->exposure, 0.1f, 10.0f, "%.2f");
    if (ImGui::Button("Scene Manager", ImVec2(230, 25)))
        showSceneManagerWindow = true;
    ImGui::End();

    // Light Manager 窗口
    if (showLightManagerWindow) {
        ImGui::SetNextWindowPos(ImVec2(SCR_WIDTH / 2 - 200, SCR_HEIGHT / 2 - 250), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(400, 500), ImGuiCond_Always);
        ImGui::Begin("Light Manager", &showLightManagerWindow, ImGuiWindowFlags_NoResize);

        if (ImGui::Button("Add Light", ImVec2(380, 30))) {
            showAddLightWindow = true;
            newLightPos[0] = 0.0f; newLightPos[1] = 10.0f; newLightPos[2] = 0.0f;
            newLightIntensity = 50.0f;  // 默认50，适合物理正确的平方反比衰减
            newLightColor[0] = newLightColor[1] = newLightColor[2] = 1.0f;
        }

        ImGui::Separator();
        ImGui::Text("Light List:");
        ImGui::Separator();

        ImGui::BeginChild("LightList", ImVec2(0, 380), true);
        for (int i = 0; i < (int)lights.size(); i++) {
            char label[128];
            sprintf_s(label, "Light #%d - Pos(%.1f, %.1f, %.1f)",
                lights[i].id,
                lights[i].position.x,
                lights[i].position.y,
                lights[i].position.z);

            if (ImGui::Selectable(label, false, 0, ImVec2(0, 30))) {
                editingLightIndex  = i;
                newLightPos[0]     = lights[i].position.x;
                newLightPos[1]     = lights[i].position.y;
                newLightPos[2]     = lights[i].position.z;
                newLightIntensity  = lights[i].intensity;
                newLightColor[0]   = lights[i].color.r;
                newLightColor[1]   = lights[i].color.g;
                newLightColor[2]   = lights[i].color.b;
                showEditLightWindow = true;
            }
        }
        ImGui::EndChild();
        ImGui::End();
    }

    // Add Light 窗口
    if (showAddLightWindow) {
        ImGui::SetNextWindowPos(ImVec2(SCR_WIDTH / 2 - 200, SCR_HEIGHT / 2 - 150), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(400, 250), ImGuiCond_Always);
        ImGui::Begin("Add Light", &showAddLightWindow, ImGuiWindowFlags_NoResize);

        ImGui::InputFloat3("Position", newLightPos);
        ImGui::InputFloat("Intensity", &newLightIntensity);
        ImGui::ColorEdit3("Color", newLightColor);

        if (ImGui::Button("Confirm", ImVec2(280, 30))) {
            lights.push_back(PointLight(
                nextLightID++,
                glm::vec3(newLightPos[0], newLightPos[1], newLightPos[2]),
                glm::vec3(newLightColor[0], newLightColor[1], newLightColor[2]),
                newLightIntensity
            ));
            showAddLightWindow = false;
        }
        ImGui::End();
    }

    // Edit Light 窗口
    if (showEditLightWindow && editingLightIndex >= 0 && editingLightIndex < (int)lights.size()) {
        ImGui::SetNextWindowPos(ImVec2(SCR_WIDTH / 2 - 150, SCR_HEIGHT / 2 - 175), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(300, 300), ImGuiCond_Always);
        ImGui::Begin("Edit Light", &showEditLightWindow, ImGuiWindowFlags_NoResize);

        ImGui::Text("Light #%d", lights[editingLightIndex].id);
        ImGui::Separator();

        ImGui::InputFloat3("Position", newLightPos);
        ImGui::InputFloat("Intensity", &newLightIntensity);
        ImGui::ColorEdit3("Color", newLightColor);

        if (ImGui::Button("Confirm", ImVec2(280, 30))) {
            lights[editingLightIndex].position  = glm::vec3(newLightPos[0], newLightPos[1], newLightPos[2]);
            lights[editingLightIndex].color     = glm::vec3(newLightColor[0], newLightColor[1], newLightColor[2]);
            lights[editingLightIndex].intensity = newLightIntensity;
            showEditLightWindow = false;
            editingLightIndex   = -1;
        }

        ImGui::Separator();
        if (ImGui::Button("Delete Light", ImVec2(280, 30))) {
            lights.erase(lights.begin() + editingLightIndex);
            showEditLightWindow = false;
            editingLightIndex   = -1;
        }
        ImGui::End();
    }

    // Lighting Model 选择窗口
    if (showLightingModelWindow) {
        ImGui::SetNextWindowPos(ImVec2(SCR_WIDTH / 2 - 100, SCR_HEIGHT / 2 - 100), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(200, 180), ImGuiCond_Always);
        ImGui::Begin("Select Lighting Model", &showLightingModelWindow, ImGuiWindowFlags_NoResize);

        if (ImGui::Selectable("Blinn-Phong", currentLightingModel == LightingModel::BLINN_PHONG)) {
            currentLightingModel = LightingModel::BLINN_PHONG;
            showLightingModelWindow = false;
        }
        if (ImGui::Selectable("Phong", currentLightingModel == LightingModel::PHONG)) {
            currentLightingModel = LightingModel::PHONG;
            showLightingModelWindow = false;
        }
        if (ImGui::Selectable("PBR", currentLightingModel == LightingModel::PBR)) {
            currentLightingModel = LightingModel::PBR;
            showLightingModelWindow = false;
        }
        ImGui::End();
    }

    // Model Properties 窗口
    if (selectedModel) {
        ImGui::SetNextWindowPos(ImVec2(SCR_WIDTH - 310, 10), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(300, 600), ImGuiCond_Always);
        ImGui::Begin("Model Properties", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

        ImGui::Text("Model: %s", selectedModel->name.c_str());
        ImGui::Separator();

        // 变换控制
        float pos[3] = { selectedModel->position.x, selectedModel->position.y, selectedModel->position.z };
        ImGui::Text("Position");
        if (ImGui::InputFloat3("X, Y, Z", pos))
            selectedModel->position = glm::vec3(pos[0], pos[1], pos[2]);

        float rot[3] = { selectedModel->rotation.x, selectedModel->rotation.y, selectedModel->rotation.z };
        ImGui::Text("Rotation");
        if (ImGui::InputFloat3("R, P, Y", rot))
            selectedModel->rotation = glm::vec3(rot[0], rot[1], rot[2]);

        float sc[3] = { selectedModel->scale.x, selectedModel->scale.y, selectedModel->scale.z };
        ImGui::Text("Scale");
        if (ImGui::InputFloat3("X, Y, Z ", sc))
            selectedModel->scale = glm::vec3(sc[0], sc[1], sc[2]);

        ImGui::Separator();

        // Mesh列表（可折叠）
        if (ImGui::CollapsingHeader("Meshes", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::BeginChild("MeshList", ImVec2(0, 200), true);
            for (int i = 0; i < (int)selectedModel->meshes.size(); i++) {
                auto& mesh = selectedModel->meshes[i];
                ImGui::PushID(i);

                // 显示mesh信息
                ImGui::Text("%s", mesh.name.c_str());
                ImGui::SameLine(180);
                ImGui::Text("(%d verts)", (int)mesh.vertices.size());

                // 显示当前使用的材质
                if (mesh.materialIndex >= 0 && mesh.materialIndex < (int)selectedModel->materials.size()) {
                    ImGui::Text("  Material: %s", selectedModel->materials[mesh.materialIndex].name.c_str());
                } else {
                    ImGui::Text("  Material: None");
                }

                // 更改材质按钮
                if (ImGui::Button("Change Material", ImVec2(260, 20))) {
                    editingMeshIndex = i;
                }

                // 材质选择下拉菜单
                if (editingMeshIndex == i) {
                    ImGui::Indent();
                    for (int j = 0; j < (int)selectedModel->materials.size(); j++) {
                        bool isSelected = (mesh.materialIndex == j);
                        if (ImGui::Selectable(selectedModel->materials[j].name.c_str(), isSelected)) {
                            mesh.materialIndex = j;
                            editingMeshIndex = -1;
                        }
                    }
                    ImGui::Unindent();
                }

                ImGui::Separator();
                ImGui::PopID();
            }
            ImGui::EndChild();
        }

        ImGui::Separator();

        // Materials列表（可折叠）
        if (ImGui::CollapsingHeader("Materials", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::BeginChild("MaterialList", ImVec2(0, 200), true);
            for (int i = 0; i < (int)selectedModel->materials.size(); i++) {
                auto& mat = selectedModel->materials[i];
                ImGui::PushID(i);

                ImGui::Text("%s", mat.name.c_str());

                // 显示透明度状态
                ImGui::SameLine(180);
                if (mat.isTransparent) {
                    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "[Transparent]");
                } else {
                    ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "[Opaque]");
                }

                // 编辑材质按钮
                if (ImGui::Button("Edit", ImVec2(130, 20))) {
                    editingMaterialIndex = i;
                    showMaterialEditorWindow = true;
                }

                ImGui::SameLine();

                // 删除材质按钮（如果有多个材质）
                if (selectedModel->materials.size() > 1) {
                    if (ImGui::Button("Delete", ImVec2(130, 20))) {
                        if (selectedModel->removeMaterial(i)) {
                            ImGui::PopID();
                            break;  // 删除后跳出循环
                        }
                    }
                }

                ImGui::Separator();
                ImGui::PopID();
            }
            ImGui::EndChild();

            // 添加新材质按钮
            if (ImGui::Button("Add New Material", ImVec2(280, 25))) {
                int newIndex = selectedModel->addMaterial("New Material");
                editingMaterialIndex = newIndex;
                showMaterialEditorWindow = true;
            }
        }

        ImGui::Separator();

        if (ImGui::Button("Delete Model", ImVec2(280, 30))) {
            auto it = std::find(models.begin(), models.end(), selectedModel);
            if (it != models.end()) {
                delete *it;
                models.erase(it);
                selectedModel = nullptr;
            }
        }
        ImGui::End();
    }

    // Material Editor 窗口
    if (showMaterialEditorWindow && selectedModel && editingMaterialIndex >= 0 &&
        editingMaterialIndex < (int)selectedModel->materials.size()) {

        ImGui::SetNextWindowPos(ImVec2(SCR_WIDTH / 2 - 200, SCR_HEIGHT / 2 - 300), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(400, 700), ImGuiCond_Always);
        ImGui::Begin("Material Editor", &showMaterialEditorWindow, ImGuiWindowFlags_NoResize);

        Material* mat = &selectedModel->materials[editingMaterialIndex];

        ImGui::Text("Material: %s", mat->name.c_str());
        ImGui::Separator();

        // 显示透明度信息
        // ImGui::Text("Transparency:");
        // ImGui::SameLine();
        // if (mat->isTransparent) {
        //     ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Transparent");
        // } else {
        //     ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Opaque");
        // }

        // Alpha阈值滑块
        // ImGui::SliderFloat("Alpha Threshold", &mat->alphaThreshold, 0.0f, 1.0f);
        // ImGui::Text("(Pixels with alpha < threshold will be discarded)");

        // ImGui::Separator();

        // 定义所有PBR贴图类型
        struct TextureButton {
            const char* label;
            const char* type;
            bool* hasFlag;
        };

        TextureButton textureButtons[] = {
            {"Change Albedo Map", "Albedo", &mat->hasAlbedoMap},
            {"Change Diffuse Map", "Diffuse", &mat->hasDiffuseMap},
            {"Change Normal Map", "Normal", &mat->hasNormalMap},
            {"Change Metallic Map", "Metallic", &mat->hasMetallicMap},
            {"Change Roughness Map", "Roughness", &mat->hasRoughnessMap},
            {"Change AO Map", "Ao", &mat->hasAoMap},
            {"Change Cavity Map", "Cavity", &mat->hasCavityMap},
            {"Change Gloss Map", "Gloss", &mat->hasGlossMap},
            {"Change Specular Map", "Specular", &mat->hasSpecularMap},
            {"Change Fuzz Map", "Fuzz", &mat->hasFuzzMap},
            {"Change Displacement Map", "Displacement", &mat->hasDisplacementMap},
            {"Change Opacity Map", "Opacity", &mat->hasOpacityMap},
            {"Change Alpha Map", "Alpha", &mat->hasAlphaMap},
            {"Change Emissive Map", "Emissive", &mat->hasEmissiveMap}
        };

        ImGui::BeginChild("TextureButtons", ImVec2(0, 500), true);
        for (const auto& btn : textureButtons) {
            // 显示是否已加载
            if (*btn.hasFlag) {
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "[Loaded]");
                ImGui::SameLine();
            } else {
                ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "[Empty] ");
                ImGui::SameLine();
            }

            // Change按钮
            if (ImGui::Button(btn.label, ImVec2(220, 25))) {
                std::string path = openFileDialog();
                if (!path.empty()) {
                    mat->loadTexture(path, btn.type);
                }
            }

            // 如果贴图已加载，显示删除按钮
            if (*btn.hasFlag) {
                ImGui::SameLine();
                std::string deleteLabel = std::string("Delete##") + btn.type;
                if (ImGui::Button(deleteLabel.c_str(), ImVec2(60, 25))) {
                    mat->removeTexture(btn.type);
                }
            }
        }
        ImGui::EndChild();

        // 自发光属性控制
        ImGui::Separator();
        ImGui::Text("Emissive Settings:");
        float emColor[3] = { mat->emissiveColor.r, mat->emissiveColor.g, mat->emissiveColor.b };
        if (ImGui::ColorEdit3("Emissive Color", emColor)) {
            mat->emissiveColor = glm::vec3(emColor[0], emColor[1], emColor[2]);
        }
        ImGui::SliderFloat("Emissive Intensity", &mat->emissiveIntensity, 0.0f, 50.0f, "%.1f");

        if (ImGui::Button("Close", ImVec2(380, 30))) {
            showMaterialEditorWindow = false;
            editingMaterialIndex = -1;
        }

        ImGui::End();
    }

    // Scene Manager 窗口
    if (showSceneManagerWindow) {
        ImGui::SetNextWindowPos(ImVec2(SCR_WIDTH / 2 - 200, SCR_HEIGHT / 2 - 100), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(400, 200), ImGuiCond_Always);
        ImGui::Begin("Scene Manager", &showSceneManagerWindow, ImGuiWindowFlags_NoResize);

        ImGui::Text("Scene Management");
        ImGui::Separator();

        // 显示当前场景信息
        ImGui::Text("Models: %d", (int)models.size());
        ImGui::Text("Lights: %d", (int)lights.size());

        ImGui::Separator();

        // 保存场景按钮
        if (ImGui::Button("Save Scene", ImVec2(380, 35))) {
            std::string path = saveFileDialog();
            if (!path.empty()) {
                // 确保文件扩展名为.scene
                if (path.find(".scene") == std::string::npos && path.find(".json") == std::string::npos) {
                    path += ".scene";
                }
                // 调用保存场景回调
                if (onSaveScene) {
                    onSaveScene(path);
                }
            }
        }

        // 加载场景按钮
        if (ImGui::Button("Load Scene", ImVec2(380, 35))) {
            std::string path = openFileDialog();
            if (!path.empty()) {
                // 调用加载场景回调
                if (onLoadScene) {
                    onLoadScene(path);
                }
            }
        }

        ImGui::End();
    }
}

std::string Controller::openFileDialog()
{
    OPENFILENAMEA ofn;
    char szFile[260] = { 0 };
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize  = sizeof(ofn);
    ofn.hwndOwner    = NULL;
    ofn.lpstrFile    = szFile;
    ofn.nMaxFile     = sizeof(szFile);
    ofn.lpstrFilter  = "All Files\0*.*\0Model Files\0*.obj;*.fbx;*.dae;*.gltf;*.blend\0Image Files\0*.png;*.jpg;*.tga\0Scene Files\0*.scene;*.json\0";
    ofn.nFilterIndex = 1;
    ofn.Flags        = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
    if (GetOpenFileNameA(&ofn) == TRUE)
        return std::string(ofn.lpstrFile);
    return "";
}

std::string Controller::saveFileDialog()
{
    OPENFILENAMEA ofn;
    char szFile[260] = { 0 };
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize  = sizeof(ofn);
    ofn.hwndOwner    = NULL;
    ofn.lpstrFile    = szFile;
    ofn.nMaxFile     = sizeof(szFile);
    ofn.lpstrFilter  = "Scene Files\0*.scene\0JSON Files\0*.json\0All Files\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrDefExt  = "scene";
    ofn.Flags        = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;
    if (GetSaveFileNameA(&ofn) == TRUE)
        return std::string(ofn.lpstrFile);
    return "";
}

std::string Controller::openHDRFileDialog()
{
    OPENFILENAMEA ofn;
    char szFile[260] = { 0 };
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize  = sizeof(ofn);
    ofn.hwndOwner    = NULL;
    ofn.lpstrFile    = szFile;
    ofn.nMaxFile     = sizeof(szFile);
    ofn.lpstrFilter  = "HDR Files\0*.hdr\0";
    ofn.nFilterIndex = 1;
    ofn.Flags        = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
    if (GetOpenFileNameA(&ofn) == TRUE)
        return std::string(ofn.lpstrFile);
    return "";
}

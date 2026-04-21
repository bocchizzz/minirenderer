#include "SceneSerializer.h"
#include <json.hpp>
#include <fstream>
#include <iostream>
#include <filesystem>

using json = nlohmann::json;
namespace fs = std::filesystem;

// 将绝对路径转换为相对于场景文件的相对路径
std::string SceneSerializer::ToRelativePath(const std::string& absolutePath, const std::string& sceneDir) {
    try {
        // 将模型路径转换为绝对路径（如果是相对路径，则相对于当前工作目录）
        fs::path absPath = fs::absolute(absolutePath);

        // 场景文件所在目录的绝对路径
        fs::path sceneDirPath = fs::absolute(sceneDir);

        // 计算相对路径
        fs::path relativePath = fs::relative(absPath, sceneDirPath);

        return relativePath.string();
    } catch (...) {
        // 如果转换失败，返回原路径
        return absolutePath;
    }
}

// 将相对路径转换为绝对路径
std::string SceneSerializer::ToAbsolutePath(const std::string& relativePath, const std::string& sceneDir) {
    try {
        fs::path relPath(relativePath);

        // 场景文件所在目录的绝对路径
        fs::path sceneDirPath = fs::absolute(sceneDir);

        // 将相对路径与场景目录组合
        fs::path absolutePath = sceneDirPath / relPath;

        // 规范化路径（解析 .. 和 . 等）
        absolutePath = fs::canonical(absolutePath);

        return absolutePath.string();
    } catch (const std::exception& e) {
        // 如果转换失败（例如文件不存在），尝试不使用 canonical
        try {
            fs::path relPath(relativePath);
            fs::path sceneDirPath = fs::absolute(sceneDir);
            fs::path absolutePath = sceneDirPath / relPath;
            return absolutePath.string();
        } catch (...) {
            // 如果还是失败，返回原路径
            return relativePath;
        }
    }
}

// 保存场景到JSON文件
bool SceneSerializer::SaveScene(const std::string& scenePath,
                                const std::vector<Model*>& models,
                                const std::vector<PointLight>& lights,
                                const Camera& camera,
                                int lightingModel) {
    try {
        // 获取场景文件所在目录
        fs::path sceneFilePath(scenePath);
        std::string sceneDir = sceneFilePath.parent_path().string();

        // 创建JSON对象
        json sceneJson;
        sceneJson["scene"]["version"] = "1.0";
        sceneJson["scene"]["name"] = sceneFilePath.stem().string();

        // 保存模型信息
        json modelsArray = json::array();
        for (size_t i = 0; i < models.size(); i++) {
            Model* model = models[i];
            json modelJson;

            modelJson["instanceId"] = i;
            modelJson["name"] = model->name;

            // 将模型完整路径转换为相对路径
            modelJson["modelPath"] = ToRelativePath(model->filePath, sceneDir);

            // 保存Transform信息
            modelJson["transform"]["position"] = {model->position.x, model->position.y, model->position.z};
            modelJson["transform"]["rotation"] = {model->rotation.x, model->rotation.y, model->rotation.z};
            modelJson["transform"]["scale"] = {model->scale.x, model->scale.y, model->scale.z};

            // 保存材质覆盖信息（只保存用户修改过的材质）
            json materialOverrides = json::array();
            for (size_t j = 0; j < model->materials.size(); j++) {
                const Material& mat = model->materials[j];

                // 检查材质是否有贴图（如果有贴图说明可能被修改过）
                bool hasAnyTexture = mat.hasAlbedoMap || mat.hasDiffuseMap || mat.hasNormalMap ||
                                     mat.hasMetallicMap || mat.hasRoughnessMap || mat.hasAoMap ||
                                     mat.hasCavityMap || mat.hasGlossMap || mat.hasSpecularMap ||
                                     mat.hasFuzzMap || mat.hasDisplacementMap || mat.hasOpacityMap ||
                                     mat.hasAlphaMap || mat.hasEmissiveMap;

                // 自发光颜色/强度非零也需要保存
                bool hasEmissiveSettings = mat.emissiveIntensity > 0.0f;

                if (hasAnyTexture) {
                    json matJson;
                    matJson["materialIndex"] = j;
                    matJson["materialName"] = mat.name;
                    matJson["isTransparent"] = mat.isTransparent;
                    matJson["alphaThreshold"] = mat.alphaThreshold;

                    // 保存所有贴图路径（转换为相对路径）
                    json texturesJson = json::object();

                    if (mat.hasAlbedoMap && !mat.albedoMapPath.empty()) {
                        texturesJson["albedo"] = ToRelativePath(mat.albedoMapPath, sceneDir);
                    }
                    if (mat.hasDiffuseMap && !mat.diffuseMapPath.empty()) {
                        texturesJson["diffuse"] = ToRelativePath(mat.diffuseMapPath, sceneDir);
                    }
                    if (mat.hasNormalMap && !mat.normalMapPath.empty()) {
                        texturesJson["normal"] = ToRelativePath(mat.normalMapPath, sceneDir);
                    }
                    if (mat.hasMetallicMap && !mat.metallicMapPath.empty()) {
                        texturesJson["metallic"] = ToRelativePath(mat.metallicMapPath, sceneDir);
                    }
                    if (mat.hasRoughnessMap && !mat.roughnessMapPath.empty()) {
                        texturesJson["roughness"] = ToRelativePath(mat.roughnessMapPath, sceneDir);
                    }
                    if (mat.hasAoMap && !mat.aoMapPath.empty()) {
                        texturesJson["ao"] = ToRelativePath(mat.aoMapPath, sceneDir);
                    }
                    if (mat.hasCavityMap && !mat.cavityMapPath.empty()) {
                        texturesJson["cavity"] = ToRelativePath(mat.cavityMapPath, sceneDir);
                    }
                    if (mat.hasGlossMap && !mat.glossMapPath.empty()) {
                        texturesJson["gloss"] = ToRelativePath(mat.glossMapPath, sceneDir);
                    }
                    if (mat.hasSpecularMap && !mat.specularMapPath.empty()) {
                        texturesJson["specular"] = ToRelativePath(mat.specularMapPath, sceneDir);
                    }
                    if (mat.hasFuzzMap && !mat.fuzzMapPath.empty()) {
                        texturesJson["fuzz"] = ToRelativePath(mat.fuzzMapPath, sceneDir);
                    }
                    if (mat.hasDisplacementMap && !mat.displacementMapPath.empty()) {
                        texturesJson["displacement"] = ToRelativePath(mat.displacementMapPath, sceneDir);
                    }
                    if (mat.hasOpacityMap && !mat.opacityMapPath.empty()) {
                        texturesJson["opacity"] = ToRelativePath(mat.opacityMapPath, sceneDir);
                    }
                    if (mat.hasAlphaMap && !mat.alphaMapPath.empty()) {
                        texturesJson["alpha"] = ToRelativePath(mat.alphaMapPath, sceneDir);
                    }

                    matJson["textures"] = texturesJson;
                    materialOverrides.push_back(matJson);
                }
            }

            if (!materialOverrides.empty()) {
                modelJson["materialOverrides"] = materialOverrides;
            }

            modelsArray.push_back(modelJson);
        }
        sceneJson["scene"]["models"] = modelsArray;

        // 保存光源信息
        json lightsArray = json::array();
        for (size_t i = 0; i < lights.size(); i++) {
            const PointLight& light = lights[i];
            json lightJson;

            lightJson["id"] = light.id;
            lightJson["position"] = {light.position.x, light.position.y, light.position.z};
            lightJson["color"] = {light.color.r, light.color.g, light.color.b};
            lightJson["intensity"] = light.intensity;

            lightsArray.push_back(lightJson);
        }
        sceneJson["scene"]["lights"] = lightsArray;

        // 保存相机信息
        sceneJson["scene"]["camera"]["position"] = {camera.Position.x, camera.Position.y, camera.Position.z};
        sceneJson["scene"]["camera"]["yaw"] = camera.Yaw;
        sceneJson["scene"]["camera"]["pitch"] = camera.Pitch;
        sceneJson["scene"]["camera"]["zoom"] = camera.Zoom;

        // 保存渲染设置
        sceneJson["scene"]["renderSettings"]["lightingModel"] = lightingModel;

        // 写入文件
        std::ofstream file(scenePath);
        if (!file.is_open()) {
            std::cout << "Failed to open file for writing: " << scenePath << std::endl;
            return false;
        }

        file << sceneJson.dump(4); // 4空格缩进，便于阅读
        file.close();

        std::cout << "Scene saved successfully: " << scenePath << std::endl;
        return true;

    } catch (const std::exception& e) {
        std::cout << "Error saving scene: " << e.what() << std::endl;
        return false;
    }
}

// 从JSON文件加载场景
bool SceneSerializer::LoadScene(const std::string& scenePath,
                                std::vector<Model*>& models,
                                std::vector<PointLight>& lights,
                                Camera& camera,
                                int& lightingModel) {
    try {
        // 读取文件
        std::ifstream file(scenePath);
        if (!file.is_open()) {
            std::cout << "Failed to open scene file: " << scenePath << std::endl;
            return false;
        }

        json sceneJson;
        file >> sceneJson;
        file.close();

        // 获取场景文件所在目录
        fs::path sceneFilePath(scenePath);
        std::string sceneDir = sceneFilePath.parent_path().string();

        // 清空现有数据
        for (auto model : models) {
            delete model;
        }
        models.clear();
        lights.clear();

        // 加载模型
        if (sceneJson["scene"].contains("models")) {
            for (const auto& modelJson : sceneJson["scene"]["models"]) {
                // 将相对路径转换为绝对路径
                std::string modelPath = ToAbsolutePath(modelJson["modelPath"], sceneDir);

                // 获取模型名称
                std::string modelName = "Model";
                if (modelJson.contains("name")) {
                    modelName = modelJson["name"];
                }

                // 加载模型
                Model* model = new Model(modelPath, modelName);

                // 只有加载成功的模型才添加到场景中
                if (!model->loadSuccess) {
                    std::cout << "Scene loading: Model loading failed, skipped: " << modelName << std::endl;
                    delete model;
                    continue;
                }

                // 设置Transform
                if (modelJson.contains("transform")) {
                    auto& transform = modelJson["transform"];

                    if (transform.contains("position")) {
                        model->position = glm::vec3(transform["position"][0],
                                                    transform["position"][1],
                                                    transform["position"][2]);
                    }

                    if (transform.contains("rotation")) {
                        model->rotation = glm::vec3(transform["rotation"][0],
                                                    transform["rotation"][1],
                                                    transform["rotation"][2]);
                    }

                    if (transform.contains("scale")) {
                        model->scale = glm::vec3(transform["scale"][0],
                                                 transform["scale"][1],
                                                 transform["scale"][2]);
                    }
                }

                // 应用材质覆盖
                if (modelJson.contains("materialOverrides")) {
                    for (const auto& matOverride : modelJson["materialOverrides"]) {
                        int matIndex = matOverride["materialIndex"];

                        if (matIndex >= 0 && matIndex < (int)model->materials.size()) {
                            Material& mat = model->materials[matIndex];

                            if (matOverride.contains("isTransparent")) {
                                mat.isTransparent = matOverride["isTransparent"];
                            }

                            if (matOverride.contains("alphaThreshold")) {
                                mat.alphaThreshold = matOverride["alphaThreshold"];
                            }

                            // 加载贴图
                            if (matOverride.contains("textures")) {
                                const auto& textures = matOverride["textures"];

                                if (textures.contains("albedo")) {
                                    std::string texPath = ToAbsolutePath(textures["albedo"], sceneDir);
                                    mat.loadTexture(texPath, "Albedo");
                                }
                                if (textures.contains("diffuse")) {
                                    std::string texPath = ToAbsolutePath(textures["diffuse"], sceneDir);
                                    mat.loadTexture(texPath, "Diffuse");
                                }
                                if (textures.contains("normal")) {
                                    std::string texPath = ToAbsolutePath(textures["normal"], sceneDir);
                                    mat.loadTexture(texPath, "Normal");
                                }
                                if (textures.contains("metallic")) {
                                    std::string texPath = ToAbsolutePath(textures["metallic"], sceneDir);
                                    mat.loadTexture(texPath, "Metallic");
                                }
                                if (textures.contains("roughness")) {
                                    std::string texPath = ToAbsolutePath(textures["roughness"], sceneDir);
                                    mat.loadTexture(texPath, "Roughness");
                                }
                                if (textures.contains("ao")) {
                                    std::string texPath = ToAbsolutePath(textures["ao"], sceneDir);
                                    mat.loadTexture(texPath, "Ao");
                                }
                                if (textures.contains("cavity")) {
                                    std::string texPath = ToAbsolutePath(textures["cavity"], sceneDir);
                                    mat.loadTexture(texPath, "Cavity");
                                }
                                if (textures.contains("gloss")) {
                                    std::string texPath = ToAbsolutePath(textures["gloss"], sceneDir);
                                    mat.loadTexture(texPath, "Gloss");
                                }
                                if (textures.contains("specular")) {
                                    std::string texPath = ToAbsolutePath(textures["specular"], sceneDir);
                                    mat.loadTexture(texPath, "Specular");
                                }
                                if (textures.contains("fuzz")) {
                                    std::string texPath = ToAbsolutePath(textures["fuzz"], sceneDir);
                                    mat.loadTexture(texPath, "Fuzz");
                                }
                                if (textures.contains("displacement")) {
                                    std::string texPath = ToAbsolutePath(textures["displacement"], sceneDir);
                                    mat.loadTexture(texPath, "Displacement");
                                }
                                if (textures.contains("opacity")) {
                                    std::string texPath = ToAbsolutePath(textures["opacity"], sceneDir);
                                    mat.loadTexture(texPath, "Opacity");
                                }
                                if (textures.contains("alpha")) {
                                    std::string texPath = ToAbsolutePath(textures["alpha"], sceneDir);
                                    mat.loadTexture(texPath, "Alpha");
                                }
                            }
                        }
                    }
                }

                models.push_back(model);
            }
        }

        // 加载光源
        if (sceneJson["scene"].contains("lights")) {
            for (const auto& lightJson : sceneJson["scene"]["lights"]) {
                PointLight light;

                light.id = lightJson["id"];
                light.position = glm::vec3(lightJson["position"][0],
                                          lightJson["position"][1],
                                          lightJson["position"][2]);
                light.color = glm::vec3(lightJson["color"][0],
                                       lightJson["color"][1],
                                       lightJson["color"][2]);
                light.intensity = lightJson["intensity"];

                lights.push_back(light);
            }
        }

        // 加载相机
        if (sceneJson["scene"].contains("camera")) {
            auto& cameraJson = sceneJson["scene"]["camera"];

            if (cameraJson.contains("position")) {
                camera.Position = glm::vec3(cameraJson["position"][0],
                                           cameraJson["position"][1],
                                           cameraJson["position"][2]);
            }

            if (cameraJson.contains("yaw")) {
                camera.Yaw = cameraJson["yaw"];
            }

            if (cameraJson.contains("pitch")) {
                camera.Pitch = cameraJson["pitch"];
            }

            if (cameraJson.contains("zoom")) {
                camera.Zoom = cameraJson["zoom"];
            }

            camera.updateCameraVectors();
        }

        // 加载渲染设置
        if (sceneJson["scene"].contains("renderSettings")) {
            auto& renderSettings = sceneJson["scene"]["renderSettings"];

            if (renderSettings.contains("lightingModel")) {
                lightingModel = renderSettings["lightingModel"];
            }
        }

        std::cout << "Scene loaded successfully: " << scenePath << std::endl;
        std::cout << "  Models: " << models.size() << std::endl;
        std::cout << "  Lights: " << lights.size() << std::endl;

        return true;

    } catch (const std::exception& e) {
        std::cout << "Error loading scene: " << e.what() << std::endl;
        return false;
    }
}

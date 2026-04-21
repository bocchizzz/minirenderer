#pragma once
#include <string>
#include <vector>
#include "Model.h"
#include "Light.h"
#include "Camera.h"

// 场景序列化器：负责场景的保存和加载
class SceneSerializer {
public:
    // 保存场景到JSON文件
    // scenePath: 场景文件路径 (.scene或.json)
    // models: 所有模型的指针列表
    // lights: 所有光源的列表
    // camera: 相机对象
    // lightingModel: 当前光照模型 (0=Blinn-Phong, 1=Phong, 2=PBR)
    static bool SaveScene(const std::string& scenePath,
                          const std::vector<Model*>& models,
                          const std::vector<PointLight>& lights,
                          const Camera& camera,
                          int lightingModel);

    // 从JSON文件加载场景
    // scenePath: 场景文件路径
    // models: 输出参数,加载的模型列表
    // lights: 输出参数,加载的光源列表
    // camera: 输出参数,相机状态
    // lightingModel: 输出参数,光照模型
    static bool LoadScene(const std::string& scenePath,
                          std::vector<Model*>& models,
                          std::vector<PointLight>& lights,
                          Camera& camera,
                          int& lightingModel);

private:
    // 将绝对路径转换为相对于场景文件的相对路径
    static std::string ToRelativePath(const std::string& absolutePath, const std::string& sceneDir);

    // 将相对路径转换为绝对路径
    static std::string ToAbsolutePath(const std::string& relativePath, const std::string& sceneDir);
};

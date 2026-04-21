#pragma once
#include <string>
#include <glad/glad.h>
#include <glm/glm.hpp>

// Material类：封装PBR材质的所有贴图
class Material {
public:
    std::string name;  // 材质名称

    // PBR贴图ID
    unsigned int albedoMapID;
    unsigned int diffuseMapID;
    unsigned int normalMapID;
    unsigned int aoMapID;
    unsigned int cavityMapID;
    unsigned int glossMapID;
    unsigned int metallicMapID;
    unsigned int roughnessMapID;
    unsigned int specularMapID;
    unsigned int fuzzMapID;
    unsigned int displacementMapID;
    unsigned int opacityMapID;
    unsigned int alphaMapID;  // Alpha贴图（透明度）
    unsigned int emissiveMapID; // 自发光贴图

    // 贴图路径（用于场景保存）
    std::string albedoMapPath;
    std::string diffuseMapPath;
    std::string normalMapPath;
    std::string aoMapPath;
    std::string cavityMapPath;
    std::string glossMapPath;
    std::string metallicMapPath;
    std::string roughnessMapPath;
    std::string specularMapPath;
    std::string fuzzMapPath;
    std::string displacementMapPath;
    std::string opacityMapPath;
    std::string alphaMapPath;
    std::string emissiveMapPath;

    // 贴图存在标志
    bool hasAlbedoMap;
    bool hasDiffuseMap;
    bool hasNormalMap;
    bool hasAoMap;
    bool hasCavityMap;
    bool hasGlossMap;
    bool hasMetallicMap;
    bool hasRoughnessMap;
    bool hasSpecularMap;
    bool hasFuzzMap;
    bool hasDisplacementMap;
    bool hasOpacityMap;
    bool hasAlphaMap;  // 是否有Alpha贴图
    bool hasEmissiveMap; // 是否有自发光贴图

    // 自发光属性（无贴图时使用颜色和强度）
    glm::vec3 emissiveColor;    // 自发光颜色
    float emissiveIntensity;    // 自发光强度

    // 透明度相关
    bool isTransparent;        // 是否为透明材质
    float alphaThreshold;      // Alpha测试阈值（小于此值的片段会被丢弃）

    // 构造函数
    Material(const std::string& materialName = "Default");

    // 析构函数：释放所有贴图资源
    ~Material();

    // 禁用拷贝构造和赋值（避免重复释放纹理）
    Material(const Material&) = delete;
    Material& operator=(const Material&) = delete;

    // 移动构造和移动赋值
    Material(Material&& other) noexcept;
    Material& operator=(Material&& other) noexcept;

    // 加载贴图
    bool loadTexture(const std::string& texturePath, const std::string& textureType);

    // 删除指定类型的贴图
    void removeTexture(const std::string& textureType);

    // 清空所有贴图
    void clearAllTextures();

private:
    // 辅助函数：根据类型获取对应的ID和标志指针
    void getTexturePointers(const std::string& textureType, unsigned int*& idPtr, bool*& flagPtr);

    // 辅助函数：根据类型获取对应的路径指针
    std::string* getTexturePath(const std::string& textureType);
};

#pragma once
#include <vector>
#include <string>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "Material.h"
#include "Frustum.h"

struct Vertex {
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec2 TexCoords;
    glm::vec3 Tangent;
    glm::vec3 Bitangent;
};

struct Mesh {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    unsigned int VAO, VBO, EBO;
    int materialIndex;  // 该mesh使用的材质索引
    std::string name;   // mesh名称（用于UI显示）

    Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices, int matIndex = 0, const std::string& meshName = "");
    void Draw();
    void setupMesh();
};

class Model {
public:
    std::vector<Mesh> meshes;
    std::vector<Material> materials;  // 材质数组
    std::string directory;  // 模型文件所在目录
    std::string filePath;   // 模型文件的完整路径
    std::string name;

    glm::vec3 position;
    glm::vec3 rotation;
    glm::vec3 scale;

    bool isSelected;
    bool loadSuccess;  // 模型是否加载成功

    // 包围盒（局部空间）
    AABB localAABB;

    // 保留旧的贴图变量用于向后兼容（已废弃，但保留以防万一）
    unsigned int textureID;
    unsigned int normalMapID;
    bool hasTexture;
    bool hasNormalMap;
    unsigned int albedoMapID;
    unsigned int diffuseMapID;
    unsigned int aoMapID;
    unsigned int cavityMapID;
    unsigned int glossMapID;
    unsigned int metallicMapID;
    unsigned int roughnessMapID;
    unsigned int specularMapID;
    unsigned int fuzzMapID;
    unsigned int displacementMapID;
    bool hasAlbedoMap;
    bool hasDiffuseMap;
    bool hasAoMap;
    bool hasCavityMap;
    bool hasGlossMap;
    bool hasMetallicMap;
    bool hasRoughnessMap;
    bool hasSpecularMap;
    bool hasFuzzMap;
    bool hasDisplacementMap;

    Model(const std::string& path, const std::string& modelName);
    void Draw();
    glm::mat4 GetModelMatrix();
    bool RayIntersect(glm::vec3 rayOrigin, glm::vec3 rayDir, float& distance);

    // 获取世界空间的包围盒
    AABB getWorldAABB() const;

    // 检查模型是否在视锥体内
    bool isInFrustum(const Frustum& frustum) const;

    // 材质管理方法
    int addMaterial(const std::string& materialName);  // 添加新材质，返回索引
    bool removeMaterial(int index);  // 删除材质
    Material* getMaterial(int index);  // 获取材质指针

    unsigned int loadTextureFromFile(const std::string& filename);

private:
    void loadModel(std::string path);
    void processNode(aiNode* node, const aiScene* scene, const glm::mat4& parentTransform);
    Mesh processMesh(aiMesh* mesh, const aiScene* scene, int meshIndex, const glm::mat4& transform);
    void loadMaterialTextures(const aiScene* scene);

    // 计算模型的局部包围盒
    void calculateAABB();
};

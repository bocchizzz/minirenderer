#include "Model.h"
#include <glad/glad.h>
#include <iostream>
#include <stb_image.h>
#include <filesystem>
#include <limits>

Mesh::Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices, int matIndex, const std::string& meshName)
    : vertices(vertices), indices(indices), materialIndex(matIndex), name(meshName) {
    setupMesh();
}

void Mesh::setupMesh() {
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoords));
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Tangent));
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Bitangent));

    glBindVertexArray(0);
}

void Mesh::Draw() {
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

Model::Model(const std::string& path, const std::string& modelName)
    : position(0.0f), rotation(0.0f), scale(0.01f), hasTexture(false),
      hasNormalMap(false), isSelected(false), loadSuccess(false), textureID(0), normalMapID(0), name(modelName),
      albedoMapID(0), diffuseMapID(0), aoMapID(0), cavityMapID(0), glossMapID(0),
      metallicMapID(0), roughnessMapID(0), specularMapID(0), fuzzMapID(0), displacementMapID(0),
      hasAlbedoMap(false), hasDiffuseMap(false), hasAoMap(false), hasCavityMap(false), hasGlossMap(false),
      hasMetallicMap(false), hasRoughnessMap(false), hasSpecularMap(false), hasFuzzMap(false), hasDisplacementMap(false) {

    namespace fs = std::filesystem;
    fs::path modelPath(path);

    // 检查文件是否存在
    if (!fs::exists(modelPath)) {
        std::cout << "Error: Model file does not exist: " << path << std::endl;
        return;
    }

    // 检查文件格式是否支持
    std::string ext = modelPath.extension().string();
    std::vector<std::string> supportedFormats = {".obj", ".fbx", ".dae", ".gltf", ".glb", ".blend", ".3ds", ".ase", ".ifc"};
    bool formatSupported = false;
    for (const auto& format : supportedFormats) {
        if (ext == format) {
            formatSupported = true;
            break;
        }
    }

    if (!formatSupported) {
        std::cout << "Error: Unsupported model format: " << ext << std::endl;
        std::cout << "Supported formats: .obj, .fbx, .dae, .gltf, .glb, .blend, .3ds, .ase, .ifc" << std::endl;
        return;
    }

    directory = modelPath.parent_path().string();
    filePath = path;  // 保存完整路径

    loadModel(path);
}

void Model::loadModel(std::string path) {
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        std::cout << "Error: Assimp failed to load model: " << importer.GetErrorString() << std::endl;
        loadSuccess = false;
        return;
    }

    // 先加载所有材质
    loadMaterialTextures(scene);

    // 然后处理节点和mesh，传入单位矩阵作为初始变换
    glm::mat4 identity(1.0f);
    processNode(scene->mRootNode, scene, identity);

    // 计算包围盒
    calculateAABB();

    // 标记加载成功
    loadSuccess = true;

    // std::cout << "Model loaded: " << name << " (" << meshes.size() << " meshes, "
    //           << materials.size() << " materials)" << std::endl;
}

void Model::processNode(aiNode* node, const aiScene* scene, const glm::mat4& parentTransform) {
    // 将Assimp的矩阵转换为GLM矩阵
    aiMatrix4x4 aiMat = node->mTransformation;
    glm::mat4 nodeTransform = glm::mat4(
        aiMat.a1, aiMat.b1, aiMat.c1, aiMat.d1,
        aiMat.a2, aiMat.b2, aiMat.c2, aiMat.d2,
        aiMat.a3, aiMat.b3, aiMat.c3, aiMat.d3,
        aiMat.a4, aiMat.b4, aiMat.c4, aiMat.d4
    );

    // 累积变换：父变换 * 当前节点变换
    glm::mat4 globalTransform = parentTransform * nodeTransform;

    // 处理该节点的所有mesh
    for (unsigned int i = 0; i < node->mNumMeshes; i++) {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        meshes.push_back(processMesh(mesh, scene, node->mMeshes[i], globalTransform));
    }

    // 递归处理子节点
    for (unsigned int i = 0; i < node->mNumChildren; i++) {
        processNode(node->mChildren[i], scene, globalTransform);
    }
}

Mesh Model::processMesh(aiMesh* mesh, const aiScene* scene, int meshIndex, const glm::mat4& transform) {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    // 提取法线变换矩阵（用于正确变换法线）
    glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(transform)));

    for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
        Vertex vertex;

        // 应用变换到顶点位置
        glm::vec4 pos = transform * glm::vec4(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z, 1.0f);
        vertex.Position = glm::vec3(pos);

        // 应用法线矩阵到法线（法线不受平移影响，只受旋转和缩放影响）
        glm::vec3 normal = normalMatrix * glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);
        vertex.Normal = glm::normalize(normal);

        if (mesh->mTextureCoords[0]) {
            vertex.TexCoords = glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y);
        } else {
            vertex.TexCoords = glm::vec2(0.0f, 0.0f);
        }

        if (mesh->mTangents) {
            // 切线和副切线也需要变换
            glm::vec3 tangent = normalMatrix * glm::vec3(mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z);
            vertex.Tangent = glm::normalize(tangent);

            glm::vec3 bitangent = normalMatrix * glm::vec3(mesh->mBitangents[i].x, mesh->mBitangents[i].y, mesh->mBitangents[i].z);
            vertex.Bitangent = glm::normalize(bitangent);
        }

        vertices.push_back(vertex);
    }

    for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
        aiFace face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; j++)
            indices.push_back(face.mIndices[j]);
    }

    // 获取mesh的材质索引
    int materialIndex = mesh->mMaterialIndex;

    // 确保材质索引有效
    if (materialIndex < 0 || materialIndex >= (int)materials.size()) {
        materialIndex = 0;  // 使用默认材质
    }

    // 获取mesh名称
    std::string meshName = mesh->mName.C_Str();
    if (meshName.empty()) {
        meshName = "Mesh_" + std::to_string(meshIndex);
    }

    return Mesh(vertices, indices, materialIndex, meshName);
}

unsigned int Model::loadTextureFromFile(const std::string& filename) {
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char* data = stbi_load(filename.c_str(), &width, &height, &nrComponents, 0);
    if (data) {
        GLenum format;
        if (nrComponents == 1) format = GL_RED;
        else if (nrComponents == 3) format = GL_RGB;
        else if (nrComponents == 4) format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    } else {
        std::cout << "Texture failed to load at path: " << filename << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}

void Model::Draw() {
    for (auto& mesh : meshes) {
        mesh.Draw();
    }
}

glm::mat4 Model::GetModelMatrix() {
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, position);
    model = glm::rotate(model, glm::radians(rotation.x), glm::vec3(1, 0, 0));
    model = glm::rotate(model, glm::radians(rotation.y), glm::vec3(0, 1, 0));
    model = glm::rotate(model, glm::radians(rotation.z), glm::vec3(0, 0, 1));
    model = glm::scale(model, scale);
    return model;
}

bool Model::RayIntersect(glm::vec3 rayOrigin, glm::vec3 rayDir, float& distance) {
    glm::mat4 modelMatrix = GetModelMatrix();
    bool hit = false;
    float minDist = FLT_MAX;

    for (auto& mesh : meshes) {
        for (size_t i = 0; i < mesh.indices.size(); i += 3) {
            glm::vec3 v0 = glm::vec3(modelMatrix * glm::vec4(mesh.vertices[mesh.indices[i]].Position, 1.0f));
            glm::vec3 v1 = glm::vec3(modelMatrix * glm::vec4(mesh.vertices[mesh.indices[i + 1]].Position, 1.0f));
            glm::vec3 v2 = glm::vec3(modelMatrix * glm::vec4(mesh.vertices[mesh.indices[i + 2]].Position, 1.0f));

            glm::vec3 edge1 = v1 - v0;
            glm::vec3 edge2 = v2 - v0;
            glm::vec3 h = glm::cross(rayDir, edge2);
            float a = glm::dot(edge1, h);

            if (a > -0.00001f && a < 0.00001f) continue;

            float f = 1.0f / a;
            glm::vec3 s = rayOrigin - v0;
            float u = f * glm::dot(s, h);

            if (u < 0.0f || u > 1.0f) continue;

            glm::vec3 q = glm::cross(s, edge1);
            float v = f * glm::dot(rayDir, q);

            if (v < 0.0f || u + v > 1.0f) continue;

            float t = f * glm::dot(edge2, q);

            if (t > 0.00001f && t < minDist) {
                minDist = t;
                hit = true;
            }
        }
    }

    if (hit) {
        distance = minDist;
        return true;
    }
    return false;
}

// 材质管理方法实现
int Model::addMaterial(const std::string& materialName) {
    materials.push_back(Material(materialName));
    return (int)materials.size() - 1;
}

bool Model::removeMaterial(int index) {
    if (index < 0 || index >= (int)materials.size()) {
        return false;
    }

    // 检查是否有mesh在使用这个材质
    for (auto& mesh : meshes) {
        if (mesh.materialIndex == index) {
            std::cout << "Warning: Material " << index << " is in use, cannot delete" << std::endl;
            return false;
        }
    }

    materials.erase(materials.begin() + index);

    // 更新所有mesh的材质索引
    for (auto& mesh : meshes) {
        if (mesh.materialIndex > index) {
            mesh.materialIndex--;
        }
    }

    return true;
}

Material* Model::getMaterial(int index) {
    if (index < 0 || index >= (int)materials.size()) {
        return nullptr;
    }
    return &materials[index];
}

void Model::loadMaterialTextures(const aiScene* scene) {
    namespace fs = std::filesystem;

    // 如果场景中没有材质，创建一个默认材质
    if (scene->mNumMaterials == 0) {
        materials.push_back(Material("Default"));
        std::cout << "No materials in scene, creating default material" << std::endl;
        return;
    }

    // 遍历所有材质
    for (unsigned int i = 0; i < scene->mNumMaterials; i++) {
        aiMaterial* mat = scene->mMaterials[i];
        aiString matName;
        mat->Get(AI_MATKEY_NAME, matName);

        std::string materialName = matName.C_Str();
        if (materialName.empty()) {
            materialName = "Material_" + std::to_string(i);
        }

        Material material(materialName);

        // 定义贴图类型映射
        struct TextureTypeMapping {
            aiTextureType aiType;
            std::string ourType;
        };

        TextureTypeMapping textureTypes[] = {
            {aiTextureType_DIFFUSE, "Diffuse"},
            {aiTextureType_BASE_COLOR, "Albedo"},
            {aiTextureType_NORMALS, "Normal"},
            {aiTextureType_METALNESS, "Metallic"},
            {aiTextureType_DIFFUSE_ROUGHNESS, "Roughness"},
            {aiTextureType_AMBIENT_OCCLUSION, "Ao"},
            {aiTextureType_SPECULAR, "Specular"},
            {aiTextureType_SHININESS, "Gloss"},
            {aiTextureType_DISPLACEMENT, "Displacement"},
            {aiTextureType_OPACITY, "Alpha"}  // Opacity贴图实际上是Alpha贴图
        };

        // 加载每种类型的贴图
        for (const auto& texType : textureTypes) {
            unsigned int texCount = mat->GetTextureCount(texType.aiType);
            for (unsigned int j = 0; j < texCount; j++) {
                aiString texPath;
                if (mat->GetTexture(texType.aiType, j, &texPath) == AI_SUCCESS) {
                    std::string fullPath = directory + "\\" + std::string(texPath.C_Str());
                    bool found = false;
                    // 1. 尝试原始路径
                    if (fs::exists(fullPath)) {
                        material.loadTexture(fullPath, texType.ourType);
                        found = true;
                    } else {
                        // 2. 尝试只用文件名(原始扩展名)
                        fs::path p(texPath.C_Str());
                        fullPath = directory + "\\" + p.filename().string();

                        if (fs::exists(fullPath)) {
                            material.loadTexture(fullPath, texType.ourType);
                            found = true;
                        } else {
                            // 3. 尝试匹配相对路径(保留目录结构,只替换扩展名)
                            fs::path originalPath(texPath.C_Str());
                            fs::path parentPath = originalPath.parent_path(); // 保留父目录(如textures/)
                            std::string baseName = originalPath.stem().string(); // 获取不带扩展名的文件名
                            std::vector<std::string> extensions = {".png", ".jpg", ".jpeg", ".tga", ".bmp"};

                            for (const auto& ext : extensions) {
                                // 重新组合: directory + 父目录 + 文件名 + 新扩展名
                                std::string testPath = directory + "\\" + parentPath.string() + "\\" + baseName + ext;
                                if (fs::exists(testPath)) {
                                    material.loadTexture(testPath, texType.ourType);
                                    found = true;
                                    // std::cout << "Texture format mismatch resolved: " << texPath.C_Str()
                                    //           << " -> " << parentPath.string() << "\\" << baseName << ext << std::endl;
                                    break;
                                }
                            }
                        }
                    }

                    if (!found) {
                        std::cout << "Warning: Texture file not found: " << texPath.C_Str() << std::endl;
                    }
                }
            }
        }

        materials.push_back(std::move(material));
    }

    // 如果没有成功加载任何材质，添加一个默认材质
    if (materials.empty()) {
        materials.push_back(Material("Default"));
    }

    std::cout << "Loaded " << materials.size() << " materials" << std::endl;
}

void Model::calculateAABB() {
    if (meshes.empty()) {
        localAABB = AABB(glm::vec3(0.0f), glm::vec3(0.0f));
        return;
    }

    // 初始化为极值
    glm::vec3 min(std::numeric_limits<float>::max());
    glm::vec3 max(std::numeric_limits<float>::lowest());

    // 遍历所有mesh的所有顶点
    for (const auto& mesh : meshes) {
        for (const auto& vertex : mesh.vertices) {
            min = glm::min(min, vertex.Position);
            max = glm::max(max, vertex.Position);
        }
    }

    localAABB = AABB(min, max);
}

AABB Model::getWorldAABB() const {
    // 获取模型矩阵
    glm::mat4 modelMatrix = glm::mat4(1.0f);
    modelMatrix = glm::translate(modelMatrix, position);
    modelMatrix = glm::rotate(modelMatrix, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
    modelMatrix = glm::rotate(modelMatrix, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
    modelMatrix = glm::rotate(modelMatrix, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
    modelMatrix = glm::scale(modelMatrix, scale);

    // 变换局部包围盒的8个角点到世界空间
    glm::vec3 corners[8] = {
        glm::vec3(localAABB.min.x, localAABB.min.y, localAABB.min.z),
        glm::vec3(localAABB.max.x, localAABB.min.y, localAABB.min.z),
        glm::vec3(localAABB.min.x, localAABB.max.y, localAABB.min.z),
        glm::vec3(localAABB.max.x, localAABB.max.y, localAABB.min.z),
        glm::vec3(localAABB.min.x, localAABB.min.y, localAABB.max.z),
        glm::vec3(localAABB.max.x, localAABB.min.y, localAABB.max.z),
        glm::vec3(localAABB.min.x, localAABB.max.y, localAABB.max.z),
        glm::vec3(localAABB.max.x, localAABB.max.y, localAABB.max.z)
    };

    // 初始化世界空间包围盒
    glm::vec3 worldMin(std::numeric_limits<float>::max());
    glm::vec3 worldMax(std::numeric_limits<float>::lowest());

    // 变换所有角点并更新世界空间包围盒
    for (int i = 0; i < 8; i++) {
        glm::vec4 worldCorner = modelMatrix * glm::vec4(corners[i], 1.0f);
        glm::vec3 corner3D = glm::vec3(worldCorner);
        worldMin = glm::min(worldMin, corner3D);
        worldMax = glm::max(worldMax, corner3D);
    }

    return AABB(worldMin, worldMax);
}

bool Model::isInFrustum(const Frustum& frustum) const {
    // 获取世界空间的包围盒
    AABB worldAABB = getWorldAABB();

    // 使用AABB进行视锥剔除检测
    return frustum.isAABBInside(worldAABB);
}


#include "Material.h"
#include <iostream>
#include <algorithm>

// STB Image for common formats (PNG, JPG, TGA, etc.)
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

Material::Material(const std::string& materialName)
    : name(materialName),
      albedoMapID(0), diffuseMapID(0), normalMapID(0), aoMapID(0), cavityMapID(0),
      glossMapID(0), metallicMapID(0), roughnessMapID(0), specularMapID(0),
      fuzzMapID(0), displacementMapID(0), opacityMapID(0), alphaMapID(0), emissiveMapID(0),
      hasAlbedoMap(false), hasDiffuseMap(false), hasNormalMap(false), hasAoMap(false),
      hasCavityMap(false), hasGlossMap(false), hasMetallicMap(false), hasRoughnessMap(false),
      hasSpecularMap(false), hasFuzzMap(false), hasDisplacementMap(false), hasOpacityMap(false),
      hasAlphaMap(false), hasEmissiveMap(false),
      emissiveColor(0.0f), emissiveIntensity(0.0f),
      isTransparent(false), alphaThreshold(0.1f) {
}

Material::~Material() {
    clearAllTextures();
}

Material::Material(Material&& other) noexcept
    : name(std::move(other.name)),
      albedoMapID(other.albedoMapID), diffuseMapID(other.diffuseMapID),
      normalMapID(other.normalMapID), aoMapID(other.aoMapID), cavityMapID(other.cavityMapID),
      glossMapID(other.glossMapID), metallicMapID(other.metallicMapID),
      roughnessMapID(other.roughnessMapID), specularMapID(other.specularMapID),
      fuzzMapID(other.fuzzMapID), displacementMapID(other.displacementMapID),
      opacityMapID(other.opacityMapID), alphaMapID(other.alphaMapID), emissiveMapID(other.emissiveMapID),
      hasAlbedoMap(other.hasAlbedoMap), hasDiffuseMap(other.hasDiffuseMap),
      hasNormalMap(other.hasNormalMap), hasAoMap(other.hasAoMap),
      hasCavityMap(other.hasCavityMap), hasGlossMap(other.hasGlossMap),
      hasMetallicMap(other.hasMetallicMap), hasRoughnessMap(other.hasRoughnessMap),
      hasSpecularMap(other.hasSpecularMap), hasFuzzMap(other.hasFuzzMap),
      hasDisplacementMap(other.hasDisplacementMap), hasOpacityMap(other.hasOpacityMap),
      hasAlphaMap(other.hasAlphaMap), hasEmissiveMap(other.hasEmissiveMap),
      emissiveColor(other.emissiveColor), emissiveIntensity(other.emissiveIntensity),
      isTransparent(other.isTransparent), alphaThreshold(other.alphaThreshold) {
    // 清空源对象的资源所有权
    other.albedoMapID = other.diffuseMapID = other.normalMapID = 0;
    other.aoMapID = other.cavityMapID = other.glossMapID = 0;
    other.metallicMapID = other.roughnessMapID = other.specularMapID = 0;
    other.fuzzMapID = other.displacementMapID = other.opacityMapID = other.alphaMapID = other.emissiveMapID = 0;
    other.hasAlbedoMap = other.hasDiffuseMap = other.hasNormalMap = false;
    other.hasAoMap = other.hasCavityMap = other.hasGlossMap = false;
    other.hasMetallicMap = other.hasRoughnessMap = other.hasSpecularMap = false;
    other.hasFuzzMap = other.hasDisplacementMap = other.hasOpacityMap = other.hasAlphaMap = other.hasEmissiveMap = false;
}

Material& Material::operator=(Material&& other) noexcept {
    if (this != &other) {
        clearAllTextures();

        name = std::move(other.name);
        albedoMapID = other.albedoMapID;
        diffuseMapID = other.diffuseMapID;
        normalMapID = other.normalMapID;
        aoMapID = other.aoMapID;
        cavityMapID = other.cavityMapID;
        glossMapID = other.glossMapID;
        metallicMapID = other.metallicMapID;
        roughnessMapID = other.roughnessMapID;
        specularMapID = other.specularMapID;
        fuzzMapID = other.fuzzMapID;
        displacementMapID = other.displacementMapID;
        opacityMapID = other.opacityMapID;
        alphaMapID = other.alphaMapID;
        emissiveMapID = other.emissiveMapID;

        hasAlbedoMap = other.hasAlbedoMap;
        hasDiffuseMap = other.hasDiffuseMap;
        hasNormalMap = other.hasNormalMap;
        hasAoMap = other.hasAoMap;
        hasCavityMap = other.hasCavityMap;
        hasGlossMap = other.hasGlossMap;
        hasMetallicMap = other.hasMetallicMap;
        hasRoughnessMap = other.hasRoughnessMap;
        hasSpecularMap = other.hasSpecularMap;
        hasFuzzMap = other.hasFuzzMap;
        hasDisplacementMap = other.hasDisplacementMap;
        hasOpacityMap = other.hasOpacityMap;
        hasAlphaMap = other.hasAlphaMap;
        hasEmissiveMap = other.hasEmissiveMap;
        emissiveColor = other.emissiveColor;
        emissiveIntensity = other.emissiveIntensity;
        isTransparent = other.isTransparent;
        alphaThreshold = other.alphaThreshold;

        // 清空源对象的资源所有权
        other.albedoMapID = other.diffuseMapID = other.normalMapID = 0;
        other.aoMapID = other.cavityMapID = other.glossMapID = 0;
        other.metallicMapID = other.roughnessMapID = other.specularMapID = 0;
        other.fuzzMapID = other.displacementMapID = other.opacityMapID = other.alphaMapID = other.emissiveMapID = 0;
        other.hasAlbedoMap = other.hasDiffuseMap = other.hasNormalMap = false;
        other.hasAoMap = other.hasCavityMap = other.hasGlossMap = false;
        other.hasMetallicMap = other.hasRoughnessMap = other.hasSpecularMap = false;
        other.hasFuzzMap = other.hasDisplacementMap = other.hasOpacityMap = other.hasAlphaMap = other.hasEmissiveMap = false;
    }
    return *this;
}

void Material::getTexturePointers(const std::string& textureType, unsigned int*& idPtr, bool*& flagPtr) {
    if (textureType == "Albedo") {
        idPtr = &albedoMapID;
        flagPtr = &hasAlbedoMap;
    } else if (textureType == "Diffuse") {
        idPtr = &diffuseMapID;
        flagPtr = &hasDiffuseMap;
    } else if (textureType == "Normal") {
        idPtr = &normalMapID;
        flagPtr = &hasNormalMap;
    } else if (textureType == "Ao") {
        idPtr = &aoMapID;
        flagPtr = &hasAoMap;
    } else if (textureType == "Cavity") {
        idPtr = &cavityMapID;
        flagPtr = &hasCavityMap;
    } else if (textureType == "Gloss") {
        idPtr = &glossMapID;
        flagPtr = &hasGlossMap;
    } else if (textureType == "Metallic") {
        idPtr = &metallicMapID;
        flagPtr = &hasMetallicMap;
    } else if (textureType == "Roughness") {
        idPtr = &roughnessMapID;
        flagPtr = &hasRoughnessMap;
    } else if (textureType == "Specular") {
        idPtr = &specularMapID;
        flagPtr = &hasSpecularMap;
    } else if (textureType == "Fuzz") {
        idPtr = &fuzzMapID;
        flagPtr = &hasFuzzMap;
    } else if (textureType == "Displacement") {
        idPtr = &displacementMapID;
        flagPtr = &hasDisplacementMap;
    } else if (textureType == "Opacity") {
        idPtr = &opacityMapID;
        flagPtr = &hasOpacityMap;
    } else if (textureType == "Alpha") {
        idPtr = &alphaMapID;
        flagPtr = &hasAlphaMap;
    } else if (textureType == "Emissive") {
        idPtr = &emissiveMapID;
        flagPtr = &hasEmissiveMap;
    } else {
        idPtr = nullptr;
        flagPtr = nullptr;
    }
}

std::string* Material::getTexturePath(const std::string& textureType) {
    if (textureType == "Albedo") return &albedoMapPath;
    else if (textureType == "Diffuse") return &diffuseMapPath;
    else if (textureType == "Normal") return &normalMapPath;
    else if (textureType == "Ao") return &aoMapPath;
    else if (textureType == "Cavity") return &cavityMapPath;
    else if (textureType == "Gloss") return &glossMapPath;
    else if (textureType == "Metallic") return &metallicMapPath;
    else if (textureType == "Roughness") return &roughnessMapPath;
    else if (textureType == "Specular") return &specularMapPath;
    else if (textureType == "Fuzz") return &fuzzMapPath;
    else if (textureType == "Displacement") return &displacementMapPath;
    else if (textureType == "Opacity") return &opacityMapPath;
    else if (textureType == "Alpha") return &alphaMapPath;
    else if (textureType == "Emissive") return &emissiveMapPath;
    else return nullptr;
}

bool Material::loadTexture(const std::string& texturePath, const std::string& textureType) {
    unsigned int* idPtr = nullptr;
    bool* flagPtr = nullptr;
    getTexturePointers(textureType, idPtr, flagPtr);

    if (!idPtr || !flagPtr) {
        std::cout << "ERROR: Unknown texture type '" << textureType << "'" << std::endl;
        return false;
    }

    // 如果已有贴图，先删除
    if (*flagPtr && *idPtr != 0) {
        glDeleteTextures(1, idPtr);
        *idPtr = 0;
        *flagPtr = false;
    }

    // 加载新贴图
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char* data = stbi_load(texturePath.c_str(), &width, &height, &nrComponents, 0);

    if (data) {
        GLenum format;
        if (nrComponents == 1) format = GL_RED;
        else if (nrComponents == 3) format = GL_RGB;
        else if (nrComponents == 4) format = GL_RGBA;
        else format = GL_RGB;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);

        *idPtr = textureID;
        *flagPtr = true;

        // 保存贴图路径
        std::string* pathPtr = getTexturePath(textureType);
        if (pathPtr) {
            *pathPtr = texturePath;
        }

        // 如果加载的是Opacity或Alpha贴图，自动标记为透明材质
        if (textureType == "Opacity" || textureType == "Alpha") {
            isTransparent = true;
        }

        // std::cout << "Successfully loaded texture: " << texturePath << " (type: " << textureType << ")" << std::endl;
        return true;
    } else {
        std::cout << "Failed to load texture: " << texturePath << std::endl;
        glDeleteTextures(1, &textureID);
        return false;
    }
}

// 删除指定类型的贴图
void Material::removeTexture(const std::string& textureType) {
    unsigned int* idPtr = nullptr;
    bool* flagPtr = nullptr;
    getTexturePointers(textureType, idPtr, flagPtr);

    if (idPtr && flagPtr) {
        // 删除OpenGL纹理
        if (*idPtr != 0) {
            glDeleteTextures(1, idPtr);
            *idPtr = 0;
        }

        // 清除标志
        *flagPtr = false;

        // 清除路径
        std::string* pathPtr = getTexturePath(textureType);
        if (pathPtr) {
            pathPtr->clear();
        }

        // 如果删除的是Opacity或Alpha贴图，检查是否还需要透明标记
        if (textureType == "Opacity" || textureType == "Alpha") {
            // 如果两个透明贴图都没有了，取消透明标记
            if (!hasOpacityMap && !hasAlphaMap) {
                isTransparent = false;
            }
        }
    }
}

void Material::clearAllTextures() {
    if (hasAlbedoMap && albedoMapID != 0) glDeleteTextures(1, &albedoMapID);
    if (hasDiffuseMap && diffuseMapID != 0) glDeleteTextures(1, &diffuseMapID);
    if (hasNormalMap && normalMapID != 0) glDeleteTextures(1, &normalMapID);
    if (hasAoMap && aoMapID != 0) glDeleteTextures(1, &aoMapID);
    if (hasCavityMap && cavityMapID != 0) glDeleteTextures(1, &cavityMapID);
    if (hasGlossMap && glossMapID != 0) glDeleteTextures(1, &glossMapID);
    if (hasMetallicMap && metallicMapID != 0) glDeleteTextures(1, &metallicMapID);
    if (hasRoughnessMap && roughnessMapID != 0) glDeleteTextures(1, &roughnessMapID);
    if (hasSpecularMap && specularMapID != 0) glDeleteTextures(1, &specularMapID);
    if (hasFuzzMap && fuzzMapID != 0) glDeleteTextures(1, &fuzzMapID);
    if (hasDisplacementMap && displacementMapID != 0) glDeleteTextures(1, &displacementMapID);
    if (hasOpacityMap && opacityMapID != 0) glDeleteTextures(1, &opacityMapID);
    if (hasAlphaMap && alphaMapID != 0) glDeleteTextures(1, &alphaMapID);
    if (hasEmissiveMap && emissiveMapID != 0) glDeleteTextures(1, &emissiveMapID);

    albedoMapID = diffuseMapID = normalMapID = 0;
    aoMapID = cavityMapID = glossMapID = 0;
    metallicMapID = roughnessMapID = specularMapID = 0;
    fuzzMapID = displacementMapID = opacityMapID = alphaMapID = emissiveMapID = 0;

    hasAlbedoMap = hasDiffuseMap = hasNormalMap = false;
    hasAoMap = hasCavityMap = hasGlossMap = false;
    hasMetallicMap = hasRoughnessMap = hasSpecularMap = false;
    hasFuzzMap = hasDisplacementMap = hasOpacityMap = hasAlphaMap = hasEmissiveMap = false;
}

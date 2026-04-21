#pragma once
#include <glm/glm.hpp>

// 光照模型枚举
enum class LightingModel {
    BLINN_PHONG = 0,
    PHONG = 1,
    PBR = 2
};

struct PointLight {
    int id;
    glm::vec3 position;
    glm::vec3 color;
    float intensity;  // 单位: 相对光通量 (在1米距离产生的照度)

    PointLight(int lightID = 0,
               glm::vec3 pos = glm::vec3(0.0f, 5.0f, 0.0f),
               glm::vec3 col = glm::vec3(1.0f),
               float inten = 50.0f)  // 默认50，适合5米高度的光源
        : id(lightID), position(pos), color(col), intensity(inten) {}
};

#pragma once

#include <glm/glm.hpp>
#include <array>

// 平面结构：用于表示视锥体的六个平面
struct Plane {
    glm::vec3 normal;   // 平面法向量
    float distance;     // 平面到原点的距离

    Plane() : normal(0.0f), distance(0.0f) {}

    // 计算点到平面的有符号距离
    float getSignedDistance(const glm::vec3& point) const {
        return glm::dot(normal, point) + distance;
    }
};

// 轴对齐包围盒（AABB）
struct AABB {
    glm::vec3 min;  // 最小点
    glm::vec3 max;  // 最大点

    AABB() : min(0.0f), max(0.0f) {}
    AABB(const glm::vec3& min, const glm::vec3& max) : min(min), max(max) {}

    // 获取中心点
    glm::vec3 getCenter() const {
        return (min + max) * 0.5f;
    }

    // 获取半径（从中心到角的距离）
    float getRadius() const {
        return glm::length(max - min) * 0.5f;
    }

    // 获取正半轴范围（用于更精确的剔除检测）
    glm::vec3 getPositiveExtent() const {
        return (max - min) * 0.5f;
    }
};

// 视锥体类
class Frustum {
public:
    // 视锥体的六个平面：左、右、下、上、近、远
    enum Side {
        LEFT = 0,
        RIGHT,
        BOTTOM,
        TOP,
        NEAR,
        FAR
    };

    std::array<Plane, 6> planes;

    // 从投影矩阵和视图矩阵构建视锥体
    void update(const glm::mat4& projectionMatrix, const glm::mat4& viewMatrix);

    // 检测点是否在视锥体内
    bool isPointInside(const glm::vec3& point) const;

    // 检测球体是否在视锥体内（用于快速剔除）
    bool isSphereInside(const glm::vec3& center, float radius) const;

    // 检测AABB是否在视锥体内（更精确的剔除检测）
    bool isAABBInside(const AABB& aabb) const;

private:
    // 标准化平面（确保法向量是单位向量）
    void normalizePlane(Plane& plane);
};

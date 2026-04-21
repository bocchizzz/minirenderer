#include "Frustum.h"

void Frustum::update(const glm::mat4& projectionMatrix, const glm::mat4& viewMatrix) {
    // 计算视图投影矩阵
    glm::mat4 vpMatrix = projectionMatrix * viewMatrix;

    // 提取视锥体的六个平面
    // 左平面
    planes[LEFT].normal.x = vpMatrix[0][3] + vpMatrix[0][0];
    planes[LEFT].normal.y = vpMatrix[1][3] + vpMatrix[1][0];
    planes[LEFT].normal.z = vpMatrix[2][3] + vpMatrix[2][0];
    planes[LEFT].distance = vpMatrix[3][3] + vpMatrix[3][0];
    normalizePlane(planes[LEFT]);

    // 右平面
    planes[RIGHT].normal.x = vpMatrix[0][3] - vpMatrix[0][0];
    planes[RIGHT].normal.y = vpMatrix[1][3] - vpMatrix[1][0];
    planes[RIGHT].normal.z = vpMatrix[2][3] - vpMatrix[2][0];
    planes[RIGHT].distance = vpMatrix[3][3] - vpMatrix[3][0];
    normalizePlane(planes[RIGHT]);

    // 下平面
    planes[BOTTOM].normal.x = vpMatrix[0][3] + vpMatrix[0][1];
    planes[BOTTOM].normal.y = vpMatrix[1][3] + vpMatrix[1][1];
    planes[BOTTOM].normal.z = vpMatrix[2][3] + vpMatrix[2][1];
    planes[BOTTOM].distance = vpMatrix[3][3] + vpMatrix[3][1];
    normalizePlane(planes[BOTTOM]);

    // 上平面
    planes[TOP].normal.x = vpMatrix[0][3] - vpMatrix[0][1];
    planes[TOP].normal.y = vpMatrix[1][3] - vpMatrix[1][1];
    planes[TOP].normal.z = vpMatrix[2][3] - vpMatrix[2][1];
    planes[TOP].distance = vpMatrix[3][3] - vpMatrix[3][1];
    normalizePlane(planes[TOP]);

    // 近平面
    planes[NEAR].normal.x = vpMatrix[0][3] + vpMatrix[0][2];
    planes[NEAR].normal.y = vpMatrix[1][3] + vpMatrix[1][2];
    planes[NEAR].normal.z = vpMatrix[2][3] + vpMatrix[2][2];
    planes[NEAR].distance = vpMatrix[3][3] + vpMatrix[3][2];
    normalizePlane(planes[NEAR]);

    // 远平面
    planes[FAR].normal.x = vpMatrix[0][3] - vpMatrix[0][2];
    planes[FAR].normal.y = vpMatrix[1][3] - vpMatrix[1][2];
    planes[FAR].normal.z = vpMatrix[2][3] - vpMatrix[2][2];
    planes[FAR].distance = vpMatrix[3][3] - vpMatrix[3][2];
    normalizePlane(planes[FAR]);
}

void Frustum::normalizePlane(Plane& plane) {
    float length = glm::length(plane.normal);
    if (length > 0.0f) {
        plane.normal /= length;
        plane.distance /= length;
    }
}

bool Frustum::isPointInside(const glm::vec3& point) const {
    // 检查点是否在所有平面的正面
    for (const auto& plane : planes) {
        if (plane.getSignedDistance(point) < 0.0f) {
            return false;
        }
    }
    return true;
}

bool Frustum::isSphereInside(const glm::vec3& center, float radius) const {
    // 检查球体是否与所有平面相交或在正面
    for (const auto& plane : planes) {
        float distance = plane.getSignedDistance(center);
        if (distance < -radius) {
            // 球体完全在平面的负面，被剔除
            return false;
        }
    }
    return true;
}

bool Frustum::isAABBInside(const AABB& aabb) const {
    // 使用更精确的AABB-平面相交测试
    glm::vec3 center = aabb.getCenter();
    glm::vec3 extent = aabb.getPositiveExtent();

    for (const auto& plane : planes) {
        // 计算AABB在平面法向量上的投影半径
        float r = extent.x * glm::abs(plane.normal.x) +
                  extent.y * glm::abs(plane.normal.y) +
                  extent.z * glm::abs(plane.normal.z);

        // 计算中心点到平面的距离
        float distance = plane.getSignedDistance(center);

        // 如果AABB完全在平面的负面，则被剔除
        if (distance < -r) {
            return false;
        }
    }
    return true;
}

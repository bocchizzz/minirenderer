#ifndef SHADER_H
#define SHADER_H

#include <vector>
#include "vector.h"


// 光源结构体：位置 + 强度
struct light
{
    vec3f position;
    vec3f intensity;
};

// Blinn-Phong 纹理着色器
// 参数：插值法线、着色点（观察空间）、uv坐标、纹理指针、光源列表、观察点位置
inline vec3f texture_fragment_shader(vec3f normal, vec3f point, vec2f uv, Texture* texture,
                               const std::vector<light>& lights, vec3f eye_pos)
{
    vec3f return_color{};
    if (texture)
    {
        return_color = texture->get_color(uv.x, uv.y);
    }
    vec3f texture_color;
    texture_color.set(return_color.x, return_color.y, return_color.z);

    // 环境光系数、漫反射系数（来自纹理）、镜面反射系数
    vec3f ka = vec3f(0.005, 0.005, 0.005);
    vec3f kd = texture_color / 255.f;
    vec3f ks = vec3f(0.7937, 0.7937, 0.7937);

    vec3f amb_light_intensity{10, 10, 10};

    // Blinn-Phong 高光指数
    float p = 150;

    vec3f result_color = {0, 0, 0};

    // 遍历所有光源，累加每个光源的 Blinn-Phong 光照贡献
    for (auto& l : lights)
    {
        auto light_dir = (l.position - point).normalized();
        auto eye_dir   = (eye_pos - point).normalized();
        auto h         = (light_dir + eye_dir).normalized();
        auto dis_sqr   = (l.position - point).dot(l.position - point);

        // 环境光分量
        auto la = ka * amb_light_intensity;
        // 漫反射分量
        auto ld = kd * (l.intensity / dis_sqr) * std::max(0.f, normal.dot(light_dir));
        // 镜面反射分量
        auto ls = ks * (l.intensity / dis_sqr) * std::pow(std::max(0.f, normal.dot(h)), p);

        result_color = result_color + la + ld + ls;
    }
    return result_color * 255.f;
}

#endif

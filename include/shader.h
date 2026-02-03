#ifndef SHADER_H
#define SHADER_H

#include <vector>
#include "vector.h"


struct light
{
    vec3f position;
    vec3f intensity;
};

vec3f texture_fragment_shader(vec3f normal, vec3f point, vec2f uv, Texture* texture)
{
    vec3f return_color{};
    if (texture)
    {
        return_color = texture->get_color(uv.x, uv.y);
    }
    vec3f texture_color;
    texture_color.set(return_color.x, return_color.y, return_color.z);

    vec3f ka = vec3f(0.005, 0.005, 0.005);
    vec3f kd = texture_color / 255.f;
    vec3f ks = vec3f(0.7937, 0.7937, 0.7937);

    auto l1 = light{{20, 20, 20}, {500, 500, 500}};
    auto l2 = light{{-20, 20, 0}, {500, 500, 500}};

    std::vector<light> lights = {l1, l2};
    vec3f amb_light_intensity{10, 10, 10};
    vec3f eye_pos{0, 0, 10};

    float p = 150;

    vec3f color = texture_color;

    vec3f result_color = {0, 0, 0};

    for (auto& light : lights)
    {
        auto light_dir = (light.position - point).normalized();
        auto eye_dir = (eye_pos - point).normalized();
        auto h = (light_dir + eye_dir).normalized();
        auto dis_sqr = (light.position - point).dot(light.position - point);

        auto la = ka * amb_light_intensity;
        auto ld = kd * (light.intensity / dis_sqr) * std::max(0.f, normal.dot(light_dir));
        auto ls = ks * (light.intensity / dis_sqr) * std::pow(std::max(0.f, normal.dot(h)), p);

        result_color = result_color + la + ld + ls;
    }
    return result_color * 255.f;
}

#endif
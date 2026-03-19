#ifndef RASTERIZER_HPP
#define RASTERIZER_HPP

#include <vector>
#include <tuple>
#include <functional>
#include <thread>
#include <mutex>
#include "matrix.h"
#include "vector.h"
#include "geometry.h"
#include "shader.h"

class Rasterizer
{
private:
    int width;
    int height;
    mat4d projection;
    mat4d view;
    mat4d model;
    // 观察点位置，用于传递给 fragment shader
    vec3f eye_pos;
    // 最大线程数，用于多线程三角形光栅化
    int max_threads;
public:
    std::vector<vec3f> framebuffer;
    std::vector<float> zbuffer;
    // 光源列表，在渲染前通过 set_lights 设定
    std::vector<light> lights;
    std::function<vec3f(vec3f, vec3f, vec2f, Texture*, const std::vector<light>&, vec3f)> fragment_shader;

    Rasterizer(int w, int h);
    // 清空帧缓冲和深度缓冲，用于每帧重新渲染
    void clear();
    void set_view(const mat4d& v);
    void set_model(const mat4d& m);
    void set_projection(const mat4d& p);
    void set_eye_pos(const vec3f& pos);
    void set_lights(const std::vector<light>& ls);
    void set_max_threads(int n);
    void set_fragement_shader(std::function<vec3f(vec3f, vec3f, vec2f, Texture*, const std::vector<light>&, vec3f)> func);
    void transfrom(mat4d& mvp, Vertex& v);
    std::tuple<float, float, float> computeBarycentric2D(float x, float y, const std::vector<Vertex>& v);
    vec3f interpolate(float alpha, float beta, float gamma, const vec3f& vert1, const vec3f& vert2, const vec3f& vert3);
    vec2f interpolate(float alpha, float beta, float gamma, const vec2f& vert1, const vec2f& vert2, const vec2f& vert3);
    void rasterizer_triangel(const Triangle& tri);
    // 多线程渲染：将三角形列表分片给多个线程并行光栅化，最后合并结果
    void render_multithreaded(const std::vector<Triangle>& triangles);
};

#endif

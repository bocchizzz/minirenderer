#include <math.h>
#include <iostream>
#include <algorithm>
#include "rasterizer.hpp"


Rasterizer::Rasterizer(int w, int h)
{
    width = w;
    height = h;
    max_threads = 16;
    zbuffer.resize(w*h, std::numeric_limits<float>::infinity());
    framebuffer.resize(w*h, vec3f());
}

void Rasterizer::clear()
{
    std::fill(zbuffer.begin(), zbuffer.end(), std::numeric_limits<float>::infinity());
    std::fill(framebuffer.begin(), framebuffer.end(), vec3f());
}

void Rasterizer::set_model(const mat4d& m)
{
    model = m;
}

void Rasterizer::set_view(const mat4d& v)
{
    view = v;
}

void Rasterizer::set_projection(const mat4d& p)
{
    projection = p;
}

void Rasterizer::set_eye_pos(const vec3f& pos)
{
    eye_pos = pos;
}

void Rasterizer::set_lights(const std::vector<light>& ls)
{
    lights = ls;
}

void Rasterizer::set_max_threads(int n)
{
    max_threads = n;
}

void Rasterizer::set_fragement_shader(std::function<vec3f(vec3f, vec3f, vec2f, Texture*, const std::vector<light>&, vec3f)> func)
{
    fragment_shader = func;
}

void Rasterizer::transfrom(mat4d& mvp, Vertex& v)
{
    float f1 = (50 - 0.1) / 2.0;
    float f2 = (50 + 0.1) / 2.0;

    v.position = mvp * v.position;

    v.position.x /= v.position.w;
    v.position.y /= v.position.w;
    v.position.z /= v.position.w;

    v.position.x = 0.5 * width * (v.position.x + 1.0);
    v.position.y = 0.5 * height * (v.position.y + 1.0);
    v.position.z = v.position.z * f1 + f2;

    mat4d inv_model = (view * model).inverse().transpose();
    vec4f n = v.norm.to_vec4(0.0);
    n = inv_model * n;
    v.norm.x = n.x;
    v.norm.y = n.y;
    v.norm.z = n.z;
    v.norm.normalize();
}

std::tuple<float, float, float> Rasterizer::computeBarycentric2D(float x, float y, const std::vector<Vertex>& v)
{
    float c1 = (x*(v[1].position.y - v[2].position.y) + (v[2].position.x - v[1].position.x)*y + v[1].position.x*v[2].position.y - v[2].position.x*v[1].position.y)
        / (v[0].position.x*(v[1].position.y - v[2].position.y) + (v[2].position.x - v[1].position.x)*v[0].position.y + v[1].position.x*v[2].position.y - v[2].position.x*v[1].position.y);

    float c2 = (x*(v[2].position.y - v[0].position.y) + (v[0].position.x - v[2].position.x)*y + v[2].position.x*v[0].position.y - v[0].position.x*v[2].position.y)
        / (v[1].position.x*(v[2].position.y - v[0].position.y) + (v[0].position.x - v[2].position.x)*v[1].position.y + v[2].position.x*v[0].position.y - v[0].position.x*v[2].position.y);

    float c3 = (x*(v[0].position.y - v[1].position.y) + (v[1].position.x - v[0].position.x)*y + v[0].position.x*v[1].position.y - v[1].position.x*v[0].position.y)
        / (v[2].position.x*(v[0].position.y - v[1].position.y) + (v[1].position.x - v[0].position.x)*v[2].position.y + v[0].position.x*v[1].position.y - v[1].position.x*v[0].position.y);

    return {c1,c2,c3};
}

vec3f Rasterizer::interpolate(float alpha, float beta, float gamma, const vec3f& vert1, const vec3f& vert2, const vec3f& vert3)
{
    return vert1*alpha + vert2*beta + vert3*gamma;
}
vec2f Rasterizer::interpolate(float alpha, float beta, float gamma, const vec2f& vert1, const vec2f& vert2, const vec2f& vert3)
{
    return vert1*alpha + vert2*beta + vert3*gamma;
}


void Rasterizer::rasterizer_triangel(const Triangle& tri)
{
    std::vector<Vertex> vertices = {tri.v1, tri.v2, tri.v3};
    std::vector<vec3f> shading_points;
    mat4d mvp = projection * view * model;
    mat4d mv = view * model;

    for (auto& v: vertices)
    {
        vec4f shading = mv * v.position;
        shading_points.push_back(vec3f(shading.x, shading.y, shading.z));
        transfrom(mvp, v);
    }

    float x_min = std::min(vertices[0].position.x, std::min(vertices[1].position.x, vertices[2].position.x));
    float x_max = std::max(vertices[0].position.x, std::max(vertices[1].position.x, vertices[2].position.x));
    float y_min = std::min(vertices[0].position.y, std::min(vertices[1].position.y, vertices[2].position.y));
    float y_max = std::max(vertices[0].position.y, std::max(vertices[1].position.y, vertices[2].position.y));

    //遍历三角形的最小包围盒的像素点，通过重心坐标判断是否在三角形内，如果是就着色
    for (int x=x_min;x<x_max;++x)
    {
        for (int y=y_min;y<y_max;++y)
        {
            float p_x = static_cast<float>(x) + 0.5;
            float p_y = static_cast<float>(y) + 0.5;
            auto[alpha, beta, gamma] = computeBarycentric2D(p_x, p_y, vertices);
            if (!(alpha >= 0 && beta >= 0 && gamma >= 0)) continue;

            float w_reciprocal = 1.0/(alpha / vertices[0].position.w + beta / vertices[1].position.w + gamma / vertices[2].position.w);
            float z_interpolated = alpha * vertices[0].position.z / vertices[0].position.w + beta * vertices[1].position.z / vertices[1].position.w + gamma * vertices[2].position.z / vertices[2].position.w;
            z_interpolated *= w_reciprocal;

            int index = (height-y)*width + x;
            if (zbuffer[index] > z_interpolated)
            {
                zbuffer[index] = z_interpolated;
                //该像素点在该模型上插值出的坐标、法线、uv
                vec2f interpolated_uv = interpolate(alpha, beta, gamma, vertices[0].tex_uv, vertices[1].tex_uv, vertices[2].tex_uv);
                vec3f shading_point = interpolate(alpha, beta, gamma, shading_points[0], shading_points[1], shading_points[2]);
                vec3f interpolated_normal = interpolate(alpha, beta, gamma, vertices[0].norm, vertices[1].norm, vertices[2].norm);
                interpolated_normal.normalize();

                auto color = fragment_shader(interpolated_normal, shading_point, interpolated_uv, tri.tex, lights, eye_pos);//着色

                framebuffer[index] = color;
            }
        }
    }
    return;
}

void Rasterizer::render_multithreaded(const std::vector<Triangle>& triangles)
{
    int tri_count = static_cast<int>(triangles.size());
    int thread_count = std::min(max_threads, tri_count);
    if (thread_count <= 1)
    {
        // 三角形太少，单线程直接渲染
        for (auto& tri : triangles) rasterizer_triangel(tri);
        return;
    }

    // 每个线程拥���独立的 framebuffer 和 zbuffer，避免竞争
    struct ThreadBuffer
    {
        std::vector<vec3f> fb;
        std::vector<float> zb;
    };
    std::vector<ThreadBuffer> thread_buffers(thread_count);
    for (int t=0;t<thread_count;++t)
    {
        thread_buffers[t].fb.resize(width*height, vec3f());
        thread_buffers[t].zb.resize(width*height, std::numeric_limits<float>::infinity());
    }

    // 线程工作函数：光栅化一段三角形到线程私有 buffer
    auto worker = [&](int thread_id, int start, int end)
    {
        auto& local_fb = thread_buffers[thread_id].fb;
        auto& local_zb = thread_buffers[thread_id].zb;

        mat4d mvp = projection * view * model;
        mat4d mv = view * model;
        mat4d inv_model = (view * model).inverse().transpose();

        for (int i=start;i<end;++i)
        {
            const Triangle& tri = triangles[i];
            std::vector<Vertex> vertices = {tri.v1, tri.v2, tri.v3};
            std::vector<vec3f> shading_points;

            for (auto& v: vertices)
            {
                vec4f shading = mv * v.position;
                shading_points.push_back(vec3f(shading.x, shading.y, shading.z));

                // 内联 transfrom 逻辑，避免访问共享成员
                float f1 = (50 - 0.1) / 2.0;
                float f2 = (50 + 0.1) / 2.0;
                v.position = mvp * v.position;
                v.position.x /= v.position.w;
                v.position.y /= v.position.w;
                v.position.z /= v.position.w;
                v.position.x = 0.5 * width * (v.position.x + 1.0);
                v.position.y = 0.5 * height * (v.position.y + 1.0);
                v.position.z = v.position.z * f1 + f2;

                vec4f n = v.norm.to_vec4(0.0);
                n = inv_model * n;
                v.norm.x = n.x;
                v.norm.y = n.y;
                v.norm.z = n.z;
                v.norm.normalize();
            }

            float x_min = std::min(vertices[0].position.x, std::min(vertices[1].position.x, vertices[2].position.x));
            float x_max = std::max(vertices[0].position.x, std::max(vertices[1].position.x, vertices[2].position.x));
            float y_min = std::min(vertices[0].position.y, std::min(vertices[1].position.y, vertices[2].position.y));
            float y_max = std::max(vertices[0].position.y, std::max(vertices[1].position.y, vertices[2].position.y));

            for (int x=x_min;x<x_max;++x)
            {
                for (int y=y_min;y<y_max;++y)
                {
                    float p_x = static_cast<float>(x) + 0.5;
                    float p_y = static_cast<float>(y) + 0.5;

                    // 重心坐标计算
                    float c1 = (p_x*(vertices[1].position.y - vertices[2].position.y) + (vertices[2].position.x - vertices[1].position.x)*p_y + vertices[1].position.x*vertices[2].position.y - vertices[2].position.x*vertices[1].position.y)
                        / (vertices[0].position.x*(vertices[1].position.y - vertices[2].position.y) + (vertices[2].position.x - vertices[1].position.x)*vertices[0].position.y + vertices[1].position.x*vertices[2].position.y - vertices[2].position.x*vertices[1].position.y);
                    float c2 = (p_x*(vertices[2].position.y - vertices[0].position.y) + (vertices[0].position.x - vertices[2].position.x)*p_y + vertices[2].position.x*vertices[0].position.y - vertices[0].position.x*vertices[2].position.y)
                        / (vertices[1].position.x*(vertices[2].position.y - vertices[0].position.y) + (vertices[0].position.x - vertices[2].position.x)*vertices[1].position.y + vertices[2].position.x*vertices[0].position.y - vertices[0].position.x*vertices[2].position.y);
                    float c3 = (p_x*(vertices[0].position.y - vertices[1].position.y) + (vertices[1].position.x - vertices[0].position.x)*p_y + vertices[0].position.x*vertices[1].position.y - vertices[1].position.x*vertices[0].position.y)
                        / (vertices[2].position.x*(vertices[0].position.y - vertices[1].position.y) + (vertices[1].position.x - vertices[0].position.x)*vertices[2].position.y + vertices[0].position.x*vertices[1].position.y - vertices[1].position.x*vertices[0].position.y);

                    if (!(c1 >= 0 && c2 >= 0 && c3 >= 0)) continue;

                    float w_reciprocal = 1.0/(c1 / vertices[0].position.w + c2 / vertices[1].position.w + c3 / vertices[2].position.w);
                    float z_interpolated = c1 * vertices[0].position.z / vertices[0].position.w + c2 * vertices[1].position.z / vertices[1].position.w + c3 * vertices[2].position.z / vertices[2].position.w;
                    z_interpolated *= w_reciprocal;

                    int index = (height-y)*width + x;
                    if (index < 0 || index >= width*height) continue;

                    if (local_zb[index] > z_interpolated)
                    {
                        local_zb[index] = z_interpolated;
                        vec2f interpolated_uv = vertices[0].tex_uv*c1 + vertices[1].tex_uv*c2 + vertices[2].tex_uv*c3;
                        vec3f shading_point = shading_points[0]*c1 + shading_points[1]*c2 + shading_points[2]*c3;
                        vec3f interpolated_normal = vertices[0].norm*c1 + vertices[1].norm*c2 + vertices[2].norm*c3;
                        interpolated_normal.normalize();

                        auto color = fragment_shader(interpolated_normal, shading_point, interpolated_uv, tri.tex, lights, eye_pos);
                        local_fb[index] = color;
                    }
                }
            }
        }
    };

    // 将三角形均匀分配给各线程
    std::vector<std::thread> threads;
    int base_count = tri_count / thread_count;
    int remainder  = tri_count % thread_count;
    int start = 0;
    for (int t=0;t<thread_count;++t)
    {
        int count = base_count + (t < remainder ? 1 : 0);
        int end = start + count;
        threads.emplace_back(worker, t, start, end);
        start = end;
    }
    for (auto& th : threads) th.join();

    // 合并各线程的 buffer：取深度最小的像素写入主 buffer
    for (int i=0;i<width*height;++i)
    {
        for (int t=0;t<thread_count;++t)
        {
            if (thread_buffers[t].zb[i] < zbuffer[i])
            {
                zbuffer[i] = thread_buffers[t].zb[i];
                framebuffer[i] = thread_buffers[t].fb[i];
            }
        }
    }
}

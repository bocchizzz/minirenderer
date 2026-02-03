#include <math.h>
#include <iostream>
#include "rasterizer.hpp"


Rasterizer::Rasterizer(int w, int h)
{
    width = w;
    height = h;
    zbuffer.resize(w*h, std::numeric_limits<float>::infinity());
    framebuffer.resize(w*h, vec3f());
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

void Rasterizer::set_fragement_shader(std::function<vec3f(vec3f, vec3f, vec2f, Texture*)> func)
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
                
                auto color = fragment_shader(interpolated_normal, shading_point, interpolated_uv, tri.tex);//着色
                
                framebuffer[index] = color;
            }
        }
    }
    return;
}
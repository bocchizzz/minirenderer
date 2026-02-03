#ifndef RASTERIZER_HPP
#define RASTERIZER_HPP

#include <vector>
#include <tuple>
#include <functional>
#include "matrix.h"
#include "vector.h"
#include "geometry.h"

class Rasterizer
{
private:
    int width;
    int height;
    mat4d projection;
    mat4d view;
    mat4d model;
public:
    std::vector<vec3f> framebuffer;
    std::vector<float> zbuffer;
    std::function<vec3f(vec3f, vec3f, vec2f, Texture*)> fragment_shader;

    Rasterizer(int w, int h);
    void set_view(const mat4d& v);
    void set_model(const mat4d& m);
    void set_projection(const mat4d& p);
    void set_fragement_shader(std::function<vec3f(vec3f, vec3f, vec2f, Texture*)> func);
    void transfrom(mat4d& mvp, Vertex& v);
    std::tuple<float, float, float> computeBarycentric2D(float x, float y, const std::vector<Vertex>& v);
    vec3f interpolate(float alpha, float beta, float gamma, const vec3f& vert1, const vec3f& vert2, const vec3f& vert3);
    vec2f interpolate(float alpha, float beta, float gamma, const vec2f& vert1, const vec2f& vert2, const vec2f& vert3);
    void rasterizer_triangel(const Triangle& tri);
};

#endif
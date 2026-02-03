#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include "vector.h"
#include "matrix.h"
#include "geometry.h"
#include "load_model.hpp"
#include "rasterizer.hpp"
#include "shader.h"

#define MY_PI 3.1415926
#define TWO_PI (2.0* MY_PI)

void saveAsPPM(int w, int h, const std::string filename, const std::vector<vec3f>& framebuffer)
{
    std::ofstream ofs(filename);
    if (!ofs) {
        throw std::runtime_error("Could not open file for writing");
    }

    // P3: 格式, 3 3: 宽和高, 255: 最大颜色深度
    ofs << "P3\n"<<w<<" "<<h<<"\n255\n";

    // 2. 写入像素数据
    for (int i = 0; i < framebuffer.size(); ++i) {
            vec3f color = {static_cast<int>(framebuffer[i].x), static_cast<int>(framebuffer[i].y), static_cast<int>(framebuffer[i].z)};

            // 写入 R G B 三个通道（对于灰度图，三个值相同）
            ofs << color[0] << " " << color[1] << " " << color[2] << "  ";
    }

    ofs.close();
    std::cout << "Image saved to " << filename << std::endl;
}

mat4d get_view_matrix(vec3f eye_pos)
{
    mat4d view{};
    view << 1,0,0,0,
            0,1,0,0,
            0,0,1,0,
            0,0,0,1;

    mat4d translate{};
    translate << 1,0,0,-eye_pos[0],
                 0,1,0,-eye_pos[1],
                 0,0,1,-eye_pos[2],
                 0,0,0,1;

    view = translate*view;

    return view;
}

mat4d get_model_matrix(float angle)
{
    mat4d rotation{};
    angle = angle * MY_PI / 180.f;
    rotation << cos(angle), 0, sin(angle), 0,
                0, 1, 0, 0,
                -sin(angle), 0, cos(angle), 0,
                0, 0, 0, 1;

    mat4d scale{};
    scale << 2.5, 0, 0, 0,
              0, 2.5, 0, 0,
              0, 0, 2.5, 0,
              0, 0, 0, 1;

    mat4d translate{};
    translate << 1, 0, 0, 0,
            0, 1, 0, 0,
            0, 0, 1, 0,
            0, 0, 0, 1;

    return translate * rotation * scale;
}

mat4d get_projection_matrix(float eye_fov, float aspect_ratio, float zNear, float zFar)
{
    mat4d projection{};
    projection<< 1,0,0,0,
                0,1,0,0,
                0,0,1,0,
                0,0,0,1;
    float fov = eye_fov / 180.f * MY_PI;

    //创建投影矩阵
    mat4d mp{};
    mp << zNear, 0.f, 0.f, 0.f,
        0.f, zNear, 0.f, 0.f,
        0.f, 0.f, (zNear+zFar), -zNear*zFar,
        0.f, 0.f, 1.f, 0.f;

    //创建平移矩阵
    mat4d mt{};
    mt << 1.f, 0.f, 0.f, 0.f,
        0.f, 1.f, 0.f, 0.f,
        0.f, 0.f, 1.f, -(zNear+zFar)/2.f,
        0.f, 0.f, 0.f, 1.f;

    //创建缩放矩阵
    mat4d ms;
    ms << -1.f / (std::tan(fov/2.f) * zNear * aspect_ratio), 0.f, 0.f, 0.f,
        0.f, -1.f / (std::tan(fov/2.f) * zNear), 0.f, 0.f,
        0.f, 0.f, 2.f / (zNear-zFar), 0.f,
        0.f, 0.f, 0.f, 1.f;

    projection = ms * mt * mp * projection;

    return projection;

}

int main()
{
    Model m = {};
    std::string obj_path = "../models/spot/spot_triangulated_good.obj";
    bool f = m.load_model(obj_path);
    vec3f v1 = vec3f(1.f, 2.f, 3.f);
    vec3f v2 = vec3f(2.f, 2.f, 2.f);
    vec3f v3 = v1 + v2;
    vec3f v4 = v1 - v2;
    vec3f v5 = v4.normalized();

    float angle = 140.0;
    vec3f eye_pos{0.0, 0.0, 10.0};
    int width = 700;
    int height = 700;

    Rasterizer* rasterizer = new Rasterizer(width, height);
    rasterizer->set_model(get_model_matrix(angle));
    rasterizer->set_view(get_view_matrix(eye_pos));
    rasterizer->set_projection(get_projection_matrix(45.0, 1, 0.1, 50));
    rasterizer->set_fragement_shader(texture_fragment_shader);

    Texture* texture = new Texture("../models/spot/spot_texture copy.png");

    for (int i=0;i<m.get_face_count();++i)
    {
        Triangle tri = {};
        tri.tex = texture;
        tri.set_vertex(m.get_vertex(i, 0), m.get_vertex(i, 1), m.get_vertex(i, 2));
        rasterizer->rasterizer_triangel(tri);

    }
    std::cout<<"Rasterizer over!!!!!!!!\n";

    saveAsPPM(width, height, "output.ppm", rasterizer->framebuffer);
    return 0;
}
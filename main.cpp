#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <opencv2/opencv.hpp>
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

// 绕y轴旋转的模型矩阵
mat4d get_rotation_y(float angle)
{
    mat4d rotation{};
    angle = angle * MY_PI / 180.f;
    rotation << cos(angle), 0, sin(angle), 0,
                0, 1, 0, 0,
                -sin(angle), 0, cos(angle), 0,
                0, 0, 0, 1;
    return rotation;
}

// 绕z轴旋转的模型矩阵
mat4d get_rotation_z(float angle)
{
    mat4d rotation{};
    angle = angle * MY_PI / 180.f;
    rotation << cos(angle), -sin(angle), 0, 0,
                sin(angle), cos(angle), 0, 0,
                0, 0, 1, 0,
                0, 0, 0, 1;
    return rotation;
}

// 组合模型矩阵：缩放 + 绕y轴旋转 + 绕z轴旋转
mat4d get_model_matrix(float angle_y, float angle_z)
{
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

    return translate * get_rotation_z(angle_z) * get_rotation_y(angle_y) * scale;
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

    // 绕y轴和z轴的旋转角度，通过键盘交互修改
    float angle_y = 140.0;
    float angle_z = 0.0;
    vec3f eye_pos{0.0, 0.0, 10.0};
    int width = 700;
    int height = 700;

    // 在渲染前设定多光源，可按需增减光源
    std::vector<light> lights = {
        light{{20, 20, 20},   {300, 300, 300}},  // 右上前方光源
        light{{-20, 20, 0},   {300, 300, 300}},  // 左上方光源
        light{{0, -20, 10},   {300, 300, 300}},  // 下方补光
        light{{20, 120, 20},   {300, 300, 300}},  // 右上前方光源
        light{{-20, 20, 300},   {300, 300, 300}},  // 左上方光源
        light{{50, -20, 10},   {100, 100, 100}},  // 下方补光
        light{{100, 20, 20},   {100, 100, 100}},  // 右上前方光源
        light{{-120, 220, 0},   {100, 100, 100}},  // 左上方光源
        light{{30, 40, 10},   {200, 200, 200}},  // 下方补光
    };

    Texture* texture = new Texture("../models/spot/spot_texture copy.png");

    Rasterizer* rasterizer = new Rasterizer(width, height);
    rasterizer->set_view(get_view_matrix(eye_pos));
    rasterizer->set_projection(get_projection_matrix(45.0, 1, 0.1, 50));
    rasterizer->set_eye_pos(eye_pos);
    rasterizer->set_lights(lights);
    rasterizer->set_max_threads(4);  // 最大4个线程并行计算光源着色
    rasterizer->set_fragement_shader(texture_fragment_shader);

    // 实时渲染循环，使用 OpenCV 显示并响应键盘输入
    while (true)
    {
        rasterizer->clear();
        rasterizer->set_model(get_model_matrix(angle_y, angle_z));

        // 构建三角形列表
        std::vector<Triangle> triangles;
        for (int i=0;i<m.get_face_count();++i)
        {
            Triangle tri = {};
            tri.tex = texture;
            tri.set_vertex(m.get_vertex(i, 0), m.get_vertex(i, 1), m.get_vertex(i, 2));
            triangles.push_back(tri);
        }
        // 多线程并行光栅化
        rasterizer->render_multithreaded(triangles);

        // 将 framebuffer 转换为 cv::Mat 用于显示
        cv::Mat image(height, width, CV_32FC3);
        for (int i=0;i<height;++i)
        {
            for (int j=0;j<width;++j)
            {
                int idx = i * width + j;
                // OpenCV 使用 BGR 格式
                image.at<cv::Vec3f>(i, j) = cv::Vec3f(
                    std::min(255.f, std::max(0.f, rasterizer->framebuffer[idx].z)),  // B
                    std::min(255.f, std::max(0.f, rasterizer->framebuffer[idx].y)),  // G
                    std::min(255.f, std::max(0.f, rasterizer->framebuffer[idx].x))   // R
                );
            }
        }
        image.convertTo(image, CV_8UC3);

        cv::imshow("MiniRenderer", image);

        // 等待键盘输入，30ms 超时
        int key = cv::waitKey(10);
        if (key == 27) break;  // ESC 退出
        if (key == 'a') angle_z += 5.0;   // a键：绕z轴正向旋转5°
        if (key == 'd') angle_z -= 5.0;   // d键：绕z轴反向旋转5°
        if (key == 'w') angle_y += 5.0;   // w键：绕y轴正向旋转5°
        if (key == 's') angle_y -= 5.0;   // s键：绕y轴反向旋转5°
    }

    cv::destroyAllWindows();
    delete texture;
    delete rasterizer;
    return 0;
}

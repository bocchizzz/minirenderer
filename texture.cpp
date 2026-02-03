#include "texture.hpp"

Texture::Texture(const std::string& name)
{
    image = cv::imread(name);
    cv::cvtColor(image, image, cv::COLOR_RGB2BGR);
    width = image.cols;
    height = image.rows;
}

vec3f Texture::get_color(float u, float v)
{
    auto u_img = u * width;
    auto v_img = (1 - v) * height;

    int u00 = static_cast<int>(floor(u_img));
    int u01 = static_cast<int>(ceil(u_img));
    int v00 = static_cast<int>(floor(v_img));
    int v01 = static_cast<int>(ceil(v_img));

    u00 = std::clamp(u00, 0, width-1);
    u01 = std::clamp(u01, 0, width-1);
    v00 = std::clamp(v00, 0, height-1);
    v01 = std::clamp(v01, 0, height-1);

    auto color00 = image.at<cv::Vec3b>(v00, u00);
    auto color01 = image.at<cv::Vec3b>(v01, u00);
    auto color10 = image.at<cv::Vec3b>(v00, u01);
    auto color11 = image.at<cv::Vec3b>(v01, u01);

    float u_radio = u_img - u00;
    float v_radio = v_img - v00;

    auto color0 = color10 * u_radio + color00 * (1-u_radio);
    auto color1 = color11 * u_radio + color01 * (1-u_radio);
    auto color = color0 * (1-v_radio) + color1 * v_radio;
    return vec3f(color[0], color[1], color[2]);
}
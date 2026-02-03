#ifndef TEXTURE_HPP
#define TEXTURE_HPP

#include <opencv2/opencv.hpp>
#include "vector.h"

class Texture
{
private:
    cv::Mat image;
    int width;
    int height;
public:
    Texture(const std::string& name);
    vec3f get_color(float u, float v);
};

#endif
#ifndef LOAD_MODEL_HPP
#define LOAD_MODEL_HPP

#include <vector>
#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include "geometry.h"

class Model
{
    private:
        std::vector<Vertex> vertices;      // 顶点buffer
        std::vector<int> indices; // 面buffer
        Vertex parse_vertex(const std::string& data, 
                        const std::vector<vec4f>& p, 
                        const std::vector<vec2f>& uv, 
                        const std::vector<vec3f>& n);
    public:
        
        bool load_model(const std::string& filename);
        int get_vertex_count();
        int get_face_count();
        Vertex get_vertex(int iface, int ivertex);
};
#endif
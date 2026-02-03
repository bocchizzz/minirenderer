#include <unordered_map>
#include <iostream>
#include "load_model.hpp"

bool Model::load_model(const std::string& filename) 
{
    std::ifstream file(filename);
    if (!file.is_open()) return false;

    std::vector<vec4f> temp_positions;
    std::vector<vec3f> temp_normals;
    std::vector<vec2f> temp_uvs;
    
    std::unordered_map<std::string, int> uniqueVertices;

    std::string line;
    while (std::getline(file, line))
    {
        std::stringstream ss(line);
        std::string prefix;
        ss >> prefix;

        if (prefix == "v") {
            vec4f v; ss >> v.x >> v.y >> v.z; v.w = 1.0f;
            temp_positions.push_back(v);
        } 
        else if (prefix == "vt") {
            vec2f vt; ss >> vt.x >> vt.y;
            temp_uvs.push_back(vt);
        } 
        else if (prefix == "vn") {
            vec3f vn; ss >> vn.x >> vn.y >> vn.z;
            temp_normals.push_back(vn);
        } 
        else if (prefix == "f") {
            std::string vertexStr;
            std::vector<int> faceIndices;

            while (ss >> vertexStr)
            {
                // 如果这个组合没出现过，则创建新顶点
                if (uniqueVertices.find(vertexStr) == uniqueVertices.end()) {
                    Vertex v = parse_vertex(vertexStr, temp_positions, temp_uvs, temp_normals);
                    int newIndex = static_cast<int>(vertices.size());
                    vertices.push_back(v);
                    uniqueVertices[vertexStr] = newIndex;
                }
                // 获取该组合对应的索引
                faceIndices.push_back(uniqueVertices[vertexStr]);
            }

            // 拆分为三角形
            for (size_t i = 1; i < faceIndices.size() - 1; ++i)
            {
                indices.push_back(faceIndices[0]);
                indices.push_back(faceIndices[i]);
                indices.push_back(faceIndices[i + 1]);
            }
        }
    }
    return true;
}

Vertex Model::parse_vertex(const std::string& data, const std::vector<vec4f>& p, const std::vector<vec2f>& uv, const std::vector<vec3f>& n) 
{
    Vertex v = {};
    int v_idx = 0, vt_idx = 0, vn_idx = 0;
    
    // 统一处理 v/vt/vn, v//vn, v/vt, v
    if (data.find("//") != std::string::npos) 
    {
        sscanf(data.c_str(), "%d//%d", &v_idx, &vn_idx);
    } 
    else
    {
        int count = sscanf(data.c_str(), "%d/%d/%d", &v_idx, &vt_idx, &vn_idx);
        if (count == 1) v_idx = std::stoi(data);
    }
    // OBJ 索引转换
    if (v_idx > 0) v.position = p[v_idx - 1];
    if (vt_idx > 0) v.tex_uv = uv[vt_idx - 1];
    if (vn_idx > 0) v.norm = n[vn_idx - 1];

    return v;
}

int Model::get_vertex_count()
{
    return vertices.size();
}

int Model::get_face_count()
{
    return indices.size() / 3;
}

Vertex Model::get_vertex(int iface, int ivertex)
{
    return vertices[indices[iface*3 + ivertex]];
}
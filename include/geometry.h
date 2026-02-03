#ifndef GEOMETRY_H
#define GEOMETRY_H

#include "vector.h"
#include "texture.hpp"

class Vertex
{
    public:
    vec4f position;
    vec3f norm;
    vec2f tex_uv;
};

class Triangle
{
    public:
    Vertex v1;
    Vertex v2;
    Vertex v3;
    Texture* tex;
    void set_vertex(Vertex v1, Vertex v2, Vertex v3)
    {
        this->v1 = v1;
        this->v2 = v2;
        this->v3 = v3;
    }
};
#endif
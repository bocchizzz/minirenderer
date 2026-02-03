#ifndef VECTOR_H
#define VECTOR_H

#include "r_math.h"

class vec2f
{
    public:
    float x;
    float y;
    vec2f(float x, float y): x(x), y(y){};
    vec2f(): x(0.f), y(0.f){};

    //运算符定义
    vec2f operator+(const vec2f& op2)
    {
        vec2f result{};
        result.x = this->x + op2.x;
        result.y = this->y + op2.y;
        return result;
    }
    vec2f operator+(const vec2f& op2) const
    {
        vec2f result{};
        result.x = this->x + op2.x;
        result.y = this->y + op2.y;
        return result;
    }
    vec2f operator-(const vec2f& op2)
    {
        vec2f result{};
        result.x = this->x - op2.x;
        result.y = this->y - op2.y;
        return result;
    }
    vec2f operator*(float m)
    {
        vec2f result{};
        result.x = this->x * m;
        result.y = this->y * m;
        return result;
    }
    const vec2f operator*(float m) const
    {
        vec2f result{};
        result.x = this->x * m;
        result.y = this->y * m;
        return result;
    }
    float dot(const vec2f& op2)
    {
        float result = 0.f;
        result += this->x * op2.x;
        result += this->y * op2.y;
        return result;
    }
    void normalize()
    {
        float sum = x*x + y*y;
        if (sum < 0.000001 && sum > -0.000001) return;
        float InvLen = math::InvSqrt(sum);
        this->x = this->x * InvLen;
        this->y = this->y * InvLen;
    }
    vec2f normalized()
    {
        float sum = x*x + y*y;
        vec2f result{};
        if (sum < 0.000001 && sum > -0.000001) return result;

        float InvLen = math::InvSqrt(sum);
        result.x = this->x * InvLen;
        result.y = this->y * InvLen;
        return result;
    }
};

class vec4f
{
    public:
    float x;
    float y;
    float z;
    float w;
    vec4f(float x, float y, float z, float w): x(x), y(y), z(z), w(w){};
    vec4f(): x(0.f), y(0.f), z(0.f), w(0.f){};

    //运算符定义
    vec4f operator+(const vec4f& op2)
    {
        vec4f result{};
        result.x = this->x + op2.x;
        result.y = this->y + op2.y;
        result.z = this->z + op2.z;
        result.w = this->w + op2.w;
        return result;
    }
    vec4f operator-(const vec4f& op2)
    {
        vec4f result{};
        result.x = this->x - op2.x;
        result.y = this->y - op2.y;
        result.z = this->z - op2.z;
        result.w = this->w - op2.w;
        return result;
    }
    float dot(const vec4f& op2)
    {
        float result = 0.f;
        result += this->x * op2.x;
        result += this->y * op2.y;
        result += this->z * op2.z;
        result += this->w * op2.w;
        return result;
    }
    const float& operator[](int i) const
    {
        if (i == 0) return x;
        else if (i == 1) return y;
        else if (i == 2) return z;
        else if (i == 3) return w;
    }
    float& operator[](int i)
    {
        if (i == 0) return x;
        else if (i == 1) return y;
        else if (i == 2) return z;
        else if (i == 3) return w;
    }
    void normalize()
    {
        float sum = x*x + y*y + z*z + w*w;
        if (sum < 0.000001 && sum > -0.000001) return;
        float InvLen = math::InvSqrt(sum);
        this->x = this->x * InvLen;
        this->y = this->y * InvLen;
        this->z = this->z * InvLen;
        this->w = this->w * InvLen;
    }
    vec4f normalized()
    {
        float sum = x*x + y*y + z*z + w*w;
        vec4f result{};
        if (sum < 0.000001 && sum > -0.000001) return result;
        
        float InvLen = math::InvSqrt(sum);
        result.x = this->x * InvLen;
        result.y = this->y * InvLen;
        result.z = this->z * InvLen;
        result.w = this->w * InvLen;
        return result;
    }
};

class vec3f
{
    public:
    float x;
    float y;
    float z;
    vec3f(float x, float y, float z): x(x), y(y), z(z){};
    vec3f(): x(0.f), y(0.f), z(0.f){};
    vec3f(const vec3f& v)
    {
        x = v.x;
        y = v.y;
        z = v.z;
    }

    void set(float x, float y, float z)
    {
        this->x = x;
        this->y = y;
        this->z = z;
    }

    //运算符定义
    vec3f operator+(const vec3f& op2)
    {
        vec3f result{};
        result.x = this->x + op2.x;
        result.y = this->y + op2.y;
        result.z = this->z + op2.z;
        return result;
    }
    vec3f operator+(const vec3f& op2) const
    {
        vec3f result{};
        result.x = this->x + op2.x;
        result.y = this->y + op2.y;
        result.z = this->z + op2.z;
        return result;
    }
    vec3f operator-(const vec3f& op2)
    {
        vec3f result{};
        result.x = this->x - op2.x;
        result.y = this->y - op2.y;
        result.z = this->z - op2.z;
        return result;
    }
    vec3f operator*(float m)
    {
        vec3f result{};
        result.x = this->x * m;
        result.y = this->y * m;
        result.z = this->z * m;
        return result;
    }
    vec3f operator*(float m) const
    {
        vec3f result{};
        result.x = this->x * m;
        result.y = this->y * m;
        result.z = this->z * m;
        return result;
    }
    vec3f operator*(const vec3f& op2)
    {
        vec3f result{};
        result.x = this->x * op2.x;
        result.y = this->y * op2.y;
        result.z = this->z * op2.z;
        return result;
    }
    vec3f operator*(const vec3f& op2) const
    {
        vec3f result{};
        result.x = this->x * op2.x;
        result.y = this->y * op2.y;
        result.z = this->z * op2.z;
        return result;
    }
    vec3f operator/(float m)
    {
        vec3f result{};
        result.x = this->x / m;
        result.y = this->y / m;
        result.z = this->z / m;
        return result;
    }
    vec3f operator/(float m) const
    {
        vec3f result{};
        result.x = this->x / m;
        result.y = this->y / m;
        result.z = this->z / m;
        return result;
    }
    const float& operator[](int i) const
    {
        if (i == 0) return x;
        else if (i == 1) return y;
        else if (i == 2) return z;
    }
    float& operator[](int i)
    {
        if (i == 0) return x;
        else if (i == 1) return y;
        else if (i == 2) return z;

    }
    float dot(const vec3f& op2)
    {
        float result = 0.f;
        result += this->x * op2.x;
        result += this->y * op2.y;
        result += this->z * op2.z;
        return result;
    }
    vec3f cross(const vec3f& op2)
    {
        vec3f result{};
        result.x = this->y*op2.z - this->z*op2.y;
        result.y = this->z*op2.x - this->x*op2.z;
        result.z = this->x*op2.y - this->y*op2.x;
        return result;
    }
    void normalize()
    {
        float sum = x*x + y*y + z*z;
        if (sum < 0.000001 && sum > -0.000001) return;

        float InvLen = math::InvSqrt(sum);
        this->x = this->x * InvLen;
        this->y = this->y * InvLen;
        this->z = this->z * InvLen;
    }
    vec3f normalized()
    {
        float sum = x*x + y*y + z*z;
        vec3f result{};
        if (sum < 0.000001 && sum > -0.000001) return result;

        float InvLen = math::InvSqrt(sum);
        result.x = this->x * InvLen;
        result.y = this->y * InvLen;
        result.z = this->z * InvLen;
        return result;
    }
    vec4f to_vec4(float w)
    {
      vec4f result{};
      result.x = x;
      result.y = y;
      result.z = z;
      result.w = w;
      return result;
    }
};


#endif
#ifndef MATRIX_H
#define MATRIX_H


#include <stdexcept>
#include "r_math.h"
#include "vector.h"

class mat3d
{
    public:
    double M[3][3];
    
    mat3d(double v = 0.f)
    {
        for (int i=0;i<3;++i)
        {
            for (int j=0;j<3;++j) M[i][j] = v;
        }
    }
    double* operator[](int row)
    {
        if (row < 0 || row > 2) throw std::out_of_range("Matrix row index out of bounds");
        return M[row];
    }
    const double* operator[](int row) const
    {
        if (row < 0 || row > 2) throw std::out_of_range("Matrix row index out of bounds");
        return M[row];
    }
    mat3d operator+ (const mat3d& op2)
    {
        mat3d result{};
        for (int i=0;i<3;++i)
        {
            for (int j=0;j<3;++j)
            {
                result[i][j] = this->M[i][j] + op2[i][j];
            }
        }
        return result;
    }
    mat3d operator- (const mat3d& op2)
    {
        mat3d result{};
        for (int i=0;i<3;++i)
        {
            for (int j=0;j<3;++j)
            {
                result[i][j] = this->M[i][j] - op2[i][j];
            }
        }
        return result;
    }
    mat3d operator* (const mat3d& op2)
    {
        mat3d result{};
        for (int i=0;i<3;++i)
        {
            for (int j=0;j<3;++j)
            {
                for (int k=0;k<3;++k)
                {
                    result[i][j] += this->M[i][k] * op2[k][j];
                }
                
            }
        }
        return result;
    }
    void normlize()
    {
        double sum = 0.f;
        for (int i=0;i<3;++i)
        {
            for (int j=0;j<3;++j)
            {
                sum += M[i][j] * M[i][j];
            }
        }
        if (sum < 0.000001 && sum > -0.000001) return;
        double InvLen = math::InvSqrt(sum);
        for (int i=0;i<3;++i)
        {
            for (int j=0;j<3;++j)
            {
                M[i][j] /= InvLen;
            }
        }
    }
    mat3d normlized()
    {
        double sum = 0.f;
        for (int i=0;i<3;++i)
        {
            for (int j=0;j<3;++j)
            {
                sum += M[i][j] * M[i][j];
            }
        }
        mat3d result{};
        if (sum < 0.000001 && sum > -0.000001) return result;
        double InvLen = math::InvSqrt(sum);
        for (int i=0;i<3;++i)
        {
            for (int j=0;j<3;++j)
            {
                M[i][j] /= InvLen;
            }
        }
    }

    mat3d transpose() const 
    {
        mat3d result;
        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 3; ++j) {
                result.M[j][i] = this->M[i][j];
            }
        }
        return result;
    }
    
    double determinant() const 
    {
        return M[0][0] * (M[1][1] * M[2][2] - M[1][2] * M[2][1]) -
               M[0][1] * (M[1][0] * M[2][2] - M[1][2] * M[2][0]) +
               M[0][2] * (M[1][0] * M[2][1] - M[1][1] * M[2][0]);
    }

    mat3d inverse() const 
    {
        double det = this->determinant();
        if (det < 1e-9 && det > -1e-9) 
        {
            throw std::runtime_error("Matrix is singular!");
        }

        double invDet = 1.0 / det;
        mat3d result;

        for (int i = 0; i < 3; ++i) 
        {
            for (int j = 0; j < 3; ++j) 
            {

                double subMat[2][2];
                int subRow = 0;
                for (int r = 0; r < 3; ++r) 
                {
                    if (r == i) continue;
                    
                    int subCol = 0;
                    for (int c = 0; c < 3; ++c) {
                        if (c == j) continue; 
                        subMat[subRow][subCol] = M[r][c];
                        subCol++;
                    }
                    subRow++;
                }

                double minor = subMat[0][0] * subMat[1][1] - subMat[0][1] * subMat[1][0];

                double sign = ((i + j) % 2 == 0) ? 1.0 : -1.0;

                result.M[j][i] = sign * minor * invDet;
            }
        }

        return result;
    }

};

class mat4d
{
    public:
    double M[4][4];
    
    mat4d(double v = 0.f)
    {
        for (int i=0;i<4;++i)
        {
            for (int j=0;j<4;++j) M[i][j] = v;
        }
    }
    struct CommaInitializer {
        mat4d& m_mat;
        int m_index;

        CommaInitializer(mat4d& mat, double val) : m_mat(mat), m_index(0) 
        {
            m_mat[m_index/4][m_index%4] = val;
            m_index++;
        }

        CommaInitializer& operator,(double val) 
        {
            if (m_index < 16) 
            {
                m_mat[m_index/4][m_index%4] = val;
                m_index++;
            }
            return *this;
        }
    };

    CommaInitializer operator<<(double val) {
        return CommaInitializer(*this, val);
    }


    double* operator[](int row)
    {
        if (row < 0 || row > 3) throw std::out_of_range("Matrix row index out of bounds");
        return M[row];
    }
    const double* operator[](int row) const
    {
        if (row < 0 || row > 3) throw std::out_of_range("Matrix row index out of bounds");
        return M[row];
    }
    mat4d operator+ (const mat4d& op2)
    {
        mat4d result{};
        for (int i=0;i<4;++i)
        {
            for (int j=0;j<4;++j)
            {
                result[i][j] = this->M[i][j] + op2[i][j];
            }
        }
        return result;
    }
    mat4d operator- (const mat4d& op2)
    {
        mat4d result{};
        for (int i=0;i<4;++i)
        {
            for (int j=0;j<4;++j)
            {
                result[i][j] = this->M[i][j] - op2[i][j];
            }
        }
        return result;
    }
    mat4d operator* (const mat4d& op2)
    {
        mat4d result{};
        for (int i=0;i<4;++i)
        {
            for (int j=0;j<4;++j)
            {
                for (int k=0;k<4;++k)
                {
                    result[i][j] += this->M[i][k] * op2[k][j];
                }
                
            }
        }
        return result;
    }
    vec4f operator* (const vec4f& v)
    {
        vec4f result{};
        for (int i=0;i<4;++i)
        {
            for (int j=0;j<4;++j)
            {
                result[i] += static_cast<float>(M[i][j]) * v[j];
            }
        }
        return result;
    }
    void normlize()
    {
        double sum = 0.f;
        for (int i=0;i<4;++i)
        {
            for (int j=0;j<4;++j)
            {
                sum += M[i][j] * M[i][j];
            }
        }
        if (sum < 0.000001 && sum > -0.000001) return;
        double InvLen = math::InvSqrt(sum);
        for (int i=0;i<4;++i)
        {
            for (int j=0;j<4;++j)
            {
                M[i][j] /= InvLen;
            }
        }
    }
    mat3d normlized()
    {
        double sum = 0.f;
        for (int i=0;i<4;++i)
        {
            for (int j=0;j<4;++j)
            {
                sum += M[i][j] * M[i][j];
            }
        }
        mat3d result{};
        if (sum < 0.000001 && sum > -0.000001) return result;
        double InvLen = math::InvSqrt(sum);
        for (int i=0;i<4;++i)
        {
            for (int j=0;j<4;++j)
            {
                M[i][j] /= InvLen;
            }
        }
    }

    mat4d transpose() const 
    {
        mat4d result;
        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                result.M[j][i] = this->M[i][j];
            }
        }
        return result;
    }
    
    double determinant() const 
    {
        double det = 0.0;

        for (int j = 0; j < 4; ++j) {
            double subMat[3][3];

            int subRow = 0;
            for (int r = 1; r < 4; ++r) { 
                int subCol = 0;
                for (int c = 0; c < 4; ++c) {
                    if (c == j) continue;
                    subMat[subRow][subCol] = M[r][c];
                    subCol++;
                }
                subRow++;
            }

            double minor = subMat[0][0] * (subMat[1][1] * subMat[2][2] - subMat[1][2] * subMat[2][1]) -
                        subMat[0][1] * (subMat[1][0] * subMat[2][2] - subMat[1][2] * subMat[2][0]) +
                        subMat[0][2] * (subMat[1][0] * subMat[2][1] - subMat[1][1] * subMat[2][0]);
            double sign = (j % 2 == 0) ? 1.0 : -1.0;
            det += sign * M[0][j] * minor;
        }

        return det;
    }

    mat4d inverse() const 
    {
        double det = this->determinant();
        if (det < 1e-9 && det > -1e-9) 
        {
            throw std::runtime_error("Matrix is singular!");
        }

        double invDet = 1.0 / det;
        mat4d result;

        for (int i = 0; i < 4; ++i) 
        {
            for (int j = 0; j < 4; ++j) 
            {

                double subMat[3][3];
                int subRow = 0;
                for (int r = 0; r < 4; ++r) 
                {
                    if (r == i) continue;
                    
                    int subCol = 0;
                    for (int c = 0; c < 4; ++c) {
                        if (c == j) continue; 
                        subMat[subRow][subCol] = M[r][c];
                        subCol++;
                    }
                    subRow++;
                }

                double minor = subMat[0][0] * (subMat[1][1] * subMat[2][2] - subMat[1][2] * subMat[2][1]) -
                        subMat[0][1] * (subMat[1][0] * subMat[2][2] - subMat[1][2] * subMat[2][0]) +
                        subMat[0][2] * (subMat[1][0] * subMat[2][1] - subMat[1][1] * subMat[2][0]);

                double sign = ((i + j) % 2 == 0) ? 1.0 : -1.0;

                result.M[j][i] = sign * minor * invDet;
            }
        }

        return result;
    }

};
#endif
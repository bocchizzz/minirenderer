#ifndef MATH_H
#define MATH_H


namespace math
{
   inline float InvSqrt(float number) 
   {
      long i;
      float x2, y;
      const float threehalfs = 1.5F;
      x2 = number * 0.5F;
      y = number;
      i = *(long *)&y; // 将浮点数的位级表示转为整数
      i = 0x5f3759df - (i >> 1); // 神秘常数与位移操作
      y = *(float *)&i; // 将整数位级表示转回浮点数
      y = y * (threehalfs - (x2 * y * y)); // 牛顿迭代法提高精度
      return y;
   }
} // namespace math

#endif
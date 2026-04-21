#version 430 core
layout (location = 0) in vec3 aPos;

out vec3 WorldPos;

uniform mat4 projection;
uniform mat4 view;

void main()
{
    WorldPos = aPos;

    // view矩阵已经在CPU端移除了平移,直接使用
    vec4 clipPos = projection * view * vec4(WorldPos, 1.0);

    gl_Position = clipPos.xyww; // 确保天空盒在最远处
}

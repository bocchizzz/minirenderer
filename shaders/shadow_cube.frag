#version 430 core
in vec4 FragPos;

uniform vec3 lightPos;
uniform float farPlane;

void main() {
    // 计算片元到光源的距离
    float lightDistance = length(FragPos.xyz - lightPos);

    // 映射到 [0, 1] 范围（线性深度）
    lightDistance = lightDistance / farPlane;

    // 写入深度
    gl_FragDepth = lightDistance;
}

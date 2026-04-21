#version 430 core

layout (location = 0) out vec4 gPosition;
layout (location = 1) out vec4 gNormal;
layout (location = 2) out vec4 gAlbedo;
layout (location = 3) out vec4 gMaterial;
layout (location = 4) out vec4 gPBR;
layout (location = 5) out vec4 gEmissive;

in vec3 FragPos;
in vec3 Normal;

void main() {
    gPosition = vec4(FragPos, 1.0);           // a=1.0 标记有几何写入
    gNormal   = vec4(normalize(Normal), 0.0);
    gAlbedo   = vec4(0.35, 0.35, 0.35, 1.0); // 地面固定灰白色
    gMaterial = vec4(1.0, 0.0, 0.0, 0.0);    // r=1.0 → 地面
    gPBR      = vec4(0.0, 0.9, 1.0, 1.0);    // metallic=0, roughness=0.9, ao=1.0, cavity=1.0
    gEmissive = vec4(0.0, 0.0, 0.0, 0.0);   // 地面无自发光
}

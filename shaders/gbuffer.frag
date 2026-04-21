#version 430 core

layout (location = 0) out vec4 gPosition;   // rgb=世界坐标, a=1.0(有几何)
layout (location = 1) out vec4 gNormal;     // rgb=世界法线, a=0.0
layout (location = 2) out vec4 gAlbedo;     // rgb=漫反射色, a=高光占位
layout (location = 3) out vec4 gMaterial;   // r=0(模型) 1(地面)
layout (location = 4) out vec4 gPBR;        // r=metallic, g=roughness, b=ao, a=cavity/gloss
layout (location = 5) out vec4 gEmissive;   // rgb=自发光颜色(HDR), a=未使用

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;
in mat3 TBN;

// Legacy texture uniforms (for backward compatibility, not used anymore)
uniform sampler2D texture_diffuse;
uniform sampler2D texture_normal;
uniform bool hasTexture;
uniform bool hasNormalMap;

// PBR texture maps (unified texture system for all lighting models)
uniform sampler2D texture_albedo;
uniform sampler2D texture_diffuse_pbr;
uniform sampler2D texture_normal_pbr;
uniform sampler2D texture_metallic;
uniform sampler2D texture_roughness;
uniform sampler2D texture_ao;
uniform sampler2D texture_cavity;
uniform sampler2D texture_gloss;
uniform sampler2D texture_specular;
uniform sampler2D texture_emissive;

uniform bool hasAlbedoMap;
uniform bool hasDiffuseMap;
uniform bool hasNormalMapPBR;
uniform bool hasMetallicMap;
uniform bool hasRoughnessMap;
uniform bool hasAoMap;
uniform bool hasCavityMap;
uniform bool hasGlossMap;
uniform bool hasSpecularMap;
uniform bool hasEmissiveMap;

// 自发光属性（无贴图时使用）
uniform vec3 emissiveColor;
uniform float emissiveIntensity;

void main() {
    gPosition = vec4(FragPos, 1.0); // a=1.0 标记有几何写入

    // Normal: use PBR normal map if available
    vec3 norm;
    if (hasNormalMapPBR) {
        norm = texture(texture_normal_pbr, TexCoords).rgb;
        norm = norm * 2.0 - 1.0;
        norm = normalize(TBN * norm);
    } else {
        norm = normalize(Normal);
    }
    gNormal = vec4(norm, 0.0);

    // Albedo/Diffuse: unified for all lighting models
    // Priority: Albedo > Diffuse > default color
    if (hasAlbedoMap) {
        gAlbedo.rgb = texture(texture_albedo, TexCoords).rgb;
    } else if (hasDiffuseMap) {
        gAlbedo.rgb = texture(texture_diffuse_pbr, TexCoords).rgb;
    } else {
        gAlbedo.rgb = vec3(0.8);
    }
    gAlbedo.a = 1.0;

    gMaterial = vec4(0.0, 0.0, 0.0, 0.0); // r=0 → 模型

    // PBR properties with defaults (used only in PBR lighting model)
    float metallic = hasMetallicMap ? texture(texture_metallic, TexCoords).r : 0.0;
    float roughness = hasRoughnessMap ? texture(texture_roughness, TexCoords).r : 0.5;
    float ao = hasAoMap ? texture(texture_ao, TexCoords).r : 1.0;
    float cavity = hasCavityMap ? texture(texture_cavity, TexCoords).r : 1.0;

    gPBR = vec4(metallic, roughness, ao, cavity);

    // 自发光：贴图优先，否则使用颜色×强度
    vec3 emissive = vec3(0.0);
    if (hasEmissiveMap) {
        emissive = texture(texture_emissive, TexCoords).rgb * emissiveIntensity;
    } else if (emissiveIntensity > 0.0) {
        emissive = emissiveColor * emissiveIntensity;
    }
    gEmissive = vec4(emissive, 0.0);
}

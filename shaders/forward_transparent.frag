#version 430 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;
in mat3 TBN;

// ------- 光源 -------
struct PointLight {
    vec3  position;
    vec3  color;
    float intensity;

    float constant;
    float linear;
    float quadratic;
};
#define MAX_LIGHTS 10
uniform PointLight lights[MAX_LIGHTS];
uniform int        numLights;
uniform vec3       viewPos;

// ------- 阴影 cubemaps（纹理单元 5..5+MAX_LIGHTS-1）-------
uniform samplerCube shadowMaps[MAX_LIGHTS];
uniform float       shadowFarPlane;

// ------- 光照模型：0=Blinn-Phong, 1=Phong, 2=PBR -------
uniform int lightingModel;

// ------- PBR贴图 -------
uniform sampler2D texture_albedo;
uniform sampler2D texture_diffuse_pbr;
uniform sampler2D texture_normal_pbr;
uniform sampler2D texture_metallic;
uniform sampler2D texture_roughness;
uniform sampler2D texture_ao;
uniform sampler2D texture_opacity;
uniform sampler2D texture_alpha;  // Alpha贴图（透明度）
uniform sampler2D texture_emissive; // 自发光贴图

uniform bool hasAlbedoMap;
uniform bool hasDiffuseMap;
uniform bool hasNormalMapPBR;
uniform bool hasMetallicMap;
uniform bool hasRoughnessMap;
uniform bool hasAoMap;
uniform bool hasOpacityMap;
uniform bool hasAlphaMap;  // 是否有Alpha贴图
uniform bool hasEmissiveMap; // 是否有自发光贴图

// 自发光属性
uniform vec3 emissiveColor;
uniform float emissiveIntensity;

// ------- Alpha测试阈值 -------
uniform float alphaThreshold;

// ------- 全局曝光值 -------
uniform float exposure;

// ------- PBR Constants -------
const float PI = 3.14159265359;

// ------- 阴影计算 -------
float ShadowCalculation(int idx, vec3 fragPos, vec3 lightPos) {
    vec3  dir          = fragPos - lightPos;
    float currentDepth = length(dir);
    float closestDepth = texture(shadowMaps[idx], dir).r * shadowFarPlane;
    float bias = 0.05;
    return (currentDepth - bias > closestDepth) ? 1.0 : 0.0;
}

// ------- Blinn-Phong光照 -------
vec3 CalcBlinnPhong(vec3 norm, vec3 fragPos, vec3 viewDir,
                    vec3 lightPos, vec3 lightColor, float intensity, int idx)
{
    vec3  lightDir    = normalize(lightPos - fragPos);
    float dist        = length(lightPos - fragPos);
    float atten       = intensity / (dist * dist + 0.0001);

    vec3  ambient     = 0.1 * lightColor;
    float diff        = max(dot(norm, lightDir), 0.0);
    vec3  diffuse     = diff * lightColor;
    vec3  halfway     = normalize(lightDir + viewDir);
    float spec        = pow(max(dot(norm, halfway), 0.0), 32.0);
    vec3  specular    = spec * lightColor;

    float shadow = ShadowCalculation(idx, fragPos, lightPos);
    return (ambient + (1.0 - shadow) * (diffuse + specular)) * atten;
}

// ------- Phong光照 -------
vec3 CalcPhong(vec3 norm, vec3 fragPos, vec3 viewDir,
               vec3 lightPos, vec3 lightColor, float intensity, int idx)
{
    vec3  lightDir    = normalize(lightPos - fragPos);
    float dist        = length(lightPos - fragPos);
    float atten       = intensity / (dist * dist + 0.0001);

    vec3  ambient     = 0.1 * lightColor;
    float diff        = max(dot(norm, lightDir), 0.0);
    vec3  diffuse     = diff * lightColor;
    vec3  reflectDir  = reflect(-lightDir, norm);
    float spec        = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
    vec3  specular    = spec * lightColor;

    float shadow = ShadowCalculation(idx, fragPos, lightPos);
    return (ambient + (1.0 - shadow) * (diffuse + specular)) * atten;
}

// ------- PBR Functions -------
float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float nom = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;

    float nom = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

// ------- PBR光照 -------
vec3 CalcPBR(vec3 norm, vec3 fragPos, vec3 viewDir, vec3 albedo, int idx,
             float metallic, float roughness, float ao)
{
    vec3 L = normalize(lights[idx].position - fragPos);
    vec3 H = normalize(viewDir + L);
    float distance = length(lights[idx].position - fragPos);
    float attenuation = lights[idx].intensity / (lights[idx].constant + lights[idx].linear * distance + 
                lights[idx].quadratic * (distance * distance));
    vec3 radiance = lights[idx].color * attenuation;

    // 计算F0（零入射角的表面反射率）
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);

    // Cook-Torrance BRDF
    float NDF = DistributionGGX(norm, H, roughness);
    float G = GeometrySmith(norm, viewDir, L, roughness);
    vec3 F = fresnelSchlick(max(dot(H, viewDir), 0.0), F0);

    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;

    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(norm, viewDir), 0.0) * max(dot(norm, L), 0.0) + 0.0001;
    vec3 specular = numerator / denominator;

    float NdotL = max(dot(norm, L), 0.0);
    float shadow = ShadowCalculation(idx, fragPos, lights[idx].position);

    vec3 Lo = (kD * albedo / PI + specular) * radiance * NdotL * (1.0 - shadow);

    // 环境光
    vec3 ambient = vec3(0.03) * albedo * ao;

    return ambient + Lo;
}

// -------------------------------------------------------
void main() {
    // 采样法线贴图
    vec3 norm;
    if (hasNormalMapPBR) {
        norm = texture(texture_normal_pbr, TexCoords).rgb;
        norm = norm * 2.0 - 1.0;
        norm = normalize(TBN * norm);
    } else {
        norm = normalize(Normal);
    }

    // 采样Albedo/Diffuse贴图
    vec3 albedo;
    float diffuseAlpha = 1.0;  // 保存Diffuse贴图的Alpha通道
    if (hasAlbedoMap) {
        vec4 albedoTexel = texture(texture_albedo, TexCoords);
        albedo = albedoTexel.rgb;
        diffuseAlpha = albedoTexel.a;
    } else if (hasDiffuseMap) {
        vec4 diffuseTexel = texture(texture_diffuse_pbr, TexCoords);
        albedo = diffuseTexel.rgb;
        diffuseAlpha = diffuseTexel.a;
    } else {
        albedo = vec3(0.8);
    }

    // 采样Alpha贴图（透明度）
    float opacity = 1.0;
    if (hasAlphaMap) {
        // Alpha贴图：从红色通道读取（灰度图）
        opacity = texture(texture_alpha, TexCoords).r;
    } else if (hasOpacityMap) {
        // 如果没有Alpha贴图但有Opacity贴图，使用Opacity贴图
        opacity = texture(texture_opacity, TexCoords).r;
    }
    // 注意：不使用Diffuse贴图的Alpha通道，因为它可能不是用于透明度的

    // Alpha测试：丢弃几乎完全透明的片段
    if (opacity < alphaThreshold) {
        discard;
    }

    // 采样PBR参数
    float metallic = hasMetallicMap ? texture(texture_metallic, TexCoords).r : 0.0;
    float roughness = hasRoughnessMap ? texture(texture_roughness, TexCoords).r : 0.5;
    float ao = hasAoMap ? texture(texture_ao, TexCoords).r : 1.0;

    // 计算光照
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 result = vec3(0.0);

    if (numLights == 0) {
        result = vec3(0.1); // 无光源：纯环境光
    } else {
        for (int i = 0; i < numLights; i++) {
            vec3 contrib;
            if (lightingModel == 2) {
                // PBR
                contrib = CalcPBR(norm, FragPos, viewDir, albedo, i,
                                  metallic, roughness, ao);
            } else if (lightingModel == 1) {
                // Phong
                contrib = CalcPhong(norm, FragPos, viewDir,
                                    lights[i].position, lights[i].color,
                                    lights[i].intensity, i);
            } else {
                // Blinn-Phong（默认）
                contrib = CalcBlinnPhong(norm, FragPos, viewDir,
                                         lights[i].position, lights[i].color,
                                         lights[i].intensity, i);
            }
            result += contrib;
        }
    }

    // 对于非PBR模型，乘以albedo
    if (lightingModel != 2) {
        result *= albedo;
    }

    // 叠加自发光
    vec3 emissive = vec3(0.0);
    if (hasEmissiveMap) {
        emissive = texture(texture_emissive, TexCoords).rgb * emissiveIntensity;
    } else if (emissiveIntensity > 0.0) {
        emissive = emissiveColor * emissiveIntensity;
    }
    result += emissive;

    // 输出最终颜色，包含透明度
    FragColor = vec4(result, opacity);
}

#version 430 core
out vec4 FragColor;

in vec2 TexCoords;

// ------- GBuffer（纹理单元 0-4）-------
uniform sampler2D gPosition;   // unit 0
uniform sampler2D gNormal;     // unit 1
uniform sampler2D gAlbedo;     // unit 2
uniform sampler2D gMaterial;   // unit 3
uniform sampler2D gPBR;        // unit 4

// ------- 自发光GBuffer（纹理单元 18）-------
uniform sampler2D gEmissive;   // unit 18

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

// ------- 阴影 cubemaps（纹理单元 5..14）-------
uniform samplerCube shadowMaps[MAX_LIGHTS];
uniform float       shadowFarPlane;

// ------- 光照模型：0=Blinn-Phong, 1=Phong, 2=PBR -------
uniform int lightingModel;

// ------- PBR Constants -------
const float PI = 3.14159265359;

// ------- IBL贴图（纹理单元 15-17）-------
uniform samplerCube irradianceMap;  // unit 15
uniform samplerCube prefilterMap;   // unit 16
uniform sampler2D   brdfLUT;        // unit 17
uniform bool hasIBL;
uniform float iblIntensity;  // IBL环境光强度系数
uniform float exposure;      // 全局曝光值

// -------------------------------------------------------
float ShadowCalculation(int idx, vec3 fragPos, vec3 lightPos) {
    vec3  dir          = fragPos - lightPos;
    float currentDepth = length(dir);
    float closestDepth = texture(shadowMaps[idx], dir).r * shadowFarPlane;
    float bias = 0.05;
    return (currentDepth - bias > closestDepth) ? 1.0 : 0.0;
}

vec3 CalcBlinnPhong(vec3 norm, vec3 fragPos, vec3 viewDir,
                    vec3 lightPos, vec3 lightColor, float intensity, int idx)
{
    vec3  lightDir    = normalize(lightPos - fragPos);
    float dist        = length(lightPos - fragPos);

    float atten = lights[idx].intensity / (lights[idx].constant + lights[idx].linear * dist + 
                lights[idx].quadratic * (dist * dist));

    vec3  ambient     = 0.1 * lightColor;
    float diff        = max(dot(norm, lightDir), 0.0);
    vec3  diffuse     = diff * lightColor;
    vec3  halfway     = normalize(lightDir + viewDir);
    float spec        = pow(max(dot(norm, halfway), 0.0), 32.0);
    vec3  specular    = spec * lightColor;

    float shadow = ShadowCalculation(idx, fragPos, lightPos);
    return (ambient + (1.0 - shadow) * (diffuse + specular)) * atten;
}

vec3 CalcPhong(vec3 norm, vec3 fragPos, vec3 viewDir,
               vec3 lightPos, vec3 lightColor, float intensity, int idx)
{
    vec3  lightDir    = normalize(lightPos - fragPos);
    float dist        = length(lightPos - fragPos);
    float atten = lights[idx].intensity / (lights[idx].constant + lights[idx].linear * dist + 
                lights[idx].quadratic * (dist * dist));

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

vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 CalcIBL(vec3 norm, vec3 viewDir, vec3 albedo, float metallic, float roughness, float ao)
{
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);

    vec3 F = fresnelSchlickRoughness(max(dot(norm, viewDir), 0.0), F0, roughness);

    vec3 kS = F;
    vec3 kD = 1.0 - kS;
    kD *= 1.0 - metallic;

    vec3 irradiance = texture(irradianceMap, norm).rgb;
    vec3 diffuse = irradiance * albedo;

    vec3 R = reflect(-viewDir, norm);
    const float MAX_REFLECTION_LOD = 4.0;
    vec3 prefilteredColor = textureLod(prefilterMap, R, roughness * MAX_REFLECTION_LOD).rgb;

    vec2 brdf = texture(brdfLUT, vec2(max(dot(norm, viewDir), 0.0), roughness)).rg;
    vec3 specular = prefilteredColor * (F * brdf.x + brdf.y);

    return (kD * diffuse + specular) * ao;
}

// CalcPBR只返回直接光照Lo，不包含环境光（环境光在main中统一处理）
vec3 CalcPBR(vec3 norm, vec3 fragPos, vec3 viewDir, vec3 albedo, int idx,
             float metallic, float roughness)
{
    vec3 L = normalize(lights[idx].position - fragPos);
    vec3 H = normalize(viewDir + L);
    float distance = length(lights[idx].position - fragPos);
    float attenuation = lights[idx].intensity / (lights[idx].constant + lights[idx].linear * distance + 
                lights[idx].quadratic * (distance * distance));
    vec3 radiance = lights[idx].color * attenuation;

    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);

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

    return (kD * albedo / PI + specular) * radiance * NdotL * (1.0 - shadow);
}

// -------------------------------------------------------
void main() {
    vec4 posA = texture(gPosition, TexCoords);

    if (posA.a < 0.5) {
        FragColor = vec4(0.05, 0.05, 0.05, 1.0);
        return;
    }

    vec3 FragPos = posA.rgb;
    vec3 norm    = normalize(texture(gNormal,   TexCoords).rgb);
    vec3 Albedo  = texture(gAlbedo,   TexCoords).rgb;
    vec4 pbrData = texture(gPBR, TexCoords);

    float metallic = pbrData.r;
    float roughness = pbrData.g;
    float ao = pbrData.b;

    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 result  = vec3(0.0);

    if (lightingModel == 2) {
        // PBR模式：直接光照 + 环境光照分离处理
        for (int i = 0; i < numLights; i++) {
            result += CalcPBR(norm, FragPos, viewDir, Albedo, i, metallic, roughness);
        }

        // 环境光：优先使用IBL，否则使用固定环境光
        if (hasIBL) {
            result += CalcIBL(norm, viewDir, Albedo, metallic, roughness, ao) * iblIntensity;
        } else {
            result += vec3(0.03) * Albedo * ao;
        }
    } else {
        // 非PBR模式
        if (numLights == 0) {
            result = vec3(0.1);
        } else {
            for (int i = 0; i < numLights; i++) {
                if (lightingModel == 1) {
                    result += CalcPhong(norm, FragPos, viewDir,
                                        lights[i].position, lights[i].color,
                                        lights[i].intensity, i);
                } else {
                    result += CalcBlinnPhong(norm, FragPos, viewDir,
                                             lights[i].position, lights[i].color,
                                             lights[i].intensity, i);
                }
            }
        }
        result *= Albedo;
    }


    // 叠加自发光
    vec3 emissive = texture(gEmissive, TexCoords).rgb;
    result += emissive;

    FragColor = vec4(result, 1.0);
}

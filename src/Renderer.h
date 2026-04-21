#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>
#include <memory>

#include "Shader.h"
#include "Model.h"
#include "Light.h"
#include "Camera.h"
#include "Frustum.h"

// 阴影立方体贴图结构
struct ShadowCubemap {
    unsigned int fbo;
    unsigned int cubemap;
};

class Renderer {
public:
    // 构造函数
    Renderer(unsigned int screenWidth, unsigned int screenHeight);

    // 析构函数
    ~Renderer();

    // 禁用拷贝
    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    // 渲染场景
    void renderScene(
        const std::vector<Model*>& models,
        const std::vector<PointLight>& lights,
        Camera& camera,
        LightingModel lightingModel,
        Model* selectedModel = nullptr,
        bool enableFrustumCulling = true  // 是否启用视锥剔除
    );

    // 更新屏幕尺寸（窗口大小改变时调用）
    void resize(unsigned int width, unsigned int height);

    void cleanupIBL();
    bool loadHDREnvironment(const std::string& hdrPath);
    bool hasEnvironmentMap() const { return envCubemap != 0; }

    // IBL强度和全局曝光控制
    float iblIntensity = 1.0f;   // IBL环境光强度系数 (0.0 ~ 5.0)
    float exposure = 1.0f;       // 全局曝光值 (0.1 ~ 10.0)

private:
    // 屏幕尺寸
    unsigned int screenWidth;
    unsigned int screenHeight;

    // 阴影相关常量
    static constexpr unsigned int SHADOW_SIZE = 2048;
    static constexpr float SHADOW_NEAR = 0.1f;
    static constexpr float SHADOW_FAR = 25.0f;
    static constexpr int MAX_LIGHTS = 100;

    // 着色器
    std::unique_ptr<Shader> gbufferShader;
    std::unique_ptr<Shader> groundGBufferShader;
    std::unique_ptr<Shader> deferredLightingShader;
    std::unique_ptr<Shader> shadowCubeShader;
    std::unique_ptr<Shader> outlineShader;
    std::unique_ptr<Shader> forwardTransparentShader;
    std::unique_ptr<Shader> equirectToCubemapShader;
    std::unique_ptr<Shader> irradianceShader;
    std::unique_ptr<Shader> prefilterShader;
    std::unique_ptr<Shader> brdfShader;
    std::unique_ptr<Shader> skyboxShader;

    // GBuffer
    unsigned int gBuffer;
    unsigned int gPosition, gNormal, gAlbedo, gMaterial, gPBR, gEmissive;
    unsigned int gDepth;

    // 阴影贴图
    std::vector<ShadowCubemap> shadowCubemaps;
    unsigned int dummyCubemap;

    // 全屏四边形
    unsigned int quadVAO, quadVBO;

    // 地面
    unsigned int groundVAO, groundVBO;

    // 视锥体
    Frustum frustum;

    // IBL
    unsigned int envCubemap;           // 环境立方体贴图
    unsigned int irradianceMap;        // 辐照度贴图
    unsigned int prefilterMap;         // 预滤波环境贴图
    unsigned int brdfLUTTexture;       // BRDF查找表
    unsigned int captureFBO, captureRBO; // 用于生成IBL贴图的帧缓冲
    unsigned int cubeVAO, cubeVBO;

    // 初始化函数
    void initShaders();
    void setupGBuffer();
    void setupQuad();
    void setupGround();
    void setupDummyCubemap();
    void setupCube();
    void setupCaptureFBO();

    // 阴影贴图管理
    ShadowCubemap createShadowCubemap();
    void deleteShadowCubemap(ShadowCubemap& sc);
    void syncShadowCubemaps(size_t numLights);
    std::vector<glm::mat4> getShadowMatrices(glm::vec3 lightPos);

    // 渲染Pass
    void renderShadowPass(const std::vector<Model*>& models, const std::vector<PointLight>& lights, bool enableFrustumCulling);
    void renderGBufferPass(const std::vector<Model*>& models, Camera& camera, bool enableFrustumCulling);
    void renderLightingPass(const std::vector<PointLight>& lights, Camera& camera, LightingModel lightingModel);
    void renderOutlinePass(const std::vector<Model*>& models, Camera& camera, Model* selectedModel);
    void renderTransparentPass(const std::vector<Model*>& models, const std::vector<PointLight>& lights, Camera& camera, LightingModel lightingModel, bool enableFrustumCulling);

    // IBL
    unsigned int loadHDRTexture(const std::string& path);
    void generateCubemapFromEquirectangular(unsigned int hdrTexture);
    void generateIrradianceMap();
    void generatePrefilterMap();
    void generateBRDFLUT();
    void renderSkyboxPass(Camera& camera);

    // 辅助函数
    // void bindMaterialTextures(Shader& shader, Material* mat, int textureUnitOffset);
};

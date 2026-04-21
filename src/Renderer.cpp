#include "Renderer.h"
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <algorithm>
#include <stb_image.h>

Renderer::Renderer(unsigned int screenWidth, unsigned int screenHeight)
    : screenWidth(screenWidth), screenHeight(screenHeight),
      gBuffer(0), gPosition(0), gNormal(0), gAlbedo(0), gMaterial(0), gPBR(0), gEmissive(0), gDepth(0),
      dummyCubemap(0), quadVAO(0), quadVBO(0), groundVAO(0), groundVBO(0),
      envCubemap(0), irradianceMap(0), prefilterMap(0), brdfLUTTexture(0),
      captureFBO(0), captureRBO(0), cubeVAO(0), cubeVBO(0) {

    // 初始化所有渲染资源
    initShaders();
    setupGBuffer();
    setupQuad();
    setupGround();
    setupDummyCubemap();
}

Renderer::~Renderer() {
    // 清理阴影贴图
    for (auto& sc : shadowCubemaps) {
        deleteShadowCubemap(sc);
    }
    shadowCubemaps.clear();

    // 清理GBuffer
    if (gBuffer) glDeleteFramebuffers(1, &gBuffer);
    if (gPosition) glDeleteTextures(1, &gPosition);
    if (gNormal) glDeleteTextures(1, &gNormal);
    if (gAlbedo) glDeleteTextures(1, &gAlbedo);
    if (gMaterial) glDeleteTextures(1, &gMaterial);
    if (gPBR) glDeleteTextures(1, &gPBR);
    if (gEmissive) glDeleteTextures(1, &gEmissive);
    if (gDepth) glDeleteRenderbuffers(1, &gDepth);

    // 清理其他资源
    if (dummyCubemap) glDeleteTextures(1, &dummyCubemap);
    if (quadVAO) glDeleteVertexArrays(1, &quadVAO);
    if (quadVBO) glDeleteBuffers(1, &quadVBO);
    if (groundVAO) glDeleteVertexArrays(1, &groundVAO);
    if (groundVBO) glDeleteBuffers(1, &groundVBO);

    // 清理IBL资源
    cleanupIBL();
    if (captureFBO) glDeleteFramebuffers(1, &captureFBO);
    if (captureRBO) glDeleteRenderbuffers(1, &captureRBO);
    if (cubeVAO) glDeleteVertexArrays(1, &cubeVAO);
    if (cubeVBO) glDeleteBuffers(1, &cubeVBO);
}

void Renderer::initShaders() {
    gbufferShader = std::make_unique<Shader>("shaders/gbuffer.vert", "shaders/gbuffer.frag");
    groundGBufferShader = std::make_unique<Shader>("shaders/ground.vert", "shaders/ground.frag");
    deferredLightingShader = std::make_unique<Shader>("shaders/deferred_lighting.vert", "shaders/deferred_lighting.frag");
    shadowCubeShader = std::make_unique<Shader>("shaders/shadow_cube.vert", "shaders/shadow_cube.geom", "shaders/shadow_cube.frag");
    outlineShader = std::make_unique<Shader>("shaders/outline.vert", "shaders/outline.frag");
    forwardTransparentShader = std::make_unique<Shader>("shaders/forward_transparent.vert", "shaders/forward_transparent.frag");

    // IBL着色器
    equirectToCubemapShader = std::make_unique<Shader>("shaders/equirect_to_cubemap.vert", "shaders/equirect_to_cubemap.frag");
    irradianceShader = std::make_unique<Shader>("shaders/irradiance.vert", "shaders/irradiance.frag");
    prefilterShader = std::make_unique<Shader>("shaders/prefilter.vert", "shaders/prefilter.frag");
    brdfShader = std::make_unique<Shader>("shaders/brdf.vert", "shaders/brdf.frag");
    skyboxShader = std::make_unique<Shader>("shaders/skybox.vert", "shaders/skybox.frag");
}

void Renderer::setupGBuffer() {
    glGenFramebuffers(1, &gBuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);

    // gPosition (RGBA16F)
    glGenTextures(1, &gPosition);
    glBindTexture(GL_TEXTURE_2D, gPosition);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, screenWidth, screenHeight, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gPosition, 0);

    // gNormal (RGBA16F)
    glGenTextures(1, &gNormal);
    glBindTexture(GL_TEXTURE_2D, gNormal);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, screenWidth, screenHeight, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, gNormal, 0);

    // gAlbedo (RGBA8)
    glGenTextures(1, &gAlbedo);
    glBindTexture(GL_TEXTURE_2D, gAlbedo);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, screenWidth, screenHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, gAlbedo, 0);

    // gMaterial (RGBA8)
    glGenTextures(1, &gMaterial);
    glBindTexture(GL_TEXTURE_2D, gMaterial);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, screenWidth, screenHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, GL_TEXTURE_2D, gMaterial, 0);

    // gPBR (RGBA16F) - PBR材质属性
    glGenTextures(1, &gPBR);
    glBindTexture(GL_TEXTURE_2D, gPBR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, screenWidth, screenHeight, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT4, GL_TEXTURE_2D, gPBR, 0);

    // gEmissive (RGBA16F) - 自发光颜色（HDR，支持高亮度值）
    glGenTextures(1, &gEmissive);
    glBindTexture(GL_TEXTURE_2D, gEmissive);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, screenWidth, screenHeight, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT5, GL_TEXTURE_2D, gEmissive, 0);

    // 深度 Renderbuffer
    glGenRenderbuffers(1, &gDepth);
    glBindRenderbuffer(GL_RENDERBUFFER, gDepth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, screenWidth, screenHeight);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, gDepth);

    unsigned int attachments[6] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1,
                                     GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3,
                                     GL_COLOR_ATTACHMENT4, GL_COLOR_ATTACHMENT5 };
    glDrawBuffers(6, attachments);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "ERROR::FRAMEBUFFER:: GBuffer is not complete!" << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::setupQuad() {
    float quadVertices[] = {
        // pos        // texCoords
        -1.0f,  1.0f,  0.0f, 1.0f,
        -1.0f, -1.0f,  0.0f, 0.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
        -1.0f,  1.0f,  0.0f, 1.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
         1.0f,  1.0f,  1.0f, 1.0f
    };
    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);
    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glBindVertexArray(0);
}

void Renderer::setupGround() {
    float groundVertices[] = {
        -50.0f, 0.0f, -50.0f,  0.0f, 1.0f, 0.0f,
         50.0f, 0.0f, -50.0f,  0.0f, 1.0f, 0.0f,
         50.0f, 0.0f,  50.0f,  0.0f, 1.0f, 0.0f,
         50.0f, 0.0f,  50.0f,  0.0f, 1.0f, 0.0f,
        -50.0f, 0.0f,  50.0f,  0.0f, 1.0f, 0.0f,
        -50.0f, 0.0f, -50.0f,  0.0f, 1.0f, 0.0f
    };
    glGenVertexArrays(1, &groundVAO);
    glGenBuffers(1, &groundVBO);
    glBindVertexArray(groundVAO);
    glBindBuffer(GL_ARRAY_BUFFER, groundVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(groundVertices), groundVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glBindVertexArray(0);
}

void Renderer::setupDummyCubemap() {
    float whitePixel = 1.0f;
    glGenTextures(1, &dummyCubemap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, dummyCubemap);
    for (unsigned int i = 0; i < 6; ++i) {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_R32F,
                     1, 1, 0, GL_RED, GL_FLOAT, &whitePixel);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
}

void Renderer::setupCube() {
    float vertices[] = {
        -1.0f,  1.0f, -1.0f,  -1.0f, -1.0f, -1.0f,   1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,   1.0f,  1.0f, -1.0f,  -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,  -1.0f, -1.0f, -1.0f,  -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,  -1.0f,  1.0f,  1.0f,  -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f, -1.0f,   1.0f, -1.0f,  1.0f,   1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,   1.0f,  1.0f, -1.0f,   1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,  -1.0f,  1.0f,  1.0f,   1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,   1.0f, -1.0f,  1.0f,  -1.0f, -1.0f,  1.0f,
        -1.0f,  1.0f, -1.0f,   1.0f,  1.0f, -1.0f,   1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,  -1.0f,  1.0f,  1.0f,  -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,  -1.0f, -1.0f,  1.0f,   1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,  -1.0f, -1.0f,  1.0f,   1.0f, -1.0f,  1.0f
    };
    glGenVertexArrays(1, &cubeVAO);
    glGenBuffers(1, &cubeVBO);
    glBindVertexArray(cubeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glBindVertexArray(0);
}

void Renderer::setupCaptureFBO() {
    glGenFramebuffers(1, &captureFBO);
    glGenRenderbuffers(1, &captureRBO);
    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
    glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 512, 512);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, captureRBO);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

ShadowCubemap Renderer::createShadowCubemap() {
    ShadowCubemap sc;
    glGenFramebuffers(1, &sc.fbo);
    glGenTextures(1, &sc.cubemap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, sc.cubemap);
    for (unsigned int i = 0; i < 6; ++i) {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_DEPTH_COMPONENT,
                     SHADOW_SIZE, SHADOW_SIZE, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    glBindFramebuffer(GL_FRAMEBUFFER, sc.fbo);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, sc.cubemap, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    return sc;
}

void Renderer::deleteShadowCubemap(ShadowCubemap& sc) {
    glDeleteFramebuffers(1, &sc.fbo);
    glDeleteTextures(1, &sc.cubemap);
    sc.fbo = sc.cubemap = 0;
}

void Renderer::syncShadowCubemaps(size_t numLights) {
    while (shadowCubemaps.size() > numLights) {
        deleteShadowCubemap(shadowCubemaps.back());
        shadowCubemaps.pop_back();
    }
    while (shadowCubemaps.size() < numLights) {
        shadowCubemaps.push_back(createShadowCubemap());
    }
}

std::vector<glm::mat4> Renderer::getShadowMatrices(glm::vec3 lightPos) {
    glm::mat4 proj = glm::perspective(glm::radians(90.0f), 1.0f, SHADOW_NEAR, SHADOW_FAR);
    return {
        proj * glm::lookAt(lightPos, lightPos + glm::vec3( 1, 0, 0), glm::vec3(0,-1, 0)),
        proj * glm::lookAt(lightPos, lightPos + glm::vec3(-1, 0, 0), glm::vec3(0,-1, 0)),
        proj * glm::lookAt(lightPos, lightPos + glm::vec3( 0, 1, 0), glm::vec3(0, 0, 1)),
        proj * glm::lookAt(lightPos, lightPos + glm::vec3( 0,-1, 0), glm::vec3(0, 0,-1)),
        proj * glm::lookAt(lightPos, lightPos + glm::vec3( 0, 0, 1), glm::vec3(0,-1, 0)),
        proj * glm::lookAt(lightPos, lightPos + glm::vec3( 0, 0,-1), glm::vec3(0,-1, 0)),
    };
}

void Renderer::resize(unsigned int width, unsigned int height) {
    screenWidth = width;
    screenHeight = height;

    // 重新创建GBuffer纹理
    if (gPosition) glDeleteTextures(1, &gPosition);
    if (gNormal) glDeleteTextures(1, &gNormal);
    if (gAlbedo) glDeleteTextures(1, &gAlbedo);
    if (gMaterial) glDeleteTextures(1, &gMaterial);
    if (gPBR) glDeleteTextures(1, &gPBR);
    if (gEmissive) glDeleteTextures(1, &gEmissive);
    if (gDepth) glDeleteRenderbuffers(1, &gDepth);

    glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);

    // 重新创建所有GBuffer纹理
    glGenTextures(1, &gPosition);
    glBindTexture(GL_TEXTURE_2D, gPosition);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, screenWidth, screenHeight, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gPosition, 0);

    glGenTextures(1, &gNormal);
    glBindTexture(GL_TEXTURE_2D, gNormal);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, screenWidth, screenHeight, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, gNormal, 0);

    glGenTextures(1, &gAlbedo);
    glBindTexture(GL_TEXTURE_2D, gAlbedo);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, screenWidth, screenHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, gAlbedo, 0);

    glGenTextures(1, &gMaterial);
    glBindTexture(GL_TEXTURE_2D, gMaterial);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, screenWidth, screenHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, GL_TEXTURE_2D, gMaterial, 0);

    glGenTextures(1, &gPBR);
    glBindTexture(GL_TEXTURE_2D, gPBR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, screenWidth, screenHeight, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT4, GL_TEXTURE_2D, gPBR, 0);

    glGenTextures(1, &gEmissive);
    glBindTexture(GL_TEXTURE_2D, gEmissive);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, screenWidth, screenHeight, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT5, GL_TEXTURE_2D, gEmissive, 0);

    glGenRenderbuffers(1, &gDepth);
    glBindRenderbuffer(GL_RENDERBUFFER, gDepth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, screenWidth, screenHeight);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, gDepth);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::renderScene(
    const std::vector<Model*>& models,
    const std::vector<PointLight>& lights,
    Camera& camera,
    LightingModel lightingModel,
    Model* selectedModel,
    bool enableFrustumCulling) {

    // 更新视锥体
    if (enableFrustumCulling) {
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom),
                                                (float)screenWidth / (float)screenHeight,
                                                0.1f, 100.0f);
        glm::mat4 view = camera.GetViewMatrix();
        frustum.update(projection, view);
    }

    // 同步阴影贴图数量
    syncShadowCubemaps(lights.size());

    // Pass 1: 阴影Pass
    renderShadowPass(models, lights, enableFrustumCulling);

    // Pass 2: GBuffer Pass
    renderGBufferPass(models, camera, enableFrustumCulling);

    // Pass 3: 延迟光照Pass
    renderLightingPass(lights, camera, lightingModel);

    // Pass 4: 描边Pass
    renderOutlinePass(models, camera, selectedModel);

    if (lightingModel == LightingModel::PBR && hasEnvironmentMap()) {
        renderSkyboxPass(camera);
    }

    // Pass 5: 透明物体Pass
    renderTransparentPass(models, lights, camera, lightingModel, enableFrustumCulling);
}

void Renderer::renderShadowPass(const std::vector<Model*>& models, const std::vector<PointLight>& lights, bool enableFrustumCulling) {
    glViewport(0, 0, SHADOW_SIZE, SHADOW_SIZE);
    shadowCubeShader->use();
    shadowCubeShader->setFloat("farPlane", SHADOW_FAR);

    for (int li = 0; li < (int)lights.size(); li++) {
        glBindFramebuffer(GL_FRAMEBUFFER, shadowCubemaps[li].fbo);
        glClear(GL_DEPTH_BUFFER_BIT);

        auto shadowMatrices = getShadowMatrices(lights[li].position);
        for (int f = 0; f < 6; f++) {
            shadowCubeShader->setMat4("shadowMatrices[" + std::to_string(f) + "]", shadowMatrices[f]);
        }
        shadowCubeShader->setVec3("lightPos", lights[li].position);

        for (auto m : models) {
            // Shadow Pass 不使用相机视锥剔除
            // 因为相机看不到的物体仍可能投射阴影到可见区域
            // 使用基于光源距离的剔除来优化性能
            float distanceToLight = glm::length(m->position - lights[li].position);
            if (distanceToLight > SHADOW_FAR * 1.5f) {
                continue;
            }

            shadowCubeShader->setMat4("model", m->GetModelMatrix());

            // 逐mesh渲染，跳过透明材质（让光线穿透透明物体）
            for (auto& mesh : m->meshes) {
                Material* mat = m->getMaterial(mesh.materialIndex);
                if (mat && mat->isTransparent) {
                    continue;
                }
                mesh.Draw();
            }
        }
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::renderGBufferPass(const std::vector<Model*>& models, Camera& camera, bool enableFrustumCulling) {
    glViewport(0, 0, screenWidth, screenHeight);
    glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom),
                                             (float)screenWidth / (float)screenHeight, 0.1f, 200.0f);
    glm::mat4 view = camera.GetViewMatrix();

    gbufferShader->use();
    gbufferShader->setMat4("projection", projection);
    gbufferShader->setMat4("view", view);

    // 遍历每个模型
    for (auto m : models) {
        // 视锥剔除检测
        if (enableFrustumCulling && !m->isInFrustum(frustum)) {
            continue;  // 跳过不在视锥体内的模型
        }

        gbufferShader->setMat4("model", m->GetModelMatrix());

        // 遍历每个mesh，使用其对应的材质
        for (auto& mesh : m->meshes) {
            // 获取该mesh的材质
            Material* mat = m->getMaterial(mesh.materialIndex);

            // 跳过透明材质（留给Forward Pass渲染）
            if (mat && mat->isTransparent) {
                continue;
            }

            if (mat) {
                // 绑定材质的所有贴图
                gbufferShader->setBool("hasAlbedoMap", mat->hasAlbedoMap);
                gbufferShader->setBool("hasDiffuseMap", mat->hasDiffuseMap);
                gbufferShader->setBool("hasNormalMapPBR", mat->hasNormalMap);
                gbufferShader->setBool("hasMetallicMap", mat->hasMetallicMap);
                gbufferShader->setBool("hasRoughnessMap", mat->hasRoughnessMap);
                gbufferShader->setBool("hasAoMap", mat->hasAoMap);
                gbufferShader->setBool("hasCavityMap", mat->hasCavityMap);
                gbufferShader->setBool("hasGlossMap", mat->hasGlossMap);
                gbufferShader->setBool("hasSpecularMap", mat->hasSpecularMap);
                gbufferShader->setBool("hasEmissiveMap", mat->hasEmissiveMap);
                gbufferShader->setVec3("emissiveColor", mat->emissiveColor);
                gbufferShader->setFloat("emissiveIntensity", mat->emissiveIntensity);

                // 绑定纹理
                if (mat->hasAlbedoMap) {
                    glActiveTexture(GL_TEXTURE10);
                    glBindTexture(GL_TEXTURE_2D, mat->albedoMapID);
                    gbufferShader->setInt("texture_albedo", 10);
                }
                if (mat->hasDiffuseMap) {
                    glActiveTexture(GL_TEXTURE11);
                    glBindTexture(GL_TEXTURE_2D, mat->diffuseMapID);
                    gbufferShader->setInt("texture_diffuse_pbr", 11);
                }
                if (mat->hasNormalMap) {
                    glActiveTexture(GL_TEXTURE12);
                    glBindTexture(GL_TEXTURE_2D, mat->normalMapID);
                    gbufferShader->setInt("texture_normal_pbr", 12);
                }
                if (mat->hasMetallicMap) {
                    glActiveTexture(GL_TEXTURE13);
                    glBindTexture(GL_TEXTURE_2D, mat->metallicMapID);
                    gbufferShader->setInt("texture_metallic", 13);
                }
                if (mat->hasRoughnessMap) {
                    glActiveTexture(GL_TEXTURE14);
                    glBindTexture(GL_TEXTURE_2D, mat->roughnessMapID);
                    gbufferShader->setInt("texture_roughness", 14);
                }
                if (mat->hasAoMap) {
                    glActiveTexture(GL_TEXTURE15);
                    glBindTexture(GL_TEXTURE_2D, mat->aoMapID);
                    gbufferShader->setInt("texture_ao", 15);
                }
                if (mat->hasCavityMap) {
                    glActiveTexture(GL_TEXTURE16);
                    glBindTexture(GL_TEXTURE_2D, mat->cavityMapID);
                    gbufferShader->setInt("texture_cavity", 16);
                }
                if (mat->hasGlossMap) {
                    glActiveTexture(GL_TEXTURE17);
                    glBindTexture(GL_TEXTURE_2D, mat->glossMapID);
                    gbufferShader->setInt("texture_gloss", 17);
                }
                if (mat->hasSpecularMap) {
                    glActiveTexture(GL_TEXTURE18);
                    glBindTexture(GL_TEXTURE_2D, mat->specularMapID);
                    gbufferShader->setInt("texture_specular", 18);
                }
                if (mat->hasEmissiveMap) {
                    glActiveTexture(GL_TEXTURE19);
                    glBindTexture(GL_TEXTURE_2D, mat->emissiveMapID);
                    gbufferShader->setInt("texture_emissive", 19);
                }
            } else {
                // 如果没有材质，设置所有标志为false
                gbufferShader->setBool("hasAlbedoMap", false);
                gbufferShader->setBool("hasDiffuseMap", false);
                gbufferShader->setBool("hasNormalMapPBR", false);
                gbufferShader->setBool("hasMetallicMap", false);
                gbufferShader->setBool("hasRoughnessMap", false);
                gbufferShader->setBool("hasAoMap", false);
                gbufferShader->setBool("hasCavityMap", false);
                gbufferShader->setBool("hasGlossMap", false);
                gbufferShader->setBool("hasSpecularMap", false);
                gbufferShader->setBool("hasEmissiveMap", false);
                gbufferShader->setVec3("emissiveColor", glm::vec3(0.0f));
                gbufferShader->setFloat("emissiveIntensity", 0.0f);
            }

            // 绘制该mesh
            mesh.Draw();
        }
    }

    // 渲染地面到 GBuffer
    groundGBufferShader->use();
    groundGBufferShader->setMat4("projection", projection);
    groundGBufferShader->setMat4("view", view);
    groundGBufferShader->setMat4("model", glm::mat4(1.0f));
    glBindVertexArray(groundVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::renderLightingPass(const std::vector<PointLight>& lights, Camera& camera, LightingModel lightingModel) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    deferredLightingShader->use();

    // 绑定 GBuffer 纹理（单元 0-4）
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gPosition);
    deferredLightingShader->setInt("gPosition", 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, gNormal);
    deferredLightingShader->setInt("gNormal", 1);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, gAlbedo);
    deferredLightingShader->setInt("gAlbedo", 2);

    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, gMaterial);
    deferredLightingShader->setInt("gMaterial", 3);

    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, gPBR);
    deferredLightingShader->setInt("gPBR", 4);

    glActiveTexture(GL_TEXTURE18);
    glBindTexture(GL_TEXTURE_2D, gEmissive);
    deferredLightingShader->setInt("gEmissive", 18);

    // 绑定阴影 cubemaps（单元 5..14，与着色器MAX_LIGHTS=10匹配）
    static constexpr int SHADER_MAX_LIGHTS = 10;
    int numReal = (int)shadowCubemaps.size();
    for (int i = 0; i < SHADER_MAX_LIGHTS; i++) {
        glActiveTexture(GL_TEXTURE5 + i);
        if (i < numReal)
            glBindTexture(GL_TEXTURE_CUBE_MAP, shadowCubemaps[i].cubemap);
        else
            glBindTexture(GL_TEXTURE_CUBE_MAP, dummyCubemap);
        deferredLightingShader->setInt("shadowMaps[" + std::to_string(i) + "]", 5 + i);
    }
    deferredLightingShader->setFloat("shadowFarPlane", SHADOW_FAR);

    // 绑定IBL纹理（单元 15-17）
    bool iblActive = (lightingModel == LightingModel::PBR && envCubemap != 0);
    deferredLightingShader->setBool("hasIBL", iblActive);
    if (iblActive) {
        glActiveTexture(GL_TEXTURE15);
        glBindTexture(GL_TEXTURE_CUBE_MAP, irradianceMap);
        deferredLightingShader->setInt("irradianceMap", 15);

        glActiveTexture(GL_TEXTURE16);
        glBindTexture(GL_TEXTURE_CUBE_MAP, prefilterMap);
        deferredLightingShader->setInt("prefilterMap", 16);

        glActiveTexture(GL_TEXTURE17);
        glBindTexture(GL_TEXTURE_2D, brdfLUTTexture);
        deferredLightingShader->setInt("brdfLUT", 17);
    }

    // 设置光源
    deferredLightingShader->setInt("numLights", (int)lights.size());
    for (int i = 0; i < (int)lights.size(); i++) {
        std::string base = "lights[" + std::to_string(i) + "]";
        deferredLightingShader->setVec3(base + ".position", lights[i].position);
        deferredLightingShader->setVec3(base + ".color", lights[i].color);
        deferredLightingShader->setFloat(base + ".intensity", lights[i].intensity);
        deferredLightingShader->setFloat(base + ".constant", 1.f);
        deferredLightingShader->setFloat(base + ".linear", 0.09);
        deferredLightingShader->setFloat(base + ".quadratic", 0.032);
    }
    deferredLightingShader->setVec3("viewPos", camera.Position);
    deferredLightingShader->setInt("lightingModel", (int)lightingModel);

    // 设置IBL强度和全局曝光
    deferredLightingShader->setFloat("iblIntensity", iblIntensity);
    deferredLightingShader->setFloat("exposure", exposure);

    // 渲染全屏四边形
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

void Renderer::renderOutlinePass(const std::vector<Model*>& models, Camera& camera, Model* selectedModel) {
    // 将 GBuffer 的深度复制到默认帧缓冲
    glBindFramebuffer(GL_READ_FRAMEBUFFER, gBuffer);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    glBlitFramebuffer(0, 0, screenWidth, screenHeight, 0, 0, screenWidth, screenHeight,
                      GL_DEPTH_BUFFER_BIT, GL_NEAREST);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom),
                                             (float)screenWidth / (float)screenHeight, 0.1f, 200.0f);
    glm::mat4 view = camera.GetViewMatrix();

    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);
    outlineShader->use();
    outlineShader->setMat4("projection", projection);
    outlineShader->setMat4("view", view);

    for (auto m : models) {
        if (m->isSelected) {
            glm::mat4 scaledMat = glm::scale(m->GetModelMatrix(), glm::vec3(1.03f));
            outlineShader->setMat4("model", scaledMat);
            m->Draw();
        }
    }
    glCullFace(GL_BACK);
    glDisable(GL_CULL_FACE);
}

void Renderer::renderTransparentPass(const std::vector<Model*>& models, const std::vector<PointLight>& lights, Camera& camera, LightingModel lightingModel, bool enableFrustumCulling) {
    // 启用混合
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE); // 透明物体不写入深度

    glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom),
                                             (float)screenWidth / (float)screenHeight, 0.1f, 200.0f);
    glm::mat4 view = camera.GetViewMatrix();

    forwardTransparentShader->use();
    forwardTransparentShader->setMat4("projection", projection);
    forwardTransparentShader->setMat4("view", view);

    // 设置光源
    forwardTransparentShader->setInt("numLights", (int)lights.size());
    for (int i = 0; i < (int)lights.size(); i++) {
        std::string base = "lights[" + std::to_string(i) + "]";
        forwardTransparentShader->setVec3(base + ".position", lights[i].position);
        forwardTransparentShader->setVec3(base + ".color", lights[i].color);
        forwardTransparentShader->setFloat(base + ".intensity", lights[i].intensity);
        forwardTransparentShader->setFloat(base + ".constant", 1.f);
        forwardTransparentShader->setFloat(base + ".linear", 0.09);
        forwardTransparentShader->setFloat(base + ".quadratic", 0.032);
    }
    forwardTransparentShader->setVec3("viewPos", camera.Position);
    forwardTransparentShader->setInt("lightingModel", (int)lightingModel);

    // 设置全局曝光
    forwardTransparentShader->setFloat("exposure", exposure);

    // 绑定阴影 cubemaps（与着色器MAX_LIGHTS=10匹配）
    static constexpr int SHADER_MAX_LIGHTS_T = 10;
    int numReal = (int)shadowCubemaps.size();
    for (int i = 0; i < SHADER_MAX_LIGHTS_T; i++) {
        glActiveTexture(GL_TEXTURE5 + i);
        if (i < numReal)
            glBindTexture(GL_TEXTURE_CUBE_MAP, shadowCubemaps[i].cubemap);
        else
            glBindTexture(GL_TEXTURE_CUBE_MAP, dummyCubemap);
        forwardTransparentShader->setInt("shadowMaps[" + std::to_string(i) + "]", 5 + i);
    }
    forwardTransparentShader->setFloat("shadowFarPlane", SHADOW_FAR);

    // 收集所有透明mesh并按深度排序
    struct TransparentMesh {
        Model* model;
        Mesh* mesh;
        Material* material;
        float distance;
    };
    std::vector<TransparentMesh> transparentMeshes;

    for (auto m : models) {
        // 视锥剔除检测
        if (enableFrustumCulling && !m->isInFrustum(frustum)) {
            continue;  // 跳过不在视锥体内的模型
        }

        for (auto& mesh : m->meshes) {
            Material* mat = m->getMaterial(mesh.materialIndex);
            if (mat && mat->isTransparent) {
                // 计算mesh中心到相机的距离
                glm::vec3 meshCenter = glm::vec3(m->GetModelMatrix() * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
                float dist = glm::length(camera.Position - meshCenter);
                transparentMeshes.push_back({m, &mesh, mat, dist});
            }
        }
    }

    // 从远到近排序
    std::sort(transparentMeshes.begin(), transparentMeshes.end(),
              [](const TransparentMesh& a, const TransparentMesh& b) {
                  return a.distance > b.distance;
              });

    // 渲染透明物体
    for (const auto& tm : transparentMeshes) {
        forwardTransparentShader->setMat4("model", tm.model->GetModelMatrix());
        forwardTransparentShader->setFloat("alphaThreshold", tm.material->alphaThreshold);

        // 绑定材质的所有贴图
        forwardTransparentShader->setBool("hasAlbedoMap", tm.material->hasAlbedoMap);
        forwardTransparentShader->setBool("hasDiffuseMap", tm.material->hasDiffuseMap);
        forwardTransparentShader->setBool("hasNormalMapPBR", tm.material->hasNormalMap);
        forwardTransparentShader->setBool("hasMetallicMap", tm.material->hasMetallicMap);
        forwardTransparentShader->setBool("hasRoughnessMap", tm.material->hasRoughnessMap);
        forwardTransparentShader->setBool("hasAoMap", tm.material->hasAoMap);
        forwardTransparentShader->setBool("hasOpacityMap", tm.material->hasOpacityMap);
        forwardTransparentShader->setBool("hasAlphaMap", tm.material->hasAlphaMap);
        forwardTransparentShader->setBool("hasEmissiveMap", tm.material->hasEmissiveMap);
        forwardTransparentShader->setVec3("emissiveColor", tm.material->emissiveColor);
        forwardTransparentShader->setFloat("emissiveIntensity", tm.material->emissiveIntensity);

        // 绑定纹理（使用纹理单元 20-27，避免与阴影贴图冲突）
        if (tm.material->hasAlbedoMap) {
            glActiveTexture(GL_TEXTURE20);
            glBindTexture(GL_TEXTURE_2D, tm.material->albedoMapID);
            forwardTransparentShader->setInt("texture_albedo", 20);
        }
        if (tm.material->hasDiffuseMap) {
            glActiveTexture(GL_TEXTURE21);
            glBindTexture(GL_TEXTURE_2D, tm.material->diffuseMapID);
            forwardTransparentShader->setInt("texture_diffuse_pbr", 21);
        }
        if (tm.material->hasNormalMap) {
            glActiveTexture(GL_TEXTURE22);
            glBindTexture(GL_TEXTURE_2D, tm.material->normalMapID);
            forwardTransparentShader->setInt("texture_normal_pbr", 22);
        }
        if (tm.material->hasMetallicMap) {
            glActiveTexture(GL_TEXTURE23);
            glBindTexture(GL_TEXTURE_2D, tm.material->metallicMapID);
            forwardTransparentShader->setInt("texture_metallic", 23);
        }
        if (tm.material->hasRoughnessMap) {
            glActiveTexture(GL_TEXTURE24);
            glBindTexture(GL_TEXTURE_2D, tm.material->roughnessMapID);
            forwardTransparentShader->setInt("texture_roughness", 24);
        }
        if (tm.material->hasAoMap) {
            glActiveTexture(GL_TEXTURE25);
            glBindTexture(GL_TEXTURE_2D, tm.material->aoMapID);
            forwardTransparentShader->setInt("texture_ao", 25);
        }
        if (tm.material->hasOpacityMap) {
            glActiveTexture(GL_TEXTURE26);
            glBindTexture(GL_TEXTURE_2D, tm.material->opacityMapID);
            forwardTransparentShader->setInt("texture_opacity", 26);
        }
        if (tm.material->hasAlphaMap) {
            glActiveTexture(GL_TEXTURE27);
            glBindTexture(GL_TEXTURE_2D, tm.material->alphaMapID);
            forwardTransparentShader->setInt("texture_alpha", 27);
        }
        if (tm.material->hasEmissiveMap) {
            glActiveTexture(GL_TEXTURE28);
            glBindTexture(GL_TEXTURE_2D, tm.material->emissiveMapID);
            forwardTransparentShader->setInt("texture_emissive", 28);
        }

        // 绘制该mesh
        tm.mesh->Draw();
    }

    // 恢复OpenGL状态
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}

bool Renderer::loadHDREnvironment(const std::string& hdrPath) {
    // 清理旧的IBL资源
    cleanupIBL();

    // 加载HDR纹理
    unsigned int hdrTexture = loadHDRTexture(hdrPath);
    if (hdrTexture == 0) {
        return false;
    }

    // 确保立方体和捕获帧缓冲已创建
    if (cubeVAO == 0) setupCube();
    if (captureFBO == 0) setupCaptureFBO();

    // 生成立方体贴图
    generateCubemapFromEquirectangular(hdrTexture);

    // 生成辐照度贴图
    generateIrradianceMap();

    // 生成预滤波贴图
    generatePrefilterMap();

    // 生成BRDF查找表
    generateBRDFLUT();

    // 删除临时HDR纹理
    glDeleteTextures(1, &hdrTexture);

    std::cout << "IBL environment loaded successfully!" << std::endl;
    return true;
}

unsigned int Renderer::loadHDRTexture(const std::string& path) {
    stbi_set_flip_vertically_on_load(true);
    int width, height, nrComponents;
    float* data = stbi_loadf(path.c_str(), &width, &height, &nrComponents, 0);
    stbi_set_flip_vertically_on_load(false); // 立即恢复设置,避免影响其他纹理加载

    unsigned int hdrTexture = 0;
    if (data) {
        glGenTextures(1, &hdrTexture);
        glBindTexture(GL_TEXTURE_2D, hdrTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, data);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
        std::cout << "HDR texture loaded: " << path << std::endl;
    } else {
        std::cout << "Failed to load HDR image: " << path << std::endl;
    }

    return hdrTexture;
}

void Renderer::generateCubemapFromEquirectangular(unsigned int hdrTexture) {
    // 创建环境立方体贴图 (使用2048分辨率以保证高质量)
    glGenTextures(1, &envCubemap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);
    for (unsigned int i = 0; i < 6; ++i) {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, 2048, 2048, 0, GL_RGB, GL_FLOAT, nullptr);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // 设置投影和视图矩阵来渲染立方体的6个面
    glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
    glm::mat4 captureViews[] = {
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))
    };

    // 转换HDR等距柱状投影到立方体贴图
    equirectToCubemapShader->use();
    equirectToCubemapShader->setInt("equirectangularMap", 0);
    equirectToCubemapShader->setMat4("projection", captureProjection);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, hdrTexture);

    // 更新深度缓冲区大小以匹配2048x2048
    glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 2048, 2048);

    glViewport(0, 0, 2048, 2048);
    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
    for (unsigned int i = 0; i < 6; ++i) {
        equirectToCubemapShader->setMat4("view", captureViews[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, envCubemap, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glBindVertexArray(cubeVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // 生成mipmap
    glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

    glViewport(0, 0, screenWidth, screenHeight);
}

void Renderer::generateIrradianceMap() {
    glGenTextures(1, &irradianceMap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, irradianceMap);
    for (unsigned int i = 0; i < 6; ++i) {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, 64, 64, 0, GL_RGB, GL_FLOAT, nullptr);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
    glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 64, 64);

    irradianceShader->use();
    irradianceShader->setInt("environmentMap", 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);

    glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
    glm::mat4 captureViews[] = {
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))
    };

    irradianceShader->setMat4("projection", captureProjection);
    glViewport(0, 0, 64, 64);
    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
    for (unsigned int i = 0; i < 6; ++i) {
        irradianceShader->setMat4("view", captureViews[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, irradianceMap, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glBindVertexArray(cubeVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, screenWidth, screenHeight);
}

void Renderer::generatePrefilterMap() {
    glGenTextures(1, &prefilterMap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, prefilterMap);
    for (unsigned int i = 0; i < 6; ++i) {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, 512, 512, 0, GL_RGB, GL_FLOAT, nullptr);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

    prefilterShader->use();
    prefilterShader->setInt("environmentMap", 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);

    glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
    glm::mat4 captureViews[] = {
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))
    };
    prefilterShader->setMat4("projection", captureProjection);
    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);

    unsigned int maxMipLevels = 5;
    for (unsigned int mip = 0; mip < maxMipLevels; ++mip) {
        unsigned int mipWidth  = 512 * std::pow(0.5, mip);
        unsigned int mipHeight = 512 * std::pow(0.5, mip);
        glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, mipWidth, mipHeight);
        glViewport(0, 0, mipWidth, mipHeight);

        float roughness = (float)mip / (float)(maxMipLevels - 1);
        prefilterShader->setFloat("roughness", roughness);
        for (unsigned int i = 0; i < 6; ++i) {
            prefilterShader->setMat4("view", captureViews[i]);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, prefilterMap, mip);

            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            glBindVertexArray(cubeVAO);
            glDrawArrays(GL_TRIANGLES, 0, 36);
            glBindVertexArray(0);
        }
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, screenWidth, screenHeight);
}

void Renderer::generateBRDFLUT() {
    glGenTextures(1, &brdfLUTTexture);
    glBindTexture(GL_TEXTURE_2D, brdfLUTTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, 512, 512, 0, GL_RG, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
    glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 512, 512);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, brdfLUTTexture, 0);

    glViewport(0, 0, 512, 512);
    brdfShader->use();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, screenWidth, screenHeight);
}

void Renderer::cleanupIBL() {
    if (envCubemap != 0) {
        glDeleteTextures(1, &envCubemap);
        envCubemap = 0;
    }
    if (irradianceMap != 0) {
        glDeleteTextures(1, &irradianceMap);
        irradianceMap = 0;
    }
    if (prefilterMap != 0) {
        glDeleteTextures(1, &prefilterMap);
        prefilterMap = 0;
    }
    if (brdfLUTTexture != 0) {
        glDeleteTextures(1, &brdfLUTTexture);
        brdfLUTTexture = 0;
    }
}

void Renderer::renderSkyboxPass(Camera& camera) {
    if (envCubemap == 0) return;

    glDepthFunc(GL_LEQUAL);
    skyboxShader->use();

    glm::mat4 view = glm::mat4(glm::mat3(camera.GetViewMatrix())); // 移除平移
    glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom),
                                             (float)screenWidth / (float)screenHeight, 0.1f, 200.0f);
    skyboxShader->setMat4("view", view);
    skyboxShader->setMat4("projection", projection);

    // 设置全局曝光
    skyboxShader->setFloat("exposure", exposure);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);
    skyboxShader->setInt("environmentMap", 0);

    glBindVertexArray(cubeVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);

    glDepthFunc(GL_LESS);
}
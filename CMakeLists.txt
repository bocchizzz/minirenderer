cmake_minimum_required(VERSION 3.15)
project(Renderer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

if(MSVC)
    add_compile_options(/utf-8)
endif()

# 外部库路径
set(EXTERNAL_DIR ${CMAKE_SOURCE_DIR}/external)
set(GLFW_DIR ${EXTERNAL_DIR}/glfw)
set(ASSIMP_DIR ${EXTERNAL_DIR}/assimp)
set(GLM_DIR ${EXTERNAL_DIR}/glm)
set(GLAD_DIR ${EXTERNAL_DIR}/glad)
set(IMGUI_DIR ${EXTERNAL_DIR}/imgui)
set(STB_DIR ${EXTERNAL_DIR}/stb)
set(JSON_DIR ${EXTERNAL_DIR}/json)
# 查找 OpenGL
find_package(OpenGL REQUIRED)

# GLAD 源文件
set(GLAD_SOURCES
    ${GLAD_DIR}/src/glad.c
)

# ImGui 源文件
set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)

# 项目源文件
set(PROJECT_SOURCES
    src/main.cpp
    src/Camera.cpp
    src/Shader.cpp
    src/Model.cpp
    src/Material.cpp
    src/Controller.cpp
    src/SceneSerializer.cpp
    src/Renderer.cpp
    src/Frustum.cpp
)

# 创建可执行文件
add_executable(${PROJECT_NAME}
    ${PROJECT_SOURCES}
    ${GLAD_SOURCES}
    ${IMGUI_SOURCES}
)

# 包含目录
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${GLAD_DIR}/include
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
    ${STB_DIR}
    ${GLM_DIR}
    ${GLFW_DIR}/include
    ${ASSIMP_DIR}/include
    ${JSON_DIR}
    ${OPENGL_INCLUDE_DIR}
)

# 链接库目录
target_link_directories(${PROJECT_NAME} PRIVATE
    ${GLFW_DIR}/lib
    ${ASSIMP_DIR}/lib
)


# 链接库
target_link_libraries(${PROJECT_NAME}
    OpenGL::GL
    glfw3
    assimp-vc143-mt
    comdlg32
)

# 复制着色器文件到构建目录
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/shaders
    ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Release/shaders
)

# 创建 models 目录
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
    ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Release/models
)

# 复制 DLL 文件到输出目录（Windows）
if(WIN32)
    # 如果使用动态链接的 GLFW
    if(EXISTS ${GLFW_DIR}/lib/glfw3.dll)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${GLFW_DIR}/lib/glfw3.dll
            ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Release
        )
    endif()

    # 复制 Assimp DLL
    if(EXISTS ${ASSIMP_DIR}/bin/assimp-vc143-mt.dll)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${ASSIMP_DIR}/bin/assimp-vc143-mt.dll
            ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Release
        )
    endif()
endif()

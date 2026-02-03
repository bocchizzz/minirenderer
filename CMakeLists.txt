cmake_minimum_required(VERSION 3.10)
project(MiniRenderer CXX)

find_package(OpenCV REQUIRED)

set(CMAKE_CXX_STANDARD 17)

# 添加源文件
add_executable(MiniRenderer main.cpp load_model.cpp rasterizer.cpp texture.cpp)

include_directories(${PROJECT_SOURCE_DIR}/include, /usr/local/include, ./include)

target_link_libraries(MiniRenderer ${OpenCV_LIBRARIES})
#version 430 core
out vec4 FragColor;

in vec3 WorldPos;

uniform samplerCube environmentMap;
uniform float exposure;  // 全局曝光值

void main()
{
    vec3 envColor = texture(environmentMap, WorldPos).rgb;

    // HDR色调映射 (Exposure)
    envColor = vec3(1.0) - exp(-envColor * exposure);
    // Gamma校正
    envColor = pow(envColor, vec3(1.0/2.2));

    FragColor = vec4(envColor, 1.0);
}

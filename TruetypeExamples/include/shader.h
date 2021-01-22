#pragma once
#include <glad/glad.h>
#include <unordered_map>
#include <glm/matrix.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

class Shader
{
public:
	Shader() {}
	Shader(const const char* resourceName);

	void Compile(const char* filepath);
	void Bind();
	void Unbind();
	void Delete();

	void UploadVec4(const char* varName, const glm::vec4& vec4);
	void UploadVec3(const char* varName, const glm::vec3& vec3);
	void UploadVec2(const char* varName, const glm::vec2& vec2);
	void UploadFloat(const char* varName, float value);
	void UploadInt(const char* varName, int value);
	void UploadIntArray(const char* varName, int size, int* array);
	void UploadUInt(const char* varName, unsigned int value);

	void UploadMat4(const char* varName, const glm::mat4& mat4);
	void UploadMat3(const char* varName, const glm::mat3& mat3);

private:
	GLuint GetVariableLocation(const char* varName);

private:
	int m_ShaderProgram;
	bool m_BeingUsed;

	std::unordered_map<const char*, GLuint> m_Variables;
};
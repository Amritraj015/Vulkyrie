#include <iostream>
#include <fstream>
#include "renderer/shader.h"
#include "glad/glad.h"
#include "core/logger.h"

namespace Vulkyrie::Renderer {

	static std::string ReadTextFile(const std::filesystem::path& path)
	{
		std::ifstream file(path);

		if (!file.is_open())
		{
            VERROR("Failed to open file: {}", path.string());

			return {};
		}

		std::ostringstream contentStream;
		contentStream << file.rdbuf();

		return contentStream.str();
	}

	u32 CreateComputeShader(const std::filesystem::path& path)
	{
		std::string shaderSource = ReadTextFile(path);

		GLuint shaderHandle = glCreateShader(GL_COMPUTE_SHADER);

		const GLchar* source = (const GLchar*)shaderSource.c_str();
		glShaderSource(shaderHandle, 1, &source, 0);

		glCompileShader(shaderHandle);

		GLint isCompiled = 0;
		glGetShaderiv(shaderHandle, GL_COMPILE_STATUS, &isCompiled);

		if (GL_FALSE == isCompiled)
		{
			GLint maxLength = 0;
			glGetShaderiv(shaderHandle, GL_INFO_LOG_LENGTH, &maxLength);

			std::vector<GLchar> infoLog(maxLength);
			glGetShaderInfoLog(shaderHandle, maxLength, &maxLength, &infoLog[0]);

            VERROR("An error occurred while compiling compute shader:", infoLog.data());

			glDeleteShader(shaderHandle);
			return -1;
		}

		GLuint program = glCreateProgram();
		glAttachShader(program, shaderHandle);
		glLinkProgram(program);

		GLint isLinked = 0;
		glGetProgramiv(program, GL_LINK_STATUS, (int*)&isLinked);

		if (GL_FALSE == isLinked)
		{
			GLint maxLength = 0;
			glGetProgramiv(program, GL_INFO_LOG_LENGTH, &maxLength);

			std::vector<GLchar> infoLog(maxLength);
			glGetProgramInfoLog(program, maxLength, &maxLength, &infoLog[0]);

			std::cerr << infoLog.data() << std::endl;

			glDeleteProgram(program);
			glDeleteShader(shaderHandle);

			return -1;
		}

		glDetachShader(program, shaderHandle);
		return program;
	}

	u32 ReloadComputeShader(u32 shaderHandle, const std::filesystem::path& path)
	{
		u32 newShaderHandle = CreateComputeShader(path);

		// Return old shader if compilation failed
		if (newShaderHandle == -1)
			return shaderHandle;

		glDeleteProgram(shaderHandle);

		return newShaderHandle;
	}

	u32 CreateGraphicsShader(const std::filesystem::path& vertexPath, const std::filesystem::path& fragmentPath)
	{
		std::string vertexShaderSource = ReadTextFile(vertexPath);
		std::string fragmentShaderSource = ReadTextFile(fragmentPath);

		// Vertex shader

		GLuint vertexShaderHandle = glCreateShader(GL_VERTEX_SHADER);

		const GLchar* source = (const GLchar*)vertexShaderSource.c_str();
		glShaderSource(vertexShaderHandle, 1, &source, 0);

		glCompileShader(vertexShaderHandle);

		GLint isCompiled = 0;
		glGetShaderiv(vertexShaderHandle, GL_COMPILE_STATUS, &isCompiled);

		if (GL_FALSE == isCompiled)
		{
			GLint maxLength = 0;
			glGetShaderiv(vertexShaderHandle, GL_INFO_LOG_LENGTH, &maxLength);

			std::vector<GLchar> infoLog(maxLength);
			glGetShaderInfoLog(vertexShaderHandle, maxLength, &maxLength, &infoLog[0]);

			std::cerr << infoLog.data() << std::endl;

			glDeleteShader(vertexShaderHandle);
			return -1;
		}

		// Fragment shader

		GLuint fragmentShaderHandle = glCreateShader(GL_FRAGMENT_SHADER);

		source = (const GLchar*)fragmentShaderSource.c_str();
		glShaderSource(fragmentShaderHandle, 1, &source, 0);

		glCompileShader(fragmentShaderHandle);

		isCompiled = 0;
		glGetShaderiv(fragmentShaderHandle, GL_COMPILE_STATUS, &isCompiled);

		if (GL_FALSE == isCompiled)
		{
			GLint maxLength = 0;
			glGetShaderiv(fragmentShaderHandle, GL_INFO_LOG_LENGTH, &maxLength);

			std::vector<GLchar> infoLog(maxLength);
			glGetShaderInfoLog(fragmentShaderHandle, maxLength, &maxLength, &infoLog[0]);

            VERROR("An error occurred while compiling fragment shader:", infoLog.data());

			glDeleteShader(fragmentShaderHandle);

			return -1;
		}

		// Program linking

		GLuint program = glCreateProgram();
		glAttachShader(program, vertexShaderHandle);
		glAttachShader(program, fragmentShaderHandle);
		glLinkProgram(program);

		GLint isLinked = 0;
		glGetProgramiv(program, GL_LINK_STATUS, (int*)&isLinked);

		if (GL_FALSE == isLinked)
		{
			GLint maxLength = 0;
			glGetProgramiv(program, GL_INFO_LOG_LENGTH, &maxLength);

			std::vector<GLchar> infoLog(maxLength);
			glGetProgramInfoLog(program, maxLength, &maxLength, &infoLog[0]);

            VERROR("An error occurred while linking graphics shader program:", infoLog.data());

			glDeleteProgram(program);
			glDeleteShader(vertexShaderHandle);
			glDeleteShader(fragmentShaderHandle);

			return -1;
		}

		glDetachShader(program, vertexShaderHandle);
		glDetachShader(program, fragmentShaderHandle);

		return program;
	}

	u32 ReloadGraphicsShader(u32 shaderHandle, const std::filesystem::path& vertexPath, const std::filesystem::path& fragmentPath)
	{
		u32 newShaderHandle = CreateGraphicsShader(vertexPath, fragmentPath);

		// Return old shader if compilation failed
		if (newShaderHandle == -1)
			return shaderHandle;

		glDeleteProgram(shaderHandle);

		return newShaderHandle;
	}

}
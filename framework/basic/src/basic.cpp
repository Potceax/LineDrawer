#include "basic.h"

#include <fstream>
#include <sstream>
#include <vector>
#include <algorithm>
#include <functional>
#include <filesystem>
#include <string_view>

namespace Basic
{
	// ===================================================================================================================
	// CLASSES
	// ===================================================================================================================
	Path::Path(const std::string& dataPathIn) :
		m_dataPath{ dataPathIn }
	{
	}
	U64 Path::IncludePath(const std::string& pathIn)
	{
		std::hash<std::string> hash;

		auto key{ hash(pathIn) };
		m_relativePaths[key] = pathIn;

		return key;
	}
	bool Path::GetPath(U64 key, std::string& pathOut) const
	{
		auto found{ m_relativePaths.find(key) };
		if (found != m_relativePaths.end())
		{
			pathOut = found->second;
			return true;
		}

		return false;
	}

	void Path::SetExecutionPath(const std::string& pathIn)
	{
		m_executionPath = pathIn;
	}

	void Path::SetDataPath(const std::string& pathIn)
	{
		m_dataPath = pathIn;
	}

	bool Path::FindFileOrThrow(const std::string& fileNameIn, std::string& foundFileOut) const
	{
		// first look for the file in specified main path/relative paths
		// ../Data/ path search
		std::string pathToFind{ m_dataPath + fileNameIn };
		std::string execPathToFind{ m_executionPath + fileNameIn };
		bool hasFound{ false };
		try
		{
			std::ifstream file{ pathToFind };
			std::ifstream execFile{ execPathToFind };
			if (file.is_open())
			{
				foundFileOut = pathToFind;
				hasFound = true;
			}
			else if (execFile.is_open())
			{
				foundFileOut = execPathToFind;
				hasFound = true;
			}
			else
			{
				for (const auto& elem : m_relativePaths)
				{
					// first check for relative path == absolute path
					pathToFind = elem.second + fileNameIn;
					file.open(pathToFind);
					if (file.is_open())
					{
						foundFileOut = pathToFind;
						hasFound = true;
						break;
					}


					// then check for relative path == actual relative path
					pathToFind = m_dataPath + elem.second + fileNameIn;
					execPathToFind = m_executionPath + elem.second + fileNameIn;

					file.open(pathToFind);
					execFile.open(execPathToFind);
					if (file.is_open())
					{
						foundFileOut = pathToFind;
						hasFound = true;
						break;
					}
					else if (execFile.is_open())
					{
						foundFileOut = execPathToFind;
						hasFound = true;
						break;
					}

				}
			}

			if (!hasFound)
			{
				throw std::runtime_error(std::string("Cannot Find " + fileNameIn + " in specified paths."));
			}

			// return found file
			return true;
		}
		catch (std::exception& e)
		{
			printf("%s\n", e.what());
		}

		// default return
		return false;
	}

	bool Path::FindFileRecursivelyOrThrow(const std::string& fileNameIn, std::string& foundFileOut) const
	{
		namespace fs = std::filesystem;

		try
		{
			// First search the main data folder
			std::string pathToFind{ m_dataPath + fileNameIn };
			std::ifstream file{ pathToFind };
			if (file.is_open())
			{
				foundFileOut = pathToFind;
				file.close();
				return true;
			}

			// Find all directories inside main data
			for (const auto& dir : fs::recursive_directory_iterator(m_dataPath))
			{
				if (!dir.is_directory())
				{
					continue;
				}

				// and search each for the needed file
				pathToFind = dir.path().string() + "/" + std::string(fileNameIn);
				file.open(pathToFind);
				if (file.is_open())
				{
					foundFileOut = pathToFind;
					file.close();
					return true;
				}
			}

			throw std::runtime_error("Cannot find " + std::string(fileNameIn) + " in " + m_dataPath);
		}
		catch (std::exception& e)
		{
			printf("%s\n", e.what());
		}

		return false;
	}

	System::System() :
		m_paths{ DATA_PATH }
	{
		//// build paths
		//m_paths.IncludePath(MESH_PATH);
		//m_paths.IncludePath(TEXTURE_PATH);
		//m_paths.IncludePath(SHADER_PATH);

		// install paths
		m_paths.IncludePath(INSTALL_MESH_PATH);
		m_paths.IncludePath(INSTALL_TEXTURE_PATH);
		m_paths.IncludePath(INSTALL_SHADER_PATH);
	}

	GLuint System::CreateShader(const std::string& shaderName, GLenum shaderType)
	{
		// read whole file to ss
		std::string foundPath;
		if (!m_paths.FindFileOrThrow(shaderName, foundPath))
		{
			return 0;
		}

		std::ifstream file{ foundPath };
		std::ostringstream ss{ };
		ss << file.rdbuf();
		std::string str{ ss.str() };
		file.close();

		const GLchar* source{ str.c_str() };
		GLuint shader = glCreateShader(shaderType);
		glShaderSource(shader, 1, &source, NULL);
		glCompileShader(shader);

		int status;
		glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
		if (status == GL_FALSE)
		{
			int length;
			glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);

			std::string shaderTypeName;
			switch (shaderType)
			{
			case GL_VERTEX_SHADER:
				shaderTypeName = "Vertex";
				break;
			case GL_FRAGMENT_SHADER:
				shaderTypeName = "Fragment";
				break;
			case GL_GEOMETRY_SHADER:
				shaderTypeName = "Geometry";
				break;
			case GL_COMPUTE_SHADER:
				shaderTypeName = "Compute";
				break;
			}

			char* log{ new char[length + 1] };
			glGetShaderInfoLog(shader, length, NULL, log);
			fprintf(stderr, "Cannot compile %s shader\n%s\n", shaderTypeName.c_str(), log);

			delete[] log;
		}

		return shader;
	}

	GLuint System::LoadProgram(const std::string& VertexShaderStr, const std::string& FragmentShaderStr)
	{
		std::vector<GLuint> shaders{};
		shaders.push_back(CreateShader(VertexShaderStr, GL_VERTEX_SHADER));
		shaders.push_back(CreateShader(FragmentShaderStr, GL_FRAGMENT_SHADER));

		GLuint program = glCreateProgram();
		std::for_each(shaders.begin(), shaders.end(), [&](GLuint& a) { glAttachShader(program, a); });
		glLinkProgram(program);

		int status{  };
		glGetProgramiv(program, GL_LINK_STATUS, &status);
		if (status == GL_FALSE)
		{
			int length;
			glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);

			char* log{ new char[length + 1] };
			glGetProgramInfoLog(program, length, NULL, log);
			fprintf(stderr, "%s\n", log);

			delete[] log;
		}

		std::for_each(shaders.begin(), shaders.end(), [&](GLuint& a) { glDetachShader(program, a); glDeleteShader(a); });

		return program;
	}

	GLuint System::LoadProgram(std::initializer_list<ShaderInfo> list)
	{
		std::vector<U32> shaders{};
		for (const auto& elem : list)
		{
			U32 currentShader{ CreateShader(elem.path, elem.type) };
			if (currentShader != 0)
			{
				shaders.emplace_back(currentShader);
			}
		}

		GLuint program = glCreateProgram();
		std::for_each(shaders.begin(), shaders.end(), [&](GLuint& a) { glAttachShader(program, a); });
		glLinkProgram(program);

		int status{  };
		glGetProgramiv(program, GL_LINK_STATUS, &status);
		if (status == GL_FALSE)
		{
			int length;
			glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);

			char* log{ new char[length + 1] };
			glGetProgramInfoLog(program, length, NULL, log);
			fprintf(stderr, "%s\n", log);

			delete[] log;
		}

		std::for_each(shaders.begin(), shaders.end(), [&](GLuint& a) { glDetachShader(program, a); glDeleteShader(a); });

		return program;

	}

	void System::SetPath(const char* pathIn)
	{
		auto path{ std::filesystem::weakly_canonical(std::filesystem::path(pathIn).parent_path()).generic_string() };
		m_paths.SetExecutionPath(path + '/');
	}

	// ===================================================================================================================
	// GLOBAL FUNCTIONS
	// ===================================================================================================================
	std::string FindFileOrThrow(const std::string& filePath)
	{
		// ../Data/ path search
		std::string pathToFind{ DATA_PATH + filePath }; // THIS HAS TO CHANGE TO DATA_PATH TODO:
		try
		{
			std::ifstream file{ pathToFind };
			if (!file.is_open())
			{
				throw std::runtime_error(std::string("Cannot Find " + filePath + " in: ..\\" + DATA_PATH));
			}

			// return found file
			return pathToFind;
		}
		catch (std::exception& e)
		{
			printf("%s\n", e.what());
		}

		// default return
		return std::string();
	}

	std::string FindFileOrThrow(const std::string_view& filePath)
	{
		namespace fs = std::filesystem;

		try
		{
			// First search the ../data/ folder
			std::string pathToFind{ filePath };
			std::ifstream file{ pathToFind };
			if (file.is_open())
			{
				file.close();
				return pathToFind;
			}

			// Find all directories inside ../data/

			for (const auto& dir : fs::recursive_directory_iterator(DATA_PATH))
			{
				if (!dir.is_directory())
				{
					continue;
				}

				// and search each for the needed file
				pathToFind = dir.path().string() + "\\" + std::string(filePath);
				file.open(pathToFind);
				if (file.is_open())
				{
					file.close();
					return pathToFind;
				}
			}

			throw std::runtime_error("Cannot find " + std::string(filePath) + " in " + DATA_PATH);
		}
		catch (std::exception& e)
		{
			printf("%s\n", e.what());
		}

		return std::string();
	}

	// to create a shader I need to -> find the file 
	GLuint CreateShader(const std::string& shaderName, GLenum shaderType)
	{
		// read whole file to ss
		std::ifstream file{ FindFileOrThrow(shaderName) };
		std::ostringstream ss{ };
		ss << file.rdbuf();
		std::string str{ ss.str() };
		file.close();

		const GLchar* source{ str.c_str() };
		GLuint shader = glCreateShader(shaderType);
		glShaderSource(shader, 1, &source, NULL);
		glCompileShader(shader);

		int status;
		glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
		if (status == GL_FALSE)
		{
			int length;
			glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);

			std::string shaderTypeName;
			switch (shaderType)
			{
			case GL_VERTEX_SHADER:
				shaderTypeName = "Vertex";
				break;
			case GL_FRAGMENT_SHADER:
				shaderTypeName = "Fragment";
				break;
			case GL_GEOMETRY_SHADER:
				shaderTypeName = "Geometry";
				break;
			case GL_TESS_CONTROL_SHADER:
				shaderTypeName = "Tesselation Control";
				break;
			case GL_TESS_EVALUATION_SHADER:
				shaderTypeName = "Tesselation Evaluation";
				break;
			case GL_COMPUTE_SHADER:
				shaderTypeName = "Compute";
				break;
			}

			char* log{ new char[length + 1] };
			glGetShaderInfoLog(shader, length, NULL, log);
			fprintf(stderr, "Cannot compile %s shader\n%s\n", shaderTypeName.c_str(), log);

			delete[] log;
		}

		return shader;
	}

	GLuint LoadProgram(const std::string& VertexShaderStr, const std::string& FragmentShaderStr)
	{
		std::vector<GLuint> shaders{};
		shaders.push_back(CreateShader(VertexShaderStr, GL_VERTEX_SHADER));
		shaders.push_back(CreateShader(FragmentShaderStr, GL_FRAGMENT_SHADER));

		GLuint program = glCreateProgram();
		std::for_each(shaders.begin(), shaders.end(), [&](GLuint& a) { glAttachShader(program, a); });
		glLinkProgram(program);

		int status{  };
		glGetProgramiv(program, GL_LINK_STATUS, &status);
		if (status == GL_FALSE)
		{
			int length;
			glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);

			char* log{ new char[length + 1] };
			glGetProgramInfoLog(program, length, NULL, log);
			fprintf(stderr, "%s\n", log);

			delete[] log;
		}

		std::for_each(shaders.begin(), shaders.end(), [&](GLuint& a) { glDetachShader(program, a); glDeleteShader(a); });

		return program;
	}

	float fovRad(float DegValue)
	{
		const float circleC{ 3.14159f * 2.f };
		const float Rad{ circleC * DegValue / 360.f };

		return 1.f / tan(Rad / 2.f);
	}

	//void BindMotion(void(*func)(int x, int y))
	//{
	//	glutMotionFunc(func);
	//}

	void APIENTRY
		DebugCallback(
			GLenum source,
			GLenum type,
			GLuint id,
			GLenum severity,
			GLsizei length,
			const GLchar* message,
			const void* userParam)
	{
		switch (id) // Nvidia error
		{
		case 131185: // glBufferData
			return;
		}

		fprintf(stderr, "-------------------------\nError message: %s\nSource: ", message);

		switch (source)
		{
		case GL_DEBUG_SOURCE_API:
			fprintf(stderr, "API\n");
			break;
		case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
			fprintf(stderr, "Window\n");
			break;
		case GL_DEBUG_SOURCE_SHADER_COMPILER:
			fprintf(stderr, "Shader\n");
			break;
		case GL_DEBUG_SOURCE_THIRD_PARTY:
			fprintf(stderr, "Third party\n");
			break;
		case GL_DEBUG_SOURCE_APPLICATION:
			fprintf(stderr, "Application\n");
			break;
		case GL_DEBUG_SOURCE_OTHER:
			fprintf(stderr, "Other\n");
			break;
		}

		fprintf(stderr, "Type: ");
		switch (type)
		{
		case GL_DEBUG_TYPE_ERROR:
			fprintf(stderr, "Error\n");
			break;
		case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
			fprintf(stderr, "Depricated behavior\n");
			break;
		case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
			fprintf(stderr, "Undefined behavior\n");
			break;
		case GL_DEBUG_TYPE_PORTABILITY:
			fprintf(stderr, "Portability\n");
			break;
		case GL_DEBUG_TYPE_PERFORMANCE:
			fprintf(stderr, "Performance\n");
			break;
		case GL_DEBUG_TYPE_MARKER:
			fprintf(stderr, "Marker\n");
			break;
		case GL_DEBUG_TYPE_PUSH_GROUP:
			fprintf(stderr, "Push group\n");
			break;
		case GL_DEBUG_TYPE_POP_GROUP:
			fprintf(stderr, "Pop group\n");
			break;
		case GL_DEBUG_TYPE_OTHER:
			fprintf(stderr, "Other\n");
			break;
		}

		fprintf(stderr, "ID: %d\n", id);

		fprintf(stderr, "Severity: ");
		switch (severity)
		{
		case GL_DEBUG_SEVERITY_LOW:
			fprintf(stderr, "Low\n");
			break;
		case GL_DEBUG_SEVERITY_MEDIUM:
			fprintf(stderr, "Medium\n");
			break;
		case GL_DEBUG_SEVERITY_HIGH:
			fprintf(stderr, "High\n");
			break;
		case GL_DEBUG_SEVERITY_NOTIFICATION:
			fprintf(stderr, "Notification\n");
			break;
		}
		fprintf(stderr, "\n");
	}
	// ===================================================================================================================

}
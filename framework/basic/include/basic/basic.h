#ifndef BASIC_H
#define BASIC_H
#include <string>
#include <map>
#include <gl_4_3.h>
#include <gl_load.hpp>
#include <functional>

namespace Basic
{
	using U8 = unsigned char;
	using U16 = unsigned short;
	using U32 = unsigned;
	using U64 = unsigned long long;

	// ===================================================================================================================
	// CLASSES
	// ===================================================================================================================

	struct ShaderInfo
	{
		std::string path;
		U32 type;
	};

	/** OOP version of file/path finding
	  * class requires a main path for look up, additional paths can be added to look up*/
	class Path
	{
	public:
		Path(const std::string& dataPathIn);

		// add path to map, return the hash key
		U64 IncludePath(const std::string& path);

		// get path string to relative paths
		bool GetPath(U64 key, std::string& pathOut) const;

		void SetDataPath(const std::string& pathIn);
		void SetExecutionPath(const std::string& pathIn);

		//specified location version
		bool FindFileOrThrow(const std::string& fileNameIn, std::string& foundFileOut) const;

		// recursive version
		bool FindFileRecursivelyOrThrow(const std::string& fileNameIn, std::string& foundFileOut) const;

	private:
		std::string m_dataPath;
		std::string m_executionPath;
		std::map<U64, std::string> m_relativePaths;
	};

	class System
	{
	public:
		System();

		static System& Get()
		{
			static System instance;
			return instance;
		}

		GLuint CreateShader(const std::string& shaderName, GLenum shaderType);
		GLuint LoadProgram(const std::string& VertexShaderStr, const std::string& FragmentShaderStr);
		GLuint LoadProgram(std::initializer_list<ShaderInfo> list);

		inline const Path& GetPathFinder() const { return m_paths; }

		// THIS IS TEMP SOLUTION
		// set execution path for paths member
		void SetPath(const char* pathIn);

	private:
		explicit System(const System& other);
		System& operator=(const System& other) = delete;

		Path m_paths;
	};
	// ===================================================================================================================
	// GLOBAL FUNCTIONS
	// ===================================================================================================================

	// Find file in the DATA_PATH directory (without recursion)
	std::string FindFileOrThrow(const std::string& filePath);

	// Find file inside DATA_PATH directory (with recursion)
	std::string FindFileOrThrow(const std::string_view& filePath); // TEMP: change the overload arg function for lessening confusion

	GLuint CreateShader(const std::string& shaderName, GLenum shaderType);
	GLuint LoadProgram(const std::string& VertexShaderStr, const std::string& FragmentShaderStr);
	float fovRad(float DegValue);

	// Function binders
	//void BindMotion(void(*func)(int x, int y));

	void APIENTRY DebugCallback(
		GLenum source,
		GLenum type,
		GLuint id,
		GLenum severity,
		GLsizei length,
		const GLchar* message,
		const void* userParam);
}
#endif // !BASIC_H
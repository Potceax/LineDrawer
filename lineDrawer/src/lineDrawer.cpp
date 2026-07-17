#include "lineDrawer.h"
#include <iostream>

#include <gl_4_3.h>
#include <gl_load.hpp>
#include <GL/freeglut.h>

#include <glm/glm.hpp>

#include "framework.h"
#include "basic.h"

// Mouse Key enum (in order of GLUT mouse input func)
enum EMouseButton
{
	TOUCH_L_MOUSE,
	TOUCH_MID_MOUSE,
	TOUCH_R_MOUSE,
	TOUCH_MOUSE_MAX
};

class MouseManip
{
public:
	MouseManip(EMouseButton buttonIn) :
		m_bIsHeld{ false }, m_mousePosition{ 0,0 }, m_mouseButton{ buttonIn }
	{}

	void MouseMotion(int x, int y)
	{
		if (true)
		{
			const glm::ivec2 currentPosition{ x, y };

			// update starting mouse position
			m_mousePosition = currentPosition;
		}
	}

	void MouseMovement(int buttonType, int state, int x, int y)
	{
		// 0 -> started (hold), 1 -> ended (released)

		if (buttonType == m_mouseButton)
		{
			m_bIsHeld = !state;
			//std::cout << state << '\n';
		}

		//if (true) // set start mouse position to current one
		//{
		//	m_mousePosition = glm::ivec2(x, y);
		//}
	}

	/** Get screen space mouse position 
	  * IMPORTANT: the start point is Top Left corner */
	const glm::ivec2& GetMousePosition() const { return m_mousePosition; }
private:
	bool m_bIsHeld;
	glm::ivec2 m_mousePosition;
	EMouseButton m_mouseButton;
};

std::unique_ptr<MouseManip> g_mouseManipPtr;

/** Mouse Bindings */
namespace
{
	void MouseMotion(int x, int y)
	{
		g_mouseManipPtr->MouseMotion(x, y);
		glutPostRedisplay();
	}

	void MouseMovement(int buttonType, int state, int x, int y)
	{
		g_mouseManipPtr->MouseMovement(buttonType, state, x, y);
		glutPostRedisplay();
	}
}

/** Defines single vertex for Line class */
struct LineVertex
{
	glm::vec4 color{};
	glm::vec2 position{};
	glm::vec2 direction{};
	Basic::U32 isCurve{};
};

/** defines ssbo storage for last part of arc direction */
struct LineSSBO
{
	glm::vec2 direction{};
	float padding[2]{};
};

/** Class responsible for storing and drawing lines/arcs */
class Line
{
	/** Helper class resposible for controlling selected Vertex and its attributes */
	template<class Vert>
	class VertControl
	{
	public:
		VertControl(Vert* vertPtrIn = nullptr) :
			m_controledVertex{ vertPtrIn }
		{
		}

		/** change the controlled vertex */
		void SetRef(Vert* vertPtrIn)
		{
			m_controledVertex = vertPtrIn;
		}

		/** update values of controlled vertex (copy) */
		void Update(const Vert& otherVert)
		{
			assert(m_controledVertex != nullptr);

			*m_controledVertex = otherVert;
		}

		/** update values of controlled vertex (attributes) [Just be caucious...] */
		template<typename... Args>
		void Update(Args... args)
		{
			assert(m_controledVertex != nullptr);

			*m_controledVertex = Vert{ args... };
		}

		const Vert& GetVert() const { return *m_controledVertex; }
		bool IsEmpty() const { return m_controledVertex == nullptr; }

	private:
		Vert* m_controledVertex{ nullptr };
	};

public:
	// VERTEX ATTRIBUTE INDICIES
	// ========================================
	const static Basic::U32 s_positionIndex{ 0 };
	const static Basic::U32 s_colorIndex{ 1 };
	const static Basic::U32 s_directionIndex{ 2 };
	const static Basic::U32 s_flagIndex{ 3 };
	// ========================================

	// SSBO BINDING
	// ========================================
	const static Basic::U32 s_SSBOBindIndex{ 0 };
	// ========================================

	Line(Basic::U64 sizeIn = 0) :
		m_data(sizeIn), m_size{ sizeIn }
	{}

	/** Create buffers */
	void Setup()
	{
		glGenVertexArrays(1, &m_VAO);

		// Set init buffer size
		glGenBuffers(1, &m_VBO);
		glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(LineVertex) * m_size, NULL, GL_STREAM_DRAW);

		// push buffer attribute info to vertex array object
		glBindVertexArray(m_VAO);

		glEnableVertexAttribArray(s_positionIndex);
		glEnableVertexAttribArray(s_colorIndex);
		glEnableVertexAttribArray(s_directionIndex);
		glEnableVertexAttribArray(s_flagIndex);

		/** set attrib read positions from the LineVertex struct */
		glVertexAttribPointer(s_positionIndex, 2, GL_FLOAT, GL_FALSE, sizeof(LineVertex), (GLvoid*)sizeof(glm::vec4));
		glVertexAttribPointer(s_colorIndex, 4, GL_FLOAT, GL_FALSE, sizeof(LineVertex), (GLvoid*)0);
		glVertexAttribPointer(s_directionIndex, 2, GL_FLOAT, GL_FALSE, sizeof(LineVertex), (GLvoid*)(sizeof(glm::vec4) + sizeof(glm::vec2)));
		glVertexAttribIPointer(s_flagIndex, 1, GL_UNSIGNED_INT, sizeof(LineVertex), (GLvoid*)(sizeof(glm::vec4) + 2 * sizeof(glm::vec2)));
		glBindVertexArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		/** SSBO setup */
		glGenBuffers(1, &m_SSBO);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_SSBO);
		glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(LineSSBO), NULL, GL_DYNAMIC_DRAW);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, s_SSBOBindIndex, m_SSBO);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
	}

	/** Update current vertex information, update buffers */
	void Update(const glm::vec2& posIn)
	{	
		// update controller TODO: Pack it up in one func to not create so much useless data
		UpdateCurrent(posIn);
		UpdateCurrentArc();
		UpdateCurrentDirection();
			
		// update buffer
		if (/*b_hasChanged*/true) // the controller will change the last value anyway, no need for check
		{
			glBindBuffer(GL_ARRAY_BUFFER, m_VBO);

			glBufferData(GL_ARRAY_BUFFER, sizeof(LineVertex) * m_size, m_data.data(), GL_STREAM_DRAW);
			glBindBuffer(GL_ARRAY_BUFFER, 0);
		}
	}

	// IMPORTANT: this template is temporary, remove it when finishing project
	/** Pass the buffers, bind program and draw lines */
	template<typename Program>
	void Draw(const Program& programIn)
	{
		glUseProgram(programIn.program);

		// uniform update
		glUniform1i(programIn.numberOfPartsUnif, 32);

		// SSBO bind
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_SSBO);

		glBindVertexArray(m_VAO);
		glDrawArrays(GL_LINE_STRIP, 0, m_size);
		glBindVertexArray(0);

		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

		glUseProgram(0);
	}

	/** Add new vert, set it as controlled */
	void AddVert(const LineVertex& vertIn) 
	{
		m_data.emplace_back(vertIn);
		m_size = m_data.size();

		if (m_size <= 1)
		{
			// add extra vert to start drawing
			m_data.emplace_back(vertIn);
			m_size++;
		}

		// set the arc/line for current vertex
		m_data.back().isCurve = m_bIsArc;

		// update/create controller
		if (m_bIsInitVert)
		{
			m_currentVertControllerPtr.reset(new VertControl<LineVertex>(&m_data[m_size - 1]));
			m_bIsInitVert = false;
		}
		else
		{
			m_currentVertControllerPtr->SetRef(&m_data[m_size - 1]);
		}

		m_bHasChanged = true;
	};

	/** Get rid of the last vert in the data array, set controlled vert to previous */
	void RemoveVert()
	{
		// do not allow empty pop
		if (m_data.empty())
		{
			return;
		}

		// remove first line entirely
		if (m_size == 2)
		{
			// clear data
			m_data.clear();
			m_size = 0;
			m_bIsArc = false;
		}
		else
		{
			m_data.pop_back();
			m_size--;
		}
		
		// update controller
		SyncCurrent();

		m_bHasChanged = true;
	}

	/** use undo/redo buffer to get all stored verticles back to data container */
	void Redo()
	{
		// do not redo from empty data
		if (m_data.empty() || m_size == m_data.size())
		{
			return;
		}

		m_size++;

		SyncCurrent();

		m_bHasChanged = true;
	}

	/** bool switch from arc to line and vice versa */
	void ToArc()
	{
		// can modify with at least 1 line
		if (m_size <= 2)
		{
			return;
		}

		m_bIsArc = true;
		m_bHasChanged = true;
	}
	void ToLine()
	{
		// can modify with at least 1 line
		if (m_size <= 2)
		{
			return;
		}

		m_bIsArc = false;
		m_bHasChanged = true;
	}

protected:
	/** Update position attribute for current vert */
	void UpdateCurrent(const glm::vec2& posIn)
	{
		if (m_currentVertControllerPtr == nullptr || m_currentVertControllerPtr->IsEmpty())
		{
			return;
		}

		LineVertex currentVertValue{ m_currentVertControllerPtr->GetVert() };
		currentVertValue.position = posIn;
		m_currentVertControllerPtr->Update(currentVertValue);
	}

	/** Update isCurve attribute for current vert */
	void UpdateCurrentArc()
	{
		if (m_currentVertControllerPtr == nullptr || m_currentVertControllerPtr->IsEmpty())
		{
			return;
		}

		LineVertex currentVertValue{ m_currentVertControllerPtr->GetVert() };
		currentVertValue.isCurve = m_size <= 2 ? 0 : (Basic::U32)(m_bIsArc);

		m_currentVertControllerPtr->Update(currentVertValue);
	}
	
	/** Update current vert direction (last to current) and line direction (from last) */
	void UpdateCurrentDirection()
	{
		if (m_currentVertControllerPtr == nullptr || m_currentVertControllerPtr->IsEmpty())
		{
			return;
		}

		// split cases into "Begining of line" and "middle of line"
		LineVertex currentVertValue{ m_currentVertControllerPtr->GetVert() };
		if (m_size <= 2) // "Begining of line" (only 2 verts) - set the same direction for both verticies
		{
			LineVertex& previousVertValue{ m_data[m_size - 2] };
			previousVertValue.direction = currentVertValue.direction = glm::normalize(currentVertValue.position - previousVertValue.position);
		}
		else // "Middle of line" (more than 2 verts) - calculate direction only for current vertex
		{
			const LineVertex& previousVertValue{ m_data[m_size - 2] };
			const LineVertex& previousPreviousVertValue{ m_data[m_size - 3] };

			// arc check
			//if (currentVertValue.isCurve) // SSBO reading has problems with NAN and flickering (dont know if this is a good idea) 
			//{
			//	glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_SSBO);
			//	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

			//	LineSSBO temp{};
			//	glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(LineSSBO), &temp);
			//	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
			//	currentVertValue.direction = glm::normalize(temp.direction);

			//	if (glm::isnan(currentVertValue.direction.x))
			//	{
			//		currentVertValue.direction = glm::normalize(previousVertValue.position - previousPreviousVertValue.position);
			//	}
			//}
			//else
			//{
			//	currentVertValue.direction = glm::normalize(previousVertValue.position - previousPreviousVertValue.position);
			//}

			currentVertValue.direction = glm::normalize(previousVertValue.position - previousPreviousVertValue.position);
		}

		//std::cout << currentVertValue.direction.x << ' ' << currentVertValue.direction.y << '\n';

		// change current vert value
		m_currentVertControllerPtr->Update(currentVertValue);
	}

	/** Synchronize VertControl with vertex array */
	void SyncCurrent()
	{
		if (m_currentVertControllerPtr == nullptr)
		{
			return;
		}

		if (m_data.empty())
		{
			m_currentVertControllerPtr->SetRef(nullptr);
		}
		else
		{
			m_currentVertControllerPtr->SetRef(&m_data[m_size - 1]);
		}

	}

private:
	// Buffers
	GLuint m_VAO{};
	GLuint m_VBO{};

	// buffer for last part dir storage
	GLuint m_SSBO{};

	// last vertex controller
	std::unique_ptr<VertControl<LineVertex>> m_currentVertControllerPtr{ nullptr };

	// vertex data
	std::vector<LineVertex> m_data{};
	Basic::U64 m_size{};

	// flags
	bool m_bIsInitVert{ true };
	bool m_bHasChanged{ false };
	bool m_bIsArc{ false };
};

std::unique_ptr<Line> g_lineDrawPtr;

struct LineDrawProgram
{
	GLuint program{};
	GLint numberOfPartsUnif{};
};

LineDrawProgram g_program;

void InitProgram(const std::string& vShader, const std::string& gShader, const std::string& fShader)
{
	LineDrawProgram data{};
	data.program = Basic::System::Get().LoadProgram(
		{
			Basic::ShaderInfo{ vShader, GL_VERTEX_SHADER },
			Basic::ShaderInfo{ gShader, GL_GEOMETRY_SHADER },
			Basic::ShaderInfo{ fShader, GL_FRAGMENT_SHADER }
		}
	);

	data.numberOfPartsUnif = glGetUniformLocation(data.program, "numberOfParts");

	g_program = data;
}

struct ScreenCoords
{
	glm::ivec2 size;
	glm::ivec2 mousePos;
	glm::vec2 NDC;
};

ScreenCoords g_coords;

/** Class responsible for the context menu, it adds options and render the context box to screen */
template<typename T>
class MenuManip
{
	// generic elem representation (name, func)
	struct ContextElem
	{
		std::string desc;
		std::function<T> func;
	};

public:
	MenuManip() = default;

	// non-class function only
	template<typename fn>
	void AddOption(Basic::U32 keyIn, const std::string& desc, fn funcIn)
	{
		m_options[keyIn] = ContextElem{ desc, funcIn };
	}

	// class function only
	template<typename T, typename P>
	void AddOption(Basic::U32 keyIn, const std::string& desc, P* ptrIn, T(P::*funcIn)())
	{
		m_options[keyIn] = ContextElem{ desc, [ptrIn, funcIn]()->T { return (ptrIn->*funcIn)(); } };
	}

	/** Look for the elem and call its function.
	  * Use as a glut bind point for the menu */
	void Menu(int key)
	{
		auto found{ m_options.find((Basic::U32)key) };
		if (found != m_options.end())
		{
			found->second.func();
		}

		glutPostRedisplay();
	}

	/** Create the context menu */
	void Setup()
	{
		// collect elements and pass them as entries
		for (const auto& elem : m_options)
		{
			const auto& contextElem{ elem.second };
			glutAddMenuEntry(contextElem.desc.c_str(), elem.first);
		}

		// Render menu with right mouse button
		glutAttachMenu(GLUT_RIGHT_BUTTON);
	}
	
private:
	std::map<Basic::U32, ContextElem> m_options;
};

std::unique_ptr<MenuManip<void()>> g_menuManipPtr{nullptr};

// example context window
void menu(int item)
{
	if (g_menuManipPtr)
	{
		g_menuManipPtr->Menu(item);
	}
}

void InitMenu()
{
	glutCreateMenu(menu);

	g_menuManipPtr.reset(new MenuManip<void()>);
	//g_menuManipPtr->AddOption(1, "first", []() { std::cout << "sandwitch."; });
	g_menuManipPtr->AddOption(2, "undo", g_lineDrawPtr.get(), &Line::RemoveVert);
	g_menuManipPtr->AddOption(3, "Arc", g_lineDrawPtr.get(), &Line::ToArc);
	g_menuManipPtr->AddOption(4, "Line", g_lineDrawPtr.get(), &Line::ToLine);

	g_menuManipPtr->Setup();
}

void init() 
{
	// Activate debug messages
#ifdef _DEBUG
	int debugFlag{};
	glGetIntegerv(GL_CONTEXT_FLAGS, &debugFlag);
	if (debugFlag & GL_CONTEXT_FLAG_DEBUG_BIT)
	{
		if (glext_ARB_debug_output)
		{
			glEnable(GL_DEBUG_CALLBACK_FUNCTION_ARB);
			glDebugMessageCallbackARB(Basic::DebugCallback, NULL);
		}
	}
#endif

	// create Program
	InitProgram("line.vert", "line.geom", "line.frag");

	// create mouseManip
	g_mouseManipPtr.reset(new MouseManip(TOUCH_L_MOUSE));

	// bind mouse functions
	glutPassiveMotionFunc(MouseMotion); // <-- for constant position check
	//glutMotionFunc(MouseMotion);         <-- for click position check
	glutMouseFunc(MouseMovement);

	// create Line
	g_lineDrawPtr.reset(new Line());
	g_lineDrawPtr->Setup();

	// create context menu bind
	InitMenu();
};

void display() 
{
	glClearColor(1.f, 1.f, 1.f, 1.f);
	glClearDepth(1.f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// get mouse position, change origin to left bottom corrner and clamp values to window border
	g_coords.mousePos = glm::clamp(g_mouseManipPtr->GetMousePosition(), glm::ivec2{0.f, 0.f}, g_coords.size);
	g_coords.mousePos.y = g_coords.size.y - g_coords.mousePos.y;

	// create NDC coordinates [0, size] -> [-1, 1]
	g_coords.NDC.x = ((float)g_coords.mousePos.x / g_coords.size.x - 0.5f) * 2.0f;
	g_coords.NDC.y = ((float)g_coords.mousePos.y / g_coords.size.y - 0.5f) * 2.0f;

	//std::cout << g_coords.NDC.x << ' ' << g_coords.NDC.y << '\n';

	// update the lineDraw and draw the result
	g_lineDrawPtr->Update(g_coords.NDC);
	g_lineDrawPtr->Draw(g_program);

	glutSwapBuffers();
	glutPostRedisplay();
};

void reshape(int w, int h) 
{
	// update max size of screen
	g_coords.size = { w, h };

	glViewport(0, 0, w, h);
};


void keyboard(unsigned char key, int x, int y) 
{
	// LINE TEST - change to mouse interactions later on
	if (key == 'a')
	{
		g_lineDrawPtr->RemoveVert();
	}
	else if (key == 'd')
	{
		g_lineDrawPtr->AddVert(LineVertex{ glm::vec4(0.5f, 0.25f, 0.9f, 1.0f), g_coords.NDC, glm::vec2(0.0f, 0.0f), false });
	}
	switch(key)
	{
	case 'a':
		g_lineDrawPtr->RemoveVert();
		break;
	case 'd':
		g_lineDrawPtr->AddVert(LineVertex{ glm::vec4(0.5f, 0.25f, 0.9f, 1.0f), g_coords.NDC, glm::vec2(0.0f, 0.0f), false });
		break;
	case 27: // esc
		glutLeaveMainLoop();
		break;
	}
};

unsigned int defaults(unsigned int displayMode, int& width, int& height) 
{
	return displayMode; 
}

// TODO: FIX THE PATH SYSTEM, add comments, throw the uneccessary code
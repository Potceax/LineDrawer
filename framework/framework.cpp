#include "framework.h"

#include <algorithm>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <exception>
#include <stdexcept>
#include <string.h>
#include <gl_4_3.h>
#include <gl_load.hpp>
#include <GL/freeglut.h>
#include <Basic.h>

#ifndef _WIN32
#define APIENTRY
#endif

void init();
void display();
void reshape(int w, int h);
void keyboard(unsigned char key, int x, int y);

unsigned int defaults(unsigned int displayMode, int& width, int& height);

int main(int argc, char** argv)
{
	glutInit(&argc, argv);

	// display settings
	unsigned int displayMode = GLUT_DOUBLE | GLUT_ALPHA | GLUT_DEPTH | GLUT_STENCIL;

	int width{ 500 }; int height{ 500 };
	displayMode = defaults(displayMode, width, height);

	// initialization of GL version
	glutInitDisplayMode(displayMode);
	glutInitContextVersion(4, 3);
	glutInitContextProfile(GLUT_COMPATIBILITY_PROFILE);

	// Enable Debug mode
#ifdef _DEBUG
	glutInitContextFlags(GLUT_DEBUG);
	auto file = std::freopen("Log.log", "w", stderr); // IDEA: Create class for logging -> as the main function it will output the captured strings to chosen file
#endif

	// window creation
	int window = glutCreateWindow("LineDrawer");
	glutInitWindowPosition(GLUT_WINDOW_X, GLUT_WINDOW_Y);
	glutInitWindowSize(width, height);

	// load of GL functions to program
	glload::LoadFunctions();

	// check the user version
	if (!glload::IsVersionGEQ(4, 3))
	{
		fprintf(stderr, "Cannot use program with OpenGL version < 4.3 (Version used %i %i)\n", glload::GetMajorVersion(), glload::GetMinorVersion());
		glutDestroyWindow(window);
		glutExit();
	}

	// main loop function binds
	glutReshapeFunc(reshape);
	glutDisplayFunc(display);
	glutKeyboardFunc(keyboard);

	//initialize Relative Path for basic
	Basic::System::Get().SetPath(argv[0]);

	init();

	// before loop initialization function call
	glutMainLoop();
	return 0;
}

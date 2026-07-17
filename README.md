# OpenGL Line Drawer
## Description
Small OpenGL project created to learn how to render lines and arcs using the programmable pipeline, and to learn more about CMake capabilities.

The project is still a work in progress: lines work correctly, but arcs are not yet oriented to match the direction given by the last line segment.

Whole project was developed with the use of graphics and math libraries (freeglut, glload, GLM) and OpenGL API 4.3.

## Installation and running
The project was developed on **Windows 10** and tested on an **AMD Radeon 6600** with an **Intel i5-12400F**. All necessary libraries are included in the project

To build a project move files to destined folder and use CMake build command:

```bash
cmake -B build
```

Then to compile the build use:

```bash
cmake --build build
```

You can also create .exe of program with:

```bash
cmake --install build --prefix <folder name> --config [debug | release]
```

## Controls

The program has very simple controls, mouse is used for selecting position of line/arc and to create context window.

Keys:
- A - undo line/arc part
- D - add line/arc part
- esc - close program

## License

MIT licence
See `LICENSE` file for details.
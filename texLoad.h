#ifndef texLoad
#define texLoad

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>

#ifdef USEGLEW
#include <GL/glew.h>
#endif
#define GL_GLEXT_PROTOTYPES
#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif

#define PI 3.14159265358979323846
#define Cos(th) cos(PI/180*(th))
#define Sin(th) sin(PI/180*(th))

#ifdef __cplusplus
extern "C" {
#endif

void Print(const char* format , ...);
void Fatal(const char* format , ...);
unsigned int LoadTexBMP(const char* file);
unsigned int LoadCubeTexBMP(const char* files[]);
void ErrCheck(const char* where);

#ifdef __cplusplus
}
#endif

#endif
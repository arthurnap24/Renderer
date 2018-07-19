#include <GL/glew.h>
#include <GL/freeglut.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <sstream>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "glm/gtc/matrix_transform.hpp"

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <GL/gl.h>
#include <GL/glx.h>
#include <GL/glu.h>

#include <pthread.h>

void keyboard(unsigned char, int, int);
void computeProjection(void);
void init(void);
bool loadOBJ(const char *);
void drawOBJ(void);
void changeOBJ(const char*);
void changeOBJBuffer(const char *);
void initBufferObject(const char *);
void toggleVertOrientation(void);
void myReshape(int, int);
void orthonormalizeAxes(void);
void updateCameraRot(float, float, float);
void applyTranslation(void);
void changeUniformModelView(void);
void changeUniformProjection(void);
void resetCamera(void);
void moveRight(void);
void moveLeft(void);
void moveUp(void);
void moveDown(void);
void moveFront(void);
void moveBack(void);
void modifyColor(float, float, float);
// new for GLX
void handleKeyboard(int);
void updateWindow1(void);
void updateWindow2(void);
void initLight(void);
void toggleShadingMode(void);
void toggleLight();
void changeLightColorMode();
void modifyChosenLightColor(float, float, float);
void modifyChosenLightAttenuation(float);

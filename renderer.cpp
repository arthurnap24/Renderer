#include "includes.h"

#define PI 3.14159265
#define DEBUG 1
#define WIDTH 800
#define HEIGHT 600
#define TRUE 1
#define FALSE 0

// for light coloring
#define G_AMBIENT 0
#define L_AMBIENT 1
#define L_DIFFUSE 2
#define L_SPECULAR 3

using namespace std;

// window1 creation with Xlib
Display *dpy;
Window root;
GLint att[] = { GLX_RGBA, GLX_DEPTH_SIZE, 24, GLX_DOUBLEBUFFER, None };
XVisualInfo *vi;
Colormap cmap;
XSetWindowAttributes swa;
Window win;
GLXContext glc;
XWindowAttributes gwa;
XEvent xev;

// window 2
Display *dpy2;
GLint att2[] = { GLX_RGBA, GLX_DEPTH_SIZE, 24, GLX_DOUBLEBUFFER, None };
XVisualInfo *vi2;
Colormap cmap2;
XSetWindowAttributes swa2;
Window win2;
GLXContext glc2;
XWindowAttributes gwa2;
XEvent xev2;


// for GLSL
GLuint vsID, fsID, pID, VBO, VAO, EBO;

// for projection matrix:
double znear, zfar, screenWidth, screenHeight; 
double r, l, t, b;
float moveSpeed = 0.10f;

// already column major
GLfloat transfMat[16] = { 0.0, 0.0, 0.0, 0.0,
                          0.0, 0.0, 0.0, 0.0,
                          0.0, 0.0, 0.0, 0.0,
                          0.0, 0.0, 0.0, 0.0 };

GLfloat projMat[16] = { 0.0, 0.0, 0.0, 0.0,
                        0.0, 0.0, 0.0, 0.0,
                        0.0, 0.0, 0.0, 0.0,
                        0.0, 0.0, 0.0, 0.0 };

GLfloat normalMat[9] = {0.0, 0.0, 0.0,
                        0.0, 0.0, 0.0,
                        0.0, 0.0, 0.0 };

GLfloat vecToViewer[3] = {0.0 ,0.0, 0.0};

float colorTrans = 0.01f;
GLfloat color[3] = { 1.0f,1.0f,1.0f };

GLint modeViewMatLocation;
GLint projectionMatLocation;
GLint colorVecLocation;
GLint normalMatLocation;
GLint vecToViewerLocation;

GLint gAmbientLocation;
GLint ambientLocation;
GLint diffuseLocation;
GLint specularLocation;

GLint lightsOnLocation;

// for materials, send to the shader:
GLint matSpecLocation;
GLint matAmbLocation;
GLint matDiffLocation;
GLint shininessLocation;

// not used?:
GLint ambCoeffLocation; 
GLint diffCoeffLocation;
GLint specCoeffLocation;

const char* bunny = "./bunny.obj";
const char* cactus = "./cactus.obj";
const char* square = "./square.obj";
GLenum displayMode = GL_TRIANGLES;
GLenum fillMode = GL_FILL;
GLenum faceOrientation = GL_CCW;
GLenum shadingMode = GL_SMOOTH;

GLfloat global_ambient[] = {0.0, 0.1, 0.1, 0.0};  // modify color of the ambient light d

// params for global ambient light, diffuse, and specular
GLfloat light_ambient[] = {0.2, 0.2, 0.2, 1.0};
GLfloat light_diffuse[] = {1.0, 1.0, 1.0, 1.0};
GLfloat light_specular[] = {1.0, 1.0, 1.0, 1.0};
GLint lightsOn = TRUE;
GLint lightColorMode = G_AMBIENT;

// for materials
GLfloat mat_ambient[] = {1.0, 1.0 ,1.0, 1.0};      // need mat diffuse as well
GLfloat mat_diffuse[] = {1.0, 1.0, 1.0, 1.0};
GLfloat mat_specular[] = {1.0, 1.0, 1.0, 1.0};   // mat specular
GLfloat shininess[] = {5.0}; // shininess

// used as intermediary storage for reading the obj files.
std::vector<double> vertDataVec;
int vertDataVecLength = 0;
std::vector<int> indicesVec;
int indicesVecLength = 0;
int numFaces = 0; //for fixed pipeline

std::vector<double> v1s;
std::vector<double> v2s;
std::vector<double> v3s;

std::vector<int> fi1s;
std::vector<int> fi2s;
std::vector<int> fi3s;
std::vector<int> normAssigned;


std::vector<double> sumwa1s; // sum of weighted areas for each vertex (numerators)
std::vector<double> sumwa2s;
std::vector<double> sumwa3s;
std::vector<double> wsum;    // sum of all the weights/ares (denominators)

std::vector<double> n1s, n2s, n3s;
std::vector<double> n1sFlat, n2sFlat, n3sFlat;

glm::vec3 eyePos = glm::vec3(0.0f, 0.0f, 9.0f);    // where the eye is located in the world
glm::vec3 targetPos = glm::vec3(0.0f, 0.0f, 0.0f); // where the camera is looking at

// LOCAL AXES:
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);    // (y-axis up of camera)
glm::vec3 cameraLeft = glm::vec3(-1.0f, 0.0f, 0.0f);  // (x-axis)
glm::vec3 eyeLookAt = targetPos - eyePos;            // (z-axis)
glm::vec3 UP = glm::vec3(0.0f, 1.0f, 0.0f);

glm::mat3 rotMat = glm::mat3();
glm::mat4 translateMat;
glm::mat4 modelViewMatrix;

// FOR DEBUGGING CAMERA ROTATIONS:
float apparentAnglePitch, apparentAngleYaw, apparentAngleRoll;

/////////////////////////////////////////////////////////////////////////////
// Rotate using axis angle:
// 1. Convert the angle from degrees to radians
// 2. A. Create the first rotation matrix for one of the axes  (yawRotMat)
//    B. Create the second rotation matrix for one of the axes (pitchRotMat)
//    C. Create the third rotation matrix for one of the axes  (rollRotMat)
// 3. Multiply the rotation matrices to the axes.
/////////////////////////////////////////////////////////////////////////////
void updateCameraRot(float pitchAngle, float yawAngle, float rollAngle)
{
  float cosYRad, sinYRad, apYawAngleRad, apPitchAngleRad, apRollAngleRad;

  apparentAngleYaw += yawAngle;
  apparentAnglePitch += pitchAngle;
  apparentAngleRoll += rollAngle;
  if (apparentAngleYaw >= 360 || apparentAngleYaw <= -360) apparentAngleYaw = 0;
  if (apparentAnglePitch >= 360 || apparentAnglePitch <= -360) apparentAnglePitch = 0;
  if (apparentAngleRoll >= 360 || apparentAngleRoll <= -360) apparentAngleRoll = 0;
	
  printf("totalYaw=%f, totalPitch=%f, totalRoll=%f\n", apparentAngleYaw, apparentAnglePitch, apparentAngleRoll);
	
  float yawAngleRad = yawAngle * PI / 180;
  float pitchAngleRad = pitchAngle * PI / 180;
  float rollAngleRad = rollAngle * PI / 180;

  apYawAngleRad = apparentAngleYaw * PI / 180;
  apPitchAngleRad = apparentAnglePitch * PI / 180;
  apRollAngleRad = apparentAngleRoll * PI / 180;

  // this orthonormalizes the axes
  orthonormalizeAxes();

  // yaw
  // from C's math library
  //cosYRad = (float)cos(apYawAngleRad);
  //sinYRad = (float)sin(apYawAngleRad);
  cosYRad = (float)cos(yawAngleRad);
  sinYRad = (float)sin(yawAngleRad);
  glm::mat3 I = glm::mat3(1.0);
  float upSkewSymmArr[9] = { 0, cameraUp[2], -cameraUp[1],
                             -cameraUp[2], 0, cameraUp[0],
                             cameraUp[1], -cameraUp[0], 0 };
	
  glm::mat3 mProjUp = glm::outerProduct(cameraUp, cameraUp);
  glm::mat3 upSkewSymm = glm::make_mat3(upSkewSymmArr);
  // create the rotation matrix (R from axis angle formula)
  glm::mat3 yawRotMat = mProjUp + cosYRad*(I - mProjUp) + sinYRad*upSkewSymm;

  eyeLookAt = glm::normalize(eyeLookAt*yawRotMat);
  cameraLeft = glm::normalize(glm::cross(cameraUp, eyeLookAt)); //cameraUp is not changed by yaw

  // pitch
  cosYRad = (float)cos(pitchAngleRad);
  sinYRad = (float)sin(pitchAngleRad);
  float leftSkewSymmArr[9] = { 0, cameraLeft[2], -cameraLeft[1],
                               -cameraLeft[2], 0, cameraLeft[0],
                               cameraLeft[1], -cameraLeft[0], 0 };
  glm::mat3 mProjLeft = glm::outerProduct(cameraLeft, cameraLeft);
  glm::mat3 leftSkewSymm = glm::make_mat3(leftSkewSymmArr);
  glm::mat3 pitchRotMat = mProjLeft + cosYRad*(I - mProjLeft) + sinYRad*leftSkewSymm;

  eyeLookAt = glm::normalize(eyeLookAt*pitchRotMat);
  cameraUp = glm::normalize(glm::cross(eyeLookAt, cameraLeft));

  // roll
  cosYRad = (float)cos(rollAngleRad);
  sinYRad = (float)sin(rollAngleRad);
  float frontSkewSymmArr[9] = { 0, eyeLookAt[2], -eyeLookAt[1],
                                -eyeLookAt[2], 0, eyeLookAt[0],
                                eyeLookAt[1], -eyeLookAt[0], 0 };

  glm::mat3 mProjFront = glm::outerProduct(eyeLookAt, eyeLookAt);
  glm::mat3 frontSkewSymm = glm::make_mat3(frontSkewSymmArr);
  glm::mat3 rollRotMat = mProjFront + cosYRad*(I - mProjFront) + sinYRad*frontSkewSymm;

  cameraLeft = glm::normalize(cameraLeft*rollRotMat);
  cameraUp = glm::normalize(glm::cross(eyeLookAt, cameraLeft));

  // Form the new rotation matrix:
  rotMat *= (yawRotMat * rollRotMat * pitchRotMat);
    

  if (DEBUG)
  {
    printf("In updateCameraRot:\n");
    printf("  eyeLookAt = <%f,%f,%f>\n", eyeLookAt[0], eyeLookAt[1], eyeLookAt[2]);
    printf("  cameraUp = <%f,%f,%f>\n", cameraUp[0], cameraUp[1], cameraUp[2]);
    printf("  cameraLeft = <%f,%f,%f>\n", cameraLeft[0], cameraLeft[1], cameraLeft[2]);
  }

  // First we load the identity matrix:

  // What gets used in glMatrixMult:
  transfMat[0] = rotMat[0][0];
  transfMat[1] = rotMat[0][1];
  transfMat[2] = rotMat[0][2];

  transfMat[3] = 0;
  transfMat[4] = rotMat[1][0];
  transfMat[5] = rotMat[1][1];
  transfMat[6] = rotMat[1][2];

  transfMat[7] = 0;
  transfMat[8] = rotMat[2][0];
  transfMat[9] = rotMat[2][1];
  transfMat[10] = rotMat[2][2];
  transfMat[11] = 0;
  
  // What gets used as normal matrix in vertex shader (Just like OpenGL fixed pipeline):
  normalMat[0] = rotMat[0][0];
  normalMat[1] = rotMat[0][1];
  normalMat[2] = rotMat[0][2];

  normalMat[3] = rotMat[1][0];
  normalMat[4] = rotMat[1][1];
  normalMat[5] = rotMat[1][2];

  normalMat[6] = rotMat[2][0];
  normalMat[7] = rotMat[2][1];
  normalMat[8] = rotMat[2][2];

  // translation multiplied by the rows
  applyTranslation();  

  changeUniformModelView();
}

// This function is used to send the new ModelView matrix to the vertex shader
void changeUniformModelView()
{
  glUniformMatrix4fv(modeViewMatLocation, 1, GL_FALSE, transfMat);
  glUniform3f(vecToViewerLocation, vecToViewer[0], vecToViewer[1], vecToViewer[2]);
  glUniformMatrix3fv(normalMatLocation, 1, GL_FALSE, normalMat);  
}

// This function is used to send the new Projection matrix to the vertex shader
void changeUniformProjection()
{
  glUniformMatrix4fv(projectionMatLocation, 1, GL_FALSE, projMat);
}

// This function is used to apply the translation to the ModelView matrix
void applyTranslation()
{
  transfMat[12] = -rotMat[0][0]*eyePos[0] -rotMat[1][0]*eyePos[1] - rotMat[2][0]*eyePos[2];
  transfMat[13] = -rotMat[0][1]*eyePos[0] -rotMat[1][1]*eyePos[1] - rotMat[2][1]*eyePos[2];
  transfMat[14] = -rotMat[0][2]*eyePos[0] -rotMat[1][2]*eyePos[1] - rotMat[2][2]*eyePos[2];
  transfMat[15] = 1;
  
  vecToViewer[0] = -transfMat[12];
  vecToViewer[1] = -transfMat[13];
  vecToViewer[2] = -transfMat[14];
}

// reinitialize every value used for computation, problem as well
void resetCamera()
{
  znear = 6.0; // distance from camera lens to what's too close
  zfar = 20.0;

  eyePos = glm::vec3(0.0f, 0.0f, 9.0f);
  targetPos = glm::vec3(0.0f, 0.0f, 0.0f);

  cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);    // (y-axis up of camera)
  cameraLeft = glm::vec3(-1.0f, 0.0f, 0.0f);  // (x-axis)
  eyeLookAt = glm::normalize(targetPos - eyePos);

  apparentAnglePitch = 0; 
  apparentAngleYaw = 0; 
  apparentAngleRoll = 0;
	
  rotMat = glm::mat3();
	
  // put transformation matrix to identity.
  transfMat[0] = 1;
  transfMat[1] = 0;
  transfMat[2] = 0;
  transfMat[3] = 0;

  transfMat[4] = 0;
  transfMat[5] = 1;
  transfMat[6] = 0;
  transfMat[7] = 0;
	
  transfMat[8] = 0;
  transfMat[9] = 0;
  transfMat[10] = 1;
  transfMat[11] = 0;
	
  transfMat[12] = 0;
  transfMat[13] = 0;
  transfMat[14] = 0;
  transfMat[15] = 1;
    
  init();
    
  changeUniformModelView();
  changeUniformProjection();
}


// maintain orthonormality of the basis vectors (axis)
void orthonormalizeAxes()
{
  eyeLookAt = glm::normalize(eyeLookAt);
  cameraLeft = glm::normalize(glm::cross(cameraUp, eyeLookAt));
  cameraUp = glm::normalize(glm::cross(eyeLookAt, cameraLeft));
}

// Loads the .obj models to vectors that will be used by the shaders
bool loadOBJ(const char * path)
{
  FILE * file = fopen(path, "r");
  // 3 vertices for each "v" line (which defines a face) and 3 for
  // their indexes.
  double v1, v2, v3;
  int fi1, fi2, fi3;
  glm::vec3 plN;     // plane normal
  glm::vec3 plNLen;  // plane normal length
  glm::vec3 plUnitN; // plane unit normal

  glm::vec3 side1, side2;  // subtract a point from another point

  double a; //area of a face

  glm::vec3 wn; // weighted normals

  // just for testing and debugging...
  int count = 0;

  vertDataVecLength = 0;
  indicesVecLength = 0;

  if (!file)
  {
    cout << "Unable to load .obj file (can't be found)";
    cout << path;
    return false;
  }
  while (1)
  {
    char lineHeader[128];
    //read the first word of the line
    int res = fscanf(file, "%s", lineHeader);
    if (res == EOF)
      break;
    if (strcmp(lineHeader, "v") == 0)
    {
      fscanf(file, "%lf %lf %lf\n", &v1, &v2, &v3);
      // vertex data:
      vertDataVec.push_back(v1);
      vertDataVec.push_back(v2);
      vertDataVec.push_back(v3);

      // colors:
      vertDataVec.push_back(1.0);
      vertDataVec.push_back(0.0);
      vertDataVec.push_back(0.0);

      // need to push normals here as well:
      vertDataVec.push_back(0.0);
      vertDataVec.push_back(0.0);
      vertDataVec.push_back(0.0);

      // count the size of this vector
      vertDataVecLength += 9; // now we are pushing 9 elems at a time

      // for Assignment 1
      v1s.push_back(v1);
      v2s.push_back(v2);
      v3s.push_back(v3);

      sumwa1s.push_back(0.0);
      sumwa2s.push_back(0.0);
      sumwa3s.push_back(0.0);
      wsum.push_back(0.0);
    }
    else if (strcmp(lineHeader, "f") == 0)
    {
      // ALL VERTICES ARE KNOWN NOW.
      fscanf(file, "%d %d %d\n", &fi1, &fi2, &fi3);
      // 3 vertex indices for each face which is a triangle
      int i1, i2, i3;

      i1 = fi1-1;
      i2 = fi2-1;
      i3 = fi3-1;

      indicesVec.push_back(i1);
      indicesVec.push_back(i2);
      indicesVec.push_back(i3);

      fi1s.push_back(i1);
      fi2s.push_back(i2);
      fi3s.push_back(i3);

      // count the size of this vector:
      indicesVecLength += 3;

      // compute face normals (counter clockwise on obj files)
      side1 = glm::vec3(v1s[i3] - v1s[i2], v2s[i3] - v2s[i2], v3s[i3] - v3s[i2]);
      side2 = glm::vec3(v1s[i1] - v1s[i2], v2s[i1] - v2s[i2], v3s[i1] - v3s[i2]);

      plN = glm::cross(side1, side2);
      plUnitN = glm::normalize(plN);  // normalize

      a = 0.5 * glm::dot(glm::cross(side1, side2), plUnitN);

      wn = glm::vec3(plUnitN[0]*a, plUnitN[1]*a, plUnitN[2]*a); // weighted normal of plane

      // update the numerator
      sumwa1s[i1] += wn[0];
      sumwa2s[i1] += wn[1];
      sumwa3s[i1] += wn[2];
      wsum[i1] += a;

      sumwa1s[i2] += wn[0];
      sumwa2s[i2] += wn[1];
      sumwa3s[i2] += wn[2];
      wsum[i2] += a;

      sumwa1s[i3] += wn[0];
      sumwa2s[i3] += wn[1];
      sumwa3s[i3] += wn[2];
      wsum[i3] += a;
    }
    count++;
  }
  fclose(file);
  int i;
  int limit = sumwa1s.size();
  int j = 6; // start index of vertex array buffer (to put normals into)

  for (i=0; i<limit; i++)
  {
    double comp1, comp2, comp3;
    double sumWeight = wsum[i];
    comp1 = sumwa1s[i]/sumWeight;
    comp2 = sumwa2s[i]/sumWeight;
    comp3 = sumwa3s[i]/sumWeight;

    // n1s, n2s, n3s are only used by fixed pipeline
    n1s.push_back(comp1);
    n2s.push_back(comp2);
    n3s.push_back(comp3);

    vertDataVec[j]= comp1;
    vertDataVec[j+1] = comp2;
    vertDataVec[j+2] = comp3;

    j+=9;  // add 9 to j because that is the difference between the next set of vertices
  }

  numFaces = fi1s.size();

  printf("in loadOBJ()\n");
  printf("n1.size()=%lu\n", n1s.size());

  printf("fi1s.size()=%ld\n", fi1s.size());
  printf("vertDataVecLength=%d\n", vertDataVecLength);
  printf("indicesVecLength=%d\n", indicesVecLength);

  return true;
}


// This function will read the shader file to be loaded and used by the program.
void readShaderFile(const GLchar* shaderPath, std::string& shaderCode)
{

  std::ifstream shaderFile;
  // ensures ifstream objects can throw exceptions:
  shaderFile.exceptions(std::ifstream::badbit);
  try
  {
    // Open files
    shaderFile.open(shaderPath);
    std::stringstream shaderStream;

    // Read file's buffer contents into streams
    shaderStream << shaderFile.rdbuf();

    // close file handlers
    shaderFile.close();

    // Convert stream into GLchar array
    shaderCode = shaderStream.str();
  }
  catch (std::ifstream::failure e)
  {
    std::cout << "ERROR::SHADER::FILE_NOT_SUCCESFULLY_READ" << std::endl;
  }
}

void setShaders()
{
  char *vs = NULL, *fs = NULL;
  const char * vShaderName1 = "./vertexshader.glsl";
  const char * fShaderName1 = "./fragshader.glsl";
  const char * vShaderName2 = "./vertexshader2.glsl";
  const char * fShaderName2 = "./fragshader2.glsl";
  // for error logging:
  GLint isLinked = 0;
  GLchar* vsInfoLog = (GLchar*)malloc(sizeof(GLchar) * 200);
  GLsizei vsLength = 200;
  GLchar* fsInfoLog = (GLchar*)malloc(sizeof(GLchar) * 200);
  GLsizei fsLength = 200;

  std::string vertShaderString; //empty string
  std::string fragShaderString; //empty string

  vsID = glCreateShader(GL_VERTEX_SHADER);
  fsID = glCreateShader(GL_FRAGMENT_SHADER);

  if (vsID == 0 || fsID == 0) printf ("One of the shaders aren't created succesfully\n");

  if (shadingMode == GL_SMOOTH)
  {
    printf("SMOOTH\n");
    readShaderFile(vShaderName1, vertShaderString);
    readShaderFile(fShaderName1, fragShaderString);
  }
  else if (shadingMode == GL_FLAT)
  {
    printf("FLAT\n");
    readShaderFile(vShaderName2, vertShaderString);
    readShaderFile(fShaderName2, fragShaderString);
  }
  const GLchar * pVertShaderSource = vertShaderString.c_str();
  const GLchar * pFragShaderSource = fragShaderString.c_str();

  glShaderSource(vsID, 1, &pVertShaderSource, NULL);
  glShaderSource(fsID, 1, &pFragShaderSource, NULL);

  glCompileShader(vsID);
  glCompileShader(fsID);

  glGetShaderInfoLog(vsID, 200, &vsLength, vsInfoLog);
  printf("%s\n", vsInfoLog);

  glGetShaderInfoLog(fsID, 200, &fsLength, fsInfoLog);
  printf("%s\n", fsInfoLog);

  pID = glCreateProgram();

  printf("pID=%d\n", pID);

  glAttachShader(pID, vsID);
  glAttachShader(pID, fsID);

  // This is needed here to replace layout syntax in gl version 330 since it's not available in version 130
  glBindAttribLocation (pID, 0, "vertPosition");
  glBindAttribLocation (pID, 1, "vertColor");
  glBindAttribLocation (pID, 2, "VertexNormal");

  glLinkProgram(pID);

  // succesfully linked the program with no errors
  glGetProgramiv(pID, GL_LINK_STATUS, &isLinked);

  if(isLinked == GL_FALSE)
  {
    printf("not linked\n");	
  }

  glUseProgram(pID);

  // get uniform input
  modeViewMatLocation = glGetUniformLocation(pID, "modelViewMatrix");
  projectionMatLocation = glGetUniformLocation(pID, "projectionMatrix");
  normalMatLocation = glGetUniformLocation(pID, "NormalMatrix");
  colorVecLocation = glGetUniformLocation(pID, "inputColor");
  vecToViewerLocation = glGetUniformLocation(pID, "vecToCamera");

  matAmbLocation = glGetUniformLocation(pID, "matAmbient");
  matDiffLocation = glGetUniformLocation(pID, "matDiffuse");
  matSpecLocation = glGetUniformLocation(pID, "matSpecular");
  shininessLocation = glGetUniformLocation(pID, "shininess");

  // lights
  gAmbientLocation = glGetUniformLocation(pID, "gAmbientColor");
  ambientLocation = glGetUniformLocation(pID, "ambientColor");
  diffuseLocation = glGetUniformLocation(pID, "diffuseColor");
  specularLocation = glGetUniformLocation(pID, "specularColor");
  lightsOnLocation = glGetUniformLocation(pID, "lightsOn");

  // materials
  ambCoeffLocation = glGetUniformLocation(pID, "ambientCoeff");
  diffCoeffLocation = glGetUniformLocation(pID, "diffuseCoeff");
  specCoeffLocation = glGetUniformLocation(pID, "specCoeff");

  init();
  int i = 0;
  printf("transfMat:\n");
  for (i=0 ; i<16; i++)
  {
    printf("%f ", transfMat[i]);
    if ( (i+1) % 4 == 0) printf("\n");
  }

  printf("projMat:\n");
  for (i=0 ; i<16; i++)
  {
    printf("%f ", projMat[i]);
    if ( (i+1) % 4 == 0) printf("\n");
  }

  //glUniformMatrix4fv(modeViewMatLocation, 1, GL_FALSE, modelViewMat);
  glUniformMatrix4fv(modeViewMatLocation, 1, GL_FALSE, transfMat);
  glUniformMatrix4fv(projectionMatLocation, 1, GL_FALSE, projMat);
  glUniformMatrix3fv(normalMatLocation, 1, GL_FALSE, normalMat);
  glUniform1i(lightsOnLocation, lightsOn);

  glUniform3f(colorVecLocation, color[0], color[1], color[2]);
  glUniform3f(vecToViewerLocation,vecToViewer[0], vecToViewer[1], vecToViewer[2]);

  glUniform4f(gAmbientLocation, global_ambient[0], global_ambient[1], global_ambient[2], global_ambient[3]);
  glUniform4f(ambientLocation, light_ambient[0], light_ambient[1], light_ambient[2], light_ambient[3]);
  glUniform4f(diffuseLocation, light_diffuse[0], light_diffuse[1], light_diffuse[2], light_diffuse[3]);
  glUniform4f(specularLocation, light_specular[0], light_specular[1], light_specular[2], light_specular[3]);

  glUniform4f(matAmbLocation, mat_ambient[0], mat_ambient[1], mat_ambient[2], mat_ambient[3]);
  glUniform4f(matDiffLocation, mat_diffuse[0], mat_diffuse[1], mat_diffuse[2], mat_diffuse[3]);
  glUniform4f(matSpecLocation, mat_specular[0], mat_specular[1], mat_specular[2], mat_specular[3]);
  glUniform1f(shininessLocation, shininess[0]);
}

void display(void)
{
  glUseProgram(pID);

  glValidateProgram(pID);
  GLint validate = 0;
  glGetProgramiv(pID, GL_VALIDATE_STATUS, &validate);

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glBindVertexArray(VAO);

  glEnable(GL_CULL_FACE);
  glFrontFace(faceOrientation);
  glCullFace(GL_BACK);

  glPolygonMode(GL_FRONT_AND_BACK, fillMode);

  glDrawElements(displayMode, indicesVecLength, GL_UNSIGNED_INT, 0);
  glBindVertexArray(0);
}

bool firstTime = true;

void initBufferObject(const char * path)
{
    int i;
    loadOBJ(path);

    printf("in initBufferObject()\n");

    GLdouble vertData[vertDataVecLength];
    GLuint indices[indicesVecLength];

    // Need to load the vertices into an array not a pointer! Can't do things dynamically.
    for (i=0; i < vertDataVecLength; i++)
    {
        vertData[i] = vertDataVec.at(i); 
    }
    for (i=0; i< indicesVecLength; i++)
    {
        indices[i] = indicesVec.at(i);
    }

    // don't make another one when loading another model
    if (firstTime)
    {
        glGenBuffers(1, &VBO); // broken here
        printf ("Fixed up to here\n");
        glGenBuffers(1, &EBO);

        glGenVertexArrays(1, &VAO);
        glBindVertexArray(VAO); 
    }

    firstTime = false;

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);

    // setup VAO
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertData), vertData, GL_DYNAMIC_DRAW);

    glVertexAttribPointer(0, 3, GL_DOUBLE, GL_FALSE, 9 * sizeof(GLdouble), (GLvoid*)0);	
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_DOUBLE, GL_FALSE, 9 * sizeof(GLdouble), (GLvoid*)(3 * sizeof(GLdouble)));	
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 3, GL_DOUBLE, GL_FALSE, 9 * sizeof(GLdouble), (GLvoid*)(6 * sizeof(GLdouble)));
    glEnableVertexAttribArray(2);

    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_DYNAMIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // Use depth buffering for hidden surface elimination
    glEnable(GL_DEPTH_TEST);
}

void modifyClippingPlane(unsigned char key)
{
  if (key == '1') znear += -0.10; //make near clipping plane closer to camera's front
  if (key == '2') znear += 0.10;  //make near clipping plane farther to camera's front
  if (key == '3') zfar += -0.10;  //make far clipping plane closer to camera's front
  if (key == '4') zfar += 0.10;   //make far clipping plane farther to camera's front

  if (znear <= 0.001)
  {
    znear = 0.001;
  }
  if (zfar >= 100)
  {
    zfar = 100;
  }

  printf("In modifyClippingPlane, znear=%f, zfar=%f\n", znear, zfar);

  glFrustum(-2.0, 2.0, -2.0, 2.0, znear, zfar);

  // compute the projection matrix
  computeProjection();

  int i;
  printf("projMat:\n");
  for (i=0 ; i<16; i++)
  {
    printf("%f ", projMat[i]);
    if ( (i+1) % 4 == 0) printf("\n");
  }

  glUniformMatrix4fv(projectionMatLocation, 1, GL_FALSE, projMat);
}

void moveRight()
{
  // remember this is the left vector of the camera
  eyePos -= (moveSpeed*glm::normalize(cameraLeft));
  targetPos -= (moveSpeed*glm::normalize(cameraLeft));
  applyTranslation();
  changeUniformModelView();
}

void moveLeft()
{
  eyePos += (moveSpeed*glm::normalize(cameraLeft));
  targetPos += (moveSpeed*glm::normalize(cameraLeft));
  applyTranslation();
  changeUniformModelView();
}

void moveUp()
{
  eyePos += (moveSpeed*glm::normalize(cameraUp));
  targetPos += (moveSpeed*glm::normalize(cameraUp));
  applyTranslation();
  changeUniformModelView();
}

void moveDown()
{
  eyePos -= (moveSpeed*glm::normalize(cameraUp));
  targetPos -= (moveSpeed*glm::normalize(cameraUp));
  applyTranslation();
  changeUniformModelView();
}

void moveFront()
{
  // eyeLookAt points to the front if it's pointing to the negative.
  eyePos += (moveSpeed*glm::normalize(eyeLookAt));
  targetPos += (moveSpeed*glm::normalize(eyeLookAt));
  applyTranslation();
  changeUniformModelView(); 
}

void moveBack()
{
  eyePos -= (moveSpeed*glm::normalize(eyeLookAt));
  targetPos -= (moveSpeed*glm::normalize(eyeLookAt));
  applyTranslation();
  changeUniformModelView();
}

void toggleVertOrientation()
{
  if (faceOrientation == GL_CCW) faceOrientation = GL_CW;
  else if (faceOrientation == GL_CW) faceOrientation = GL_CCW;
}

void toggleShadingMode()
{
  if (shadingMode == GL_SMOOTH)
  {
    shadingMode = GL_FLAT;
    printf("In toggleShadingMode(): changed to GL_FLAT\n");
  }
  else if (shadingMode == GL_FLAT)
  {
    shadingMode = GL_SMOOTH;
  }
  setShaders();
}

// compute using frustum, formula is from openGL website
void computeProjection()
{
  double A, B, C, D, alpha, beta;

  l = -2.0;
  r = 2.0;
  b = -2.0;
  t = 2.0;

  A = (r+l)/(r-l);
  B = (t+b)/(t-b);
  C = -(zfar+znear)/(zfar-znear);
  D = -(2*zfar*znear)/(zfar-znear);
  alpha = 2*znear / (r-l);
  beta = 2*znear / (t-b);

  printf("t+b=%f\n", t+b);

  projMat[0] = alpha;
  projMat[5] = beta;
  projMat[8] = A;
  projMat[9] = B;
  projMat[10] = C;
  projMat[11] = -1;
  projMat[14] = D; 
}

void myReshape(int w, int h)
{
  glViewport(0, 0, w, h);
}

void init()
{
  znear = 6.0;
  zfar = 20.0;

  transfMat[0] = 1.0;
  transfMat[5] = 1.0;
  transfMat[10] = 1.0;
  transfMat[12] = -eyePos[0];
  transfMat[13] = -eyePos[1];
  transfMat[14] = -eyePos[2];
  transfMat[15] = 1.0;

  vecToViewer[0] = eyePos[0];
  vecToViewer[1] = eyePos[1];
  vecToViewer[2] = eyePos[2];

  computeProjection();
}

// modify color of the model
void modifyColor(float redOffset, float greenOffset, float blueOffset)
{
  color[0] += redOffset;
  color[1] += greenOffset;
  color[2] += blueOffset;

  if (color[0] < 0) color[0] = 0;
  if (color[0] > 1) color[0] = 1;

  if (color[1] < 0) color[1] = 0;
  if (color[1] > 1) color[1] = 1;

  if (color[2] < 0) color[2] = 0;
  if (color[2] > 1) color[2] = 1;

  glUniform3f(colorVecLocation, color[0], color[1], color[2]);
}

// switch between coloring modes
void changeLightColorMode()
{
  lightColorMode++;
  if (lightColorMode > 3) lightColorMode = 0;

  printf("lightColorMode=%d\n", lightColorMode);
}

void modifyGlobalAmbientLightColor(float redOffset, float greenOffset, float blueOffset)
{
  GLfloat aRed, aGreen, aBlue;

  global_ambient[0] += redOffset;
  global_ambient[1] += greenOffset;
  global_ambient[2] += blueOffset;

  aRed = global_ambient[0];
  aGreen = global_ambient[1];
  aBlue = global_ambient[2];

  if (aRed < -1) global_ambient[0] = -1;
  if (aRed > 1) global_ambient[0] = 1;

  if (aGreen < -1) global_ambient[1] = -1;
  if (aGreen > 1) global_ambient[1] = 1;

  if (aBlue < -1) global_ambient[2] = -1;
  if (aBlue > 1) global_ambient[2] = 1;
}

void modifyGlobalAmbientAttenuation(float offset)
{
  GLfloat a;
  global_ambient[3] += offset;
  a = global_ambient[3];
  if (a < -1) global_ambient[3] = -1;
  if (a > 1) global_ambient[3] = 1;
}

void modifySpecular(float redOffset, float greenOffset, float blueOffset)
{
  GLfloat sRed, sGreen, sBlue;

  light_specular[0] += redOffset;
  light_specular[1] += greenOffset;
  light_specular[2] += blueOffset;

  sRed = light_specular[0];
  sGreen = light_specular[1];
  sBlue = light_specular[2];

  if (sRed < -1) light_specular[0] = -1;
  if (sRed > 1) light_specular[0] = 1;

  if (sGreen < -1) light_specular[1] = -1;
  if (sGreen > 1) light_specular[1] = 1;

  if (sBlue < -1) light_specular[2] = -1;
  if (sBlue > 1) light_specular[2] = 1;
}

void modifySpecularAttenuation(float offset)
{
  GLfloat a;
  light_specular[3] += offset;
  a = light_specular[3];
  if (a < -1) light_specular[3] = -1;
  if (a > 1) light_specular[3] = 1;
}

void modifyDiffuse(float redOffset, float greenOffset, float blueOffset)
{
  GLfloat dRed, dGreen, dBlue;

  light_diffuse[0] += redOffset;
  light_diffuse[1] += greenOffset;
  light_diffuse[2] += blueOffset;

  dRed = light_diffuse[0];
  dGreen = light_diffuse[1];
  dBlue = light_diffuse[2];

  if (dRed < -1) light_diffuse[0] = -1;
  if (dRed > 1) light_diffuse[0] = 1;

  if (dGreen < -1) light_diffuse[1] = -1;
  if (dGreen > 1) light_diffuse[1] = 1;

  if (dBlue < -1) light_diffuse[2] = -1;
  if (dBlue > 1) light_diffuse[2] = 1;
}

void modifyDiffuseAttenuation(float offset)
{
  GLfloat a;
  light_diffuse[3] += offset;
  a = light_diffuse[3];
  if (a < -1) light_diffuse[3] = -1;
  if (a > 1) light_diffuse[3] = 1;
}

void modifyAmbient(float redOffset, float greenOffset, float blueOffset)
{
  GLfloat aRed, aGreen, aBlue;

  light_ambient[0] += redOffset;
  light_ambient[1] += greenOffset;
  light_ambient[2] += blueOffset;

  aRed = light_ambient[0];
  aGreen = light_ambient[1];
  aBlue = light_ambient[2];

  if (aRed < -1) light_ambient[0] = -1;
  if (aRed > 1) light_ambient[0] = 1;

  if (aGreen < -1) light_ambient[1] = -1;
  if (aGreen > 1) light_ambient[1] = 1;

  if (aBlue < -1) light_ambient[2] = -1;
  if (aBlue > 1) light_ambient[2] = 1;
}

void modifyAmbientAttenuation(float offset)
{
  GLfloat a;
  light_ambient[3] += offset;
  a = light_ambient[3];
  if (a < -1) light_ambient[3] = -1;
  if (a > 1) light_ambient[3] = 1;
}


// need this wrapper solution for attenuation as well
void modifyChosenLightColor(float redOffset, float greenOffset, float blueOffset)
{
  if (lightColorMode == G_AMBIENT) 
  {
    modifyGlobalAmbientLightColor(redOffset, greenOffset, blueOffset);
  }
  else if (lightColorMode == L_AMBIENT)
  {
    modifyAmbient(redOffset, greenOffset, blueOffset);
  }
  else if (lightColorMode == L_DIFFUSE)
  {
    modifyDiffuse(redOffset, greenOffset, blueOffset);
  }
  else if (lightColorMode == L_SPECULAR)
  {
    modifySpecular(redOffset, greenOffset, blueOffset);
  }

  glUniform4f(gAmbientLocation, global_ambient[0], global_ambient[1], global_ambient[2], global_ambient[3]);
  glUniform4f(ambientLocation, light_ambient[0], light_ambient[1], light_ambient[2], light_ambient[3]);
  glUniform4f(diffuseLocation, light_diffuse[0], light_diffuse[1], light_diffuse[2], light_diffuse[3]);
  glUniform4f(specularLocation, light_specular[0], light_specular[1], light_specular[2], light_specular[3]);
}

void modifyChosenLightAttenuation(float offset)
{
  if (lightColorMode == G_AMBIENT)
  {
    modifyGlobalAmbientAttenuation(offset);
  }
  else if (lightColorMode == L_AMBIENT)
  {
    modifyAmbientAttenuation(offset);
  }
  else if (lightColorMode == L_DIFFUSE)
  {
    modifyDiffuseAttenuation(offset);
  }
  else if (lightColorMode == L_SPECULAR)
  {
    modifySpecularAttenuation(offset);
  }
  glUniform4f(gAmbientLocation, global_ambient[0], global_ambient[1], global_ambient[2], global_ambient[3]);
  glUniform4f(ambientLocation, light_ambient[0], light_ambient[1], light_ambient[2], light_ambient[3]);
  glUniform4f(diffuseLocation, light_diffuse[0], light_diffuse[1], light_diffuse[2], light_diffuse[3]);
  glUniform4f(specularLocation, light_specular[0], light_specular[1], light_specular[2], light_specular[3]);
}


// handle keyboard events
void handleKeyboard(int key)
{
  printf("key=%x\n", key);
  if (key == 0x19) moveUp();      // w
  if (key == 0x27) moveDown();    // a
  if (key == 0x28) moveRight();   // s
  if (key == 0x26) moveLeft();    // d
  if (key == 0x34) moveFront();   // z
  if (key == 0x35) moveBack();    // x

  if (key == 0x2e) updateCameraRot(0, 1.0, 0);    // l
  if (key == 0x2d) updateCameraRot(0, -1.0, 0);   // k
  if (key == 0x20) updateCameraRot(1.0, 0, 0);    // o
  if (key == 0x3c) updateCameraRot(-1.0, 0, 0);   // .
  if (key == 0x3d) updateCameraRot(0, 0, -1.0);   // /
  if (key == 0x3b) updateCameraRot(0, 0, 1.0);    // ,

  if (key == 0xa) modifyClippingPlane('1');  // 1 make near clipping plane closer
  if (key == 0xb) modifyClippingPlane('2');  // 2 make near clipping plane farther
  if (key == 0xc) modifyClippingPlane('3');  // 3 make far clipping plane closer
  if (key == 0xd) modifyClippingPlane('4');  // 4 make far clipping plane farther
  if (key == 0x13) resetCamera();

  if (key == 0x1b) modifyColor(colorTrans,0,0);    // r
  if (key == 0x2a) modifyColor(0,colorTrans,0);    // g
  if (key == 0x38) modifyColor(0,0,colorTrans);    // b
  if (key == 0x1a) modifyColor(-colorTrans,0,0);   // e
  if (key == 0x29) modifyColor(0,-colorTrans,0);   // f
  if (key == 0x37) modifyColor(0,0,-colorTrans);   // v

  if (key == 0x9) toggleLight();

  if (key == 0x60) changeLightColorMode();
  if (key == 0x22) modifyChosenLightColor(colorTrans,0,0);    // [
  if (key == 0x23) modifyChosenLightColor(0,colorTrans,0);    // ]
  if (key == 0x33) modifyChosenLightColor(0,0,colorTrans);    // '\'
  if (key == 0x2f) modifyChosenLightColor(-colorTrans,0,0);   // ;
  if (key == 0x30) modifyChosenLightColor(0,-colorTrans,0);   // '
  if (key == 0x24) modifyChosenLightColor(0,0,-colorTrans);   // return (enter)

  // need chosen attenuation as wells
  if (key == 0x21) modifyChosenLightAttenuation(colorTrans);   //p
  if (key == 0x14) modifyChosenLightAttenuation(-colorTrans);  //-

  if (key == 0x43)   // f1
  {
    changeOBJ(bunny);
  }
  if (key == 0x44)   // f2
  {
    changeOBJ(cactus);
  }
  if (key == 0x45) displayMode = GL_POINTS;  //f3
  if (key == 0x46)                           //f4
  {
    displayMode = GL_TRIANGLES;
    fillMode = GL_FILL;
  }
  if (key == 0x47)                           //f5
  {
    displayMode = GL_TRIANGLES;
    fillMode = GL_LINE;
  }
  if (key == 0x48) toggleVertOrientation();  //f6
  if (key == 0x12) toggleShadingMode();      //9

  if (key == 0x18) exit(0);
}

// turn only light on or off
void toggleLight()
{
  if (lightsOn) lightsOn = FALSE;
  else lightsOn = TRUE;

  glUniform1i(lightsOnLocation, lightsOn);
}

void drawOBJ()
{
  GLfloat abcAttenuation[] = {0.5, 0.5, 0.5}; // attenuation = 1 / a + bd + cd^2
  GLfloat light_position[] = { 0.0, 0.0, 3.0, 1.0};
  unsigned int i;
  // Uncomment if want to start with the triangle models
  glEnable(GL_CULL_FACE); // don't cull yet. Don't know how to specify my normals.
  glEnable(GL_LIGHTING);

  if (lightsOn) glEnable(GL_LIGHT0);
  else if (!lightsOn) glDisable(GL_LIGHT0);

  glLightModelfv(GL_LIGHT_MODEL_AMBIENT, global_ambient);
  glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);
  glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
  glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular);
  glLightfv(GL_LIGHT0, GL_POSITION, light_position);
  glLightfv(GL_LIGHT0, GL_QUADRATIC_ATTENUATION, abcAttenuation); 

  glFrontFace(faceOrientation); //toggle between GL_CW and GL_CCW

  glCullFace(GL_BACK);

  glPolygonMode(GL_FRONT_AND_BACK, fillMode);
  glBegin(displayMode);

  glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);  // and materials, move to initLight? YES MUST BE SPECIFIED EVERYTIME VERTICES ARE DRAWN
  glMaterialfv(GL_FRONT, GL_SHININESS, shininess);
  glMaterialfv(GL_FRONT, GL_AMBIENT, mat_ambient); // combined ambient and diffuse
  glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_diffuse);

  glm::vec3 n;   // apply transformation then normalize the vector representing normals


  for (i = 0; i < numFaces; i++)
  {
    int fi1 = fi1s[i];
    int fi2 = fi2s[i];
    int fi3 = fi3s[i];

    //glColor3fv(color);
    //IF THE LIGHT DOESN'T MOVE, USE THIS:
    n = glm::normalize(glm::vec3(n1s[fi1], n2s[fi1], n3s[fi1])) * rotMat;
    glNormal3f(n[0],n[1],n[2]);
    glVertex3d(v1s[fi1], v2s[fi1], v3s[fi1]);

    n = glm::normalize(glm::vec3(n1s[fi2], n2s[fi2], n3s[fi2])) * rotMat;
    glNormal3f(n[0],n[1],n[2]);
    glVertex3d(v1s[fi2], v2s[fi2], v3s[fi2]);

    n = glm::normalize(glm::vec3(n1s[fi3], n2s[fi3], n3s[fi3])) * rotMat;
    glNormal3f(n[0],n[1],n[2]);
    glVertex3d(v1s[fi3], v2s[fi3], v3s[fi3]);


    //IF THE LIGHT MOVES WITH THE SCENE, USE THIS:
    glNormal3f(n1s[fi1],n2s[fi1],n3s[fi1]);
    glVertex3d(v1s.at(fi1), v2s.at(fi1), v3s.at(fi1));

    glNormal3f(n1s[fi2],n2s[fi2],n3s[fi2]);
    glVertex3d(v1s.at(fi2), v2s.at(fi2), v3s.at(fi2));

    glNormal3f(n1s[fi3],n2s[fi3],n3s[fi3]);
    glVertex3d(v1s.at(fi3), v2s.at(fi3), v3s.at(fi3));
  }
  glEnd();
}

void changeOBJ(const char* modelName)
{
  v1s.clear();
  v2s.clear();
  v3s.clear();

  fi1s.clear();
  fi2s.clear();
  fi3s.clear();
  // clear information about normals too
  sumwa1s.clear();
  sumwa2s.clear();
  sumwa3s.clear();

  wsum.clear();

  n1s.clear();
  n2s.clear();
  n3s.clear();

  vertDataVec.clear();
  indicesVec.clear();

  initBufferObject(modelName);
}

// display function of openGL
void displayFixedPipeline()
{
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glShadeModel(shadingMode);

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glMultMatrixf(projMat);

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glMultMatrixf(transfMat);

  drawOBJ();

  glFlush();
}

// Use for X11 instead of glutPostRedisplay() from Freeglut. Also takes care of resizing.
// only updates the first window.
void updateWindows()
{
  glXMakeCurrent(dpy2, win2, glc2);
  XGetWindowAttributes(dpy2, win2, &gwa2);
  glViewport(0, 0, gwa2.width, gwa2.height);
  displayFixedPipeline();
  glXSwapBuffers(dpy2, win2);

  glXMakeCurrent(dpy, win, glc);
  XGetWindowAttributes(dpy, win, &gwa);
  glViewport(0, 0, gwa.width, gwa.height);
  display();
  glXSwapBuffers(dpy, win);
}

// Ran in a thread just like glutMainLoop() to do start everything.
void* updateLoop(void * i)
{
  glXMakeCurrent(dpy, win, glc);
  glewInit(); // must be called after the window is created
  //initBufferObject(square);
  //initBufferObject(cactus);
  initBufferObject(bunny);
  setShaders();
  glEnable(GL_DEPTH_TEST);

  glXMakeCurrent(dpy2, win2, glc2);
  glewInit(); // must be called after the window is created

  glEnable(GL_DEPTH_TEST);

  while(1)
  {
    XMapWindow(dpy2, win2);
    XNextEvent(dpy2, &xev2);
    if (xev2.type == Expose)
    {
      updateWindows();
    }
    else if (xev2.type == KeyPress)
    {
      int key = xev2.xkey.keycode;
      handleKeyboard(key);
      updateWindows();
    }
  }
}

void initLight(void)
{
  // params for point light, light position doesn't change
  GLfloat light_position[] = { 0.0, 0.0, 5.0, 1.0}; // must have 1 on the homogenous coord to be a point source (attenuation needed) 

  glClearColor (0.0, 0.0, 0.0, 0.0);
  glShadeModel (GL_SMOOTH);

  glLightfv(GL_LIGHT0, GL_POSITION, light_position);

  glEnable(GL_LIGHTING);
  glEnable(GL_LIGHT0);

  glEnable(GL_DEPTH_TEST);
  glEnable(GL_COLOR_MATERIAL);
}

int main(int argc, char **argv)
{
  pthread_t tid;
  // first Xlib function to call in a multi threaded program
  XInitThreads();
  glClearColor(1.0, 1.0, 1.0, 1.0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  dpy = XOpenDisplay(NULL);
  dpy2 = XOpenDisplay(NULL);

  if (dpy == NULL)
  {
    printf("\n\tcannot connect to Xserver\n\n");
    exit(0);
  }

  root = DefaultRootWindow(dpy);

  vi = glXChooseVisual(dpy, 0, att);
  vi2 = glXChooseVisual(dpy2, 0, att2);

  if (vi == NULL)
  {
    printf("\n\tno appropriate visual found\n\n");
    exit(0);
  }
  else
  {
    printf("\n\tvisual %p selected\n", (void *)vi->visualid);
  }

  cmap = XCreateColormap(dpy, root, vi->visual, AllocNone);
  cmap2 = XCreateColormap(dpy2, root, vi2->visual, AllocNone);

  swa.colormap = cmap;
  swa.event_mask = ExposureMask | KeyPressMask | FocusChangeMask;
  swa.backing_store = NotUseful;

  swa2.colormap = cmap2;
  swa2.event_mask = ExposureMask | KeyPressMask | FocusChangeMask;
  swa2.backing_store = NotUseful;

  win = XCreateWindow(dpy, root, 0, 0, 1000, 1000, 0, vi->depth, InputOutput, vi->visual, CWColormap | CWEventMask, &swa);
  win2 = XCreateWindow(dpy2, root, 0, 0, 1000, 1000, 0, vi2->depth, InputOutput, vi2->visual, CWColormap | CWEventMask, &swa2);

  XMapWindow(dpy, win);
  XMapWindow(dpy2, win2);

  XStoreName(dpy, win, "Assignment 2: Shader Based Rendering");
  XStoreName(dpy2, win2, "Assignment 1: Fixed Pipeline Rendering");

  glc = glXCreateContext(dpy, vi, NULL, GL_TRUE);
  glc2 = glXCreateContext(dpy2, vi2, NULL, GL_TRUE);

  //initialize lighting
  initLight();

  if (pthread_create(&tid, NULL, updateLoop, NULL))
  {
    printf("pthread_create() returned non-zero for thread 2\n");
  }

  pthread_join(tid, NULL);

  return 0;
}

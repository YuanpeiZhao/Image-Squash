#include <windows.h>
#include <iostream>
#include <cassert>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "gl_core_3_3.h"
#include <GL/glew.h>
#include <GL/freeglut.h>
#include <GL/glext.h>
#include "imgui_impl_glut.h"
#include "util.hpp"
#include "mesh.hpp"

//#pragma comment (lib, "glew32.lib")

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

using namespace std;
using namespace glm;

#define POINT_X 32
#define POINT_Y 32

// Global state
GLint width, height;
unsigned int viewmode;	// View triangle or obj file
GLuint shader;			// Shader program
GLuint uniXform;		// Shader location of xform mtx
GLuint vao;				// Vertex array object
GLuint vbuf;			// Vertex buffer
GLuint EBO;
GLuint texture;
GLsizei vcount;			// Number of vertices
Mesh* mesh;				// Mesh loaded from .obj file
// Camera state
vec3 camCoords;			// Spherical coordinates (theta, phi, radius) of the camera
bool camRot;			// Whether the camera is currently rotating
vec2 camOrigin;			// Original camera coordinates upon clicking
vec2 mouseOrigin;		// Original mouse coordinates upon clicking

vec3 points[POINT_X][POINT_Y];
vec2 texureCoords[POINT_X][POINT_Y];

unsigned int indices[6 * (POINT_X - 1) * (POINT_Y - 1)];
float vertices[5 * POINT_X * POINT_Y];

// Constants
const int MENU_VIEWMODE = 0;		// Toggle view mode
const int MENU_EXIT = 1;			// Exit application
const int VIEWMODE_TRIANGLE = 0;	// View triangle
const int VIEWMODE_OBJ = 1;			// View obj-loaded mesh

// Initialization functions
void initState();
void initGLUT(int* argc, char** argv);
void initOpenGL();
void initViewPlane();
void initPoints();
void initVertices();

GLuint LoadTexture(const char* filename);

// Callback functions
void display();
void keyRelease(unsigned char key, int x, int y);
void mouseBtn(int button, int state, int x, int y);
void mouseMove(int x, int y);
void idle();
void menu(int cmd);
void cleanup();
void draw_gui();

int main(int argc, char** argv) {
	try {
		// Initialize
		initState();
		initGLUT(&argc, argv);
		initOpenGL();
		initViewPlane();

	} catch (const exception& e) {
		// Handle any errors
		cerr << "Fatal error: " << e.what() << endl;
		cleanup();
		return -1;
	}

	// Execute main loop
	glutMainLoop();

	return 0;
}

//Draw the ImGui user interface
void draw_gui()
{
	ImGui_ImplGlut_NewFrame();

	ImGui::Begin("Yuanpei Zhao");
	ImGui::End();

	static bool show_test = false;
	ImGui::ShowTestWindow(&show_test);

	ImGui::Render();
}

void initState() {
	// Initialize global state
	width = 0;
	height = 0;
	viewmode = VIEWMODE_TRIANGLE;
	shader = 0;
	uniXform = 0;
	vao = 0;
	vbuf = 0;
	vcount = 0;
	mesh = NULL;

	camCoords = vec3(0.0, 0.0, 1.8);
	camRot = false;
}

void initGLUT(int* argc, char** argv) {
	// Set window and context settings
	width = 800; height = 800;
	glutInit(argc, argv);
	glutInitWindowSize(width, height);
	glutInitContextVersion(3, 3);
	glutInitContextProfile(GLUT_CORE_PROFILE);
	glutInitDisplayMode(GLUT_RGBA | GLUT_DEPTH | GLUT_DOUBLE);
	// Create the window
	glutCreateWindow("FreeGlut Window");

	// Create a menu
	glutCreateMenu(menu);
	glutAddMenuEntry("Toggle view mode", MENU_VIEWMODE);
	glutAddMenuEntry("Exit", MENU_EXIT);
	glutAttachMenu(GLUT_RIGHT_BUTTON);

	// GLUT callbacks
	glutDisplayFunc(display);
	glutKeyboardUpFunc(keyRelease);
	glutMouseFunc(mouseBtn);
	glutMotionFunc(mouseMove);
	glutIdleFunc(idle);
	glutCloseFunc(cleanup);
}

void initOpenGL() {
	// Set clear color and depth
	glClearColor(0.0f, 0.1f, 0.1f, 1.0f);
	glClearDepth(1.0f);
	// Enable depth testing
	glEnable(GL_DEPTH_TEST);

	// Compile and link shader program
	vector<GLuint> shaders;
	shaders.push_back(compileShader(GL_VERTEX_SHADER, "sh_v.glsl"));
	shaders.push_back(compileShader(GL_FRAGMENT_SHADER, "sh_f.glsl"));
	shader = linkProgram(shaders);
	// Release shader sources
	for (auto s = shaders.begin(); s != shaders.end(); ++s)
		glDeleteShader(*s);
	shaders.clear();
	// Locate uniforms
	uniXform = glGetUniformLocation(shader, "xform");
	assert(glGetError() == GL_NO_ERROR);

	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
}

void initPoints() {

	// Init all vertices
	for (int y = 0; y < POINT_X; y++)
	{
		for (int x = 0; x < POINT_Y; x++)
		{
			points[y][x] = vec3(-1.0+2.0*x/(POINT_X-1), 1.0-2.0*y/(POINT_Y-1), 0.0);
		}
	}

	// Init the indeces of triangles
	for (int i = 0; i < 2 * (POINT_X - 1) * (POINT_Y - 1); i++)
	{
		int NY = (i / 2) % (POINT_X - 1);
		int NX = (i / 2) / (POINT_X - 1);
		if (i % 2 == 0)
		{
			indices[3 * i] = POINT_X * NX + NY;
			indices[3 * i + 1] = POINT_X * NX + NY + 1;
			indices[3 * i + 2] = POINT_X * (NX + 1) + NY;
		}
		else
		{
			indices[3 * i] = POINT_X * NX + NY + 1;
			indices[3 * i + 1] = POINT_X * (NX + 1) + NY + 1;
			indices[3 * i + 2] = POINT_X * (NX + 1) + NY;
		}
	}

	// Init Texture Coords
	for (int y = 0; y < POINT_X; y++)
	{
		for (int x = 0; x < POINT_Y; x++)
		{
			texureCoords[y][x] = vec2(x / POINT_X, 1.0 - y / POINT_Y);
		}
	}
}

void initVertices() {
	for (int i = 0; i < POINT_X * POINT_Y; i++)
	{
		vertices[5 * i] = points[i / POINT_X][i % POINT_X].x;
		vertices[5 * i + 1] = points[i / POINT_X][i % POINT_X].y;
		vertices[5 * i + 2] = points[i / POINT_X][i % POINT_X].z;
		vertices[5 * i + 3] = texureCoords[i / POINT_X][i % POINT_X].x;
		vertices[5 * i + 4] = texureCoords[i / POINT_X][i % POINT_X].y;
	}
}

void initViewPlane() {

	initPoints();
	initVertices();

	glGenVertexArrays(1, &vao);
	glGenBuffers(1, &vbuf);
	glGenBuffers(1, &EBO);

	glBindVertexArray(vao);

	glBindBuffer(GL_ARRAY_BUFFER, vbuf);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_DYNAMIC_DRAW);

	// position attribute
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	// texture coord attribute
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);

	texture = LoadTexture("test.jpg");
}

GLuint LoadTexture(const char* filename)
{

	GLuint texture;

	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture); // all upcoming GL_TEXTURE_2D operations now have effect on this texture object
	// set the texture wrapping parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	// set texture wrapping to GL_REPEAT (default wrapping method)
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	// set texture filtering parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	// load image, create texture and generate mipmaps
	int width, height, nrChannels;
	// The FileSystem::getPath(...) is part of the GitHub repository so we can find files on any IDE/platform; replace it with your own image path.
	unsigned char* data = stbi_load(filename, &width, &height, &nrChannels, 0);

	if (data)
	{
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	else
	{
		std::cout << "Failed to load texture" << std::endl;
	}
	stbi_image_free(data);

	return texture;
}

void display() {
	try {

		draw_gui();
		// Clear the back buffer
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Get ready to draw
		glUseProgram(shader);

		mat4 xform;
		float aspect = (float)width / (float)height;
		// Create perspective projection matrix
		mat4 proj = perspective(45.0f, aspect, 0.1f, 100.0f);
		// Create view transformation matrix
		mat4 view = translate(mat4(1.0f), vec3(0.0, 0.0, -camCoords.z));
		mat4 rot = rotate(mat4(1.0f), radians(camCoords.y), vec3(1.0, 0.0, 0.0));
		rot = rotate(rot, radians(camCoords.x), vec3(0.0, 1.0, 0.0));
		xform = proj * view * rot;

		texture = LoadTexture("test.jpg");
		// bind Texture
		glBindTexture(GL_TEXTURE_2D, texture);

		glBindVertexArray(vao);
		// Send transformation matrix to shader
		glUniformMatrix4fv(uniXform, 1, GL_FALSE, value_ptr(xform));

		// Draw the triangle
		//glDrawArrays(GL_TRIANGLES, 0, vcount);
		//glBindVertexArray(0);

		glDrawElements(GL_TRIANGLES, 6*(POINT_X-1)*(POINT_Y-1), GL_UNSIGNED_INT, 0);

		assert(glGetError() == GL_NO_ERROR);

		// Revert context state
		//glUseProgram(0);

		//draw_gui();

		// Display the back buffer
		glutSwapBuffers();

	} catch (const exception& e) {
		cerr << "Fatal error: " << e.what() << endl;
		glutLeaveMainLoop();
	}
}

void keyRelease(unsigned char key, int x, int y) {
	switch (key) {
	case 27:	// Escape key
		menu(MENU_EXIT);
		break;
	}
}

void mouseBtn(int button, int state, int x, int y) {
	if (state == GLUT_DOWN && button == GLUT_LEFT_BUTTON) {
		// Activate rotation mode
		//camRot = true;
		//camOrigin = vec2(camCoords);
		//mouseOrigin = vec2(x, y);
		vertices[0] = 0.0f;
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(float), vertices);
		glutPostRedisplay();
	}
	if (state == GLUT_UP && button == GLUT_LEFT_BUTTON) {
		// Deactivate rotation
		//camRot = false;
		vertices[0] = 1.0f;
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(float), vertices);
		glutPostRedisplay();
	}
	if (button == 3) {
		camCoords.z = clamp(camCoords.z - 0.1f, 0.1f, 10.0f);
		glutPostRedisplay();
	}
	if (button == 4) {
		camCoords.z = clamp(camCoords.z + 0.1f, 0.1f, 10.0f);
		glutPostRedisplay();
	}
}

void mouseMove(int x, int y) {
	if (camRot) {
		// Convert mouse delta into degrees, add to rotation
		float rotScale = min(width / 450.0f, height / 270.0f);
		vec2 mouseDelta = vec2(x, y) - mouseOrigin;
		vec2 newAngle = camOrigin + mouseDelta / rotScale;
		newAngle.y = clamp(newAngle.y, -90.0f, 90.0f);
		while (newAngle.x > 180.0f) newAngle.x -= 360.0f;
		while (newAngle.y < -180.0f) newAngle.y += 360.0f;
		if (length(newAngle - vec2(camCoords)) > FLT_EPSILON) {
			camCoords.x = newAngle.x;
			camCoords.y = newAngle.y;
			glutPostRedisplay();
		}
	}
}

void idle() {}

void menu(int cmd) {
	switch (cmd) {
	case MENU_VIEWMODE:
		viewmode = (viewmode + 1) % 2;
		glutPostRedisplay();	// Tell GLUT to redraw the screen
		break;

	case MENU_EXIT:
		glutLeaveMainLoop();
		break;
	}
}

void cleanup() {
	// Release all resources
	if (shader) { glDeleteProgram(shader); shader = 0; }
	uniXform = 0;
	if (vao) { glDeleteVertexArrays(1, &vao); vao = 0; }
	if (vbuf) { glDeleteBuffers(1, &vbuf); vbuf = 0; }
	vcount = 0;
	if (mesh) { delete mesh; mesh = NULL; }
}
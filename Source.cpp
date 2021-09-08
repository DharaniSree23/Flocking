#include <GL/glut.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <iostream>
#include <glm/vec3.hpp> // glm::vec3
#include <glm/vec4.hpp> // glm::vec4                                    
#include <glm/mat4x4.hpp> 
#include <glm/gtc/matrix_transform.hpp>
#include<vector>
#include<glm/gtx/scalar_multiplication.hpp>
#include <random>
#include <stack>
#include <GL/gl.h>
#include <list>
#include "imgui.h"
#include "imgui_impl_glfw_gl3.h"
#include <GLFW\glfw3.h>


using namespace std;
double h = 0.03; // timestep
double obstaclex = 2.5;
double obstacley = -2.5;
double obstaclez = 0;
double RadiusOfObstacle = 0.3;

bool show_demo_window = true;
bool show_another_window = false;
ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
GLFWwindow* window;

std::default_random_engine generator;
std::stack<int> mystack;

glm::vec3 PointOfSphere = glm::vec3(obstaclex, obstacley, obstaclez);

double Pi = 3.14;
glm::vec3 VortexCenter = glm::vec3(0.0f, 0.0f, 0.0f);
double VortexSpeed = 1.3;
glm::vec3 VortexAngle = glm::vec3(Pi / 8, Pi / 9, Pi / 5);

typedef union v2f_t {
	struct {
		float x;
		float y;
	};
	float v[2];
} v2f_t;

typedef std::list<v2f_t> v2flist_t;

v2flist_t sphere_centers;

int rotateon;

double xmin, xmax, ymin, ymax, zmin, zmax;
double maxdiff;

int lastx, lasty;
int xchange, ychange;
float spin = 0.0;
float spinup = 0.0;

int random(int m)
{
	return rand() % m;
}


//assigning randomvelocity
glm::vec3 uniformDirectionalVelocity() {

	double x, y, z;

	std::uniform_real_distribution<double> distribution(-180, 180);
	double theeta = distribution(generator);
	std::uniform_real_distribution<double> distribution2(0, 1);
	double f = distribution2(generator);
	double delta = 80;
	double Phi = sqrt(f) * delta;

	theeta = theeta * 3.14 / 180;
	Phi = Phi * 3.14 / 180;

	x = sin(Phi) * cos(theeta);
	y = sin(theeta) * sin(Phi);
	z = cos(Phi);

	glm::vec3 Uz = glm::vec3(0, 0, 1);
	glm::vec3 a = glm::vec3(1, 0, 0);
	glm::vec3 vector2 = glm::cross(a, Uz);
	glm::vec3 Ux = vector2 / glm::length(vector2);
	glm::vec3 Uy = glm::cross(Uz, Ux);

	glm::mat3 M = glm::mat3(Ux, Uy, Uz);
	glm::vec3 V = glm::vec3(x, y, z);

	glm::vec3 C = M * V;
	return C;
}

struct Particle {
	Particle()
	{
		Position = glm::vec3(10, 50, 0);
		Velocity = glm::vec3(0, 0, 0); //default vel
		NetAcceleration = glm::vec3(0, 0, 0);
		Age = 1;
		active = false;
	}
	glm::vec3 Position;
	glm::vec3 Velocity;
	glm::vec3 NetAcceleration;
	int Age;
	bool active;
};

double TotalParticles = 50;
Particle P[50];
int inactivestack[50];
int inactivecount;
int NumberParticles = 0;

void GenerateParticle() {

	int FreeIndex = mystack.top();
	mystack.pop();
	P[FreeIndex].Position = uniformDirectionalVelocity();
	P[FreeIndex].Velocity = uniformDirectionalVelocity() * 0;
	P[FreeIndex].active = true;
	NumberParticles++;

}

glm::vec3 VelocityOfVortex(glm::vec3 Point)
{
	glm::vec3 VDis = glm::vec3(Point - VortexCenter);
	glm::vec3 VDirec = glm::cross(VortexAngle, VDis);
	return(VDirec);
}

double factor(glm::vec3 Vector)
{
	glm::vec3 VDis = glm::vec3(Vector - VortexCenter);
	glm::vec3 VDirec = glm::cross(VortexAngle, VDis);
	double Fac = 1 + pow(VDis.x, 2) + pow(VDis.y, 2) + pow(VDis.z, 2);
	return 1 / (Fac + VortexSpeed);
}

//to draw sphere using mouse click

void simulate(void) {

	int const window_width = glutGet(GLUT_WINDOW_WIDTH);
	int const window_height = glutGet(GLUT_WINDOW_HEIGHT);
	float const window_aspect = (float)window_width / (float)window_height;

	glClearColor(0.9, 0.9, 0, 1.0);
	glClearDepth(1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glViewport(0, 0, window_width, window_height);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	if (window_aspect > 1.) {
		glOrtho(-window_aspect, window_aspect, -1, 1, -1, 1);
	}
	else {
		glOrtho(-1, 1, -1 / window_aspect, 1 / window_aspect, -1, 1);
	}

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();


	GLfloat const light_pos[4] = { -1.00,  1.00,  1.00, 0. };
	GLfloat const light_color[4] = { 0.85,  0.90,  0.70, 1. };
	GLfloat const light_ambient[4] = { 0.10,  0.10,  0.30, 1. };
	glLightfv(GL_LIGHT0, GL_POSITION, light_pos),
		glLightfv(GL_LIGHT0, GL_DIFFUSE, light_color);
	glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);

	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);

	glEnable(GL_DEPTH_TEST);

	for (v2flist_t::iterator sc = sphere_centers.begin();
		sc != sphere_centers.end();
		sc++) {

		glPushMatrix();
		glTranslatef(sc->x, sc->y, 0);
		glutSolidSphere(0.1, 31, 10);
		glPopMatrix();
	}

	glutSwapBuffers();
}

void display(void)
{
	

	if (NumberParticles < TotalParticles) {
		GenerateParticle();
	}


	GLfloat box_ambient[] = { 0.1, 0.1, 0.1 };
	GLfloat smallr00[] = { 0.8, 0.0, 0.0 };
	GLfloat small0g0[] = { 0.0, 0.8, 0.0 };
	GLfloat small00b[] = { 0.0, 0.0, 0.8 };
	GLfloat smallrg0[] = { 0.8, 0.8, 0.0 };
	GLfloat smallr0b[] = { 0.8, 0.0, 0.8 };
	GLfloat small0gb[] = { 0.0, 0.8, 0.8 };
	GLfloat smallrgb[] = { 0.8, 0.8, 0.8 };

	GLfloat box_diffuse[] = { 0.7, 0.7, 0.7 };
	GLfloat box_specular[] = { 0.1, 0.1, 0.1 };
	GLfloat box_shininess[] = { 1.0 };
	GLfloat ball_ambient[] = { 0.4, 0.0, 0.0 };
	GLfloat ball_diffuse[] = { 0.3, 0.0, 0.0 };
	GLfloat ball_specular[] = { 0.3, 0.3, 0.3 };
	GLfloat ball_shininess[] = { 10.0 };

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glPushMatrix();

	//rotate the view
	glRotatef(spinup, 1.0, 0.0, 0.0);
	glRotatef(spin, 0.5, 1.0, 0.3);


	//Obstacle1
	glColor3f(0.8, 0.8, 0.0);
	glPushMatrix();
	glTranslated(obstaclex, obstacley, obstaclez);
	glutSolidSphere(RadiusOfObstacle, 10, 10);
	glPopMatrix();

	//glPointSize(10);
	glColor3f(0.5, 0.8, 0.0);
	//ImGui_ImplGlfwGL3_NewFrame();

	// Flocking
	for (int i = 0; i < TotalParticles; i++) {

		if (P[i].active == true) {

			if (i == 0) {

				//LeadBoid

				glm::vec3 VelocityLB = glm::vec3(0, 0, 0);

				//Vortices
				VelocityLB.x += ((VelocityOfVortex(P[i].Position).x) - P[i].Velocity.x) * factor(P[i].Position);
				VelocityLB.y += ((VelocityOfVortex(P[i].Position).y) - P[i].Velocity.y) * factor(P[i].Position);
				VelocityLB.z += ((VelocityOfVortex(P[i].Position).z) - P[i].Velocity.z) * factor(P[i].Position);

				P[i].Velocity = VelocityLB;

				glm::vec3 NewVelocity = P[i].Velocity + P[i].NetAcceleration * h;
				glm::vec3 NewPosition = P[i].Position + P[i].Velocity * h;
				glColor3f(0.0, 0.0, 0.8);


				P[i].Velocity = NewVelocity;
				P[i].Position = NewPosition;


				glPushMatrix();
				glTranslated(P[i].Position.x, P[i].Position.y, P[i].Position.z);
				glVertex3d(P[i].Position.x, P[i].Position.y, P[i].Position.z);
				glutSolidSphere(0.1, 10, 10);
				glPopMatrix();

			}
			else {
				glColor3f(0.0, 0.8, 0.0);
			}

			glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, ball_ambient);
			glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, ball_diffuse);
			glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, ball_specular);
			glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, ball_shininess);


			glPushMatrix();
			glTranslated(P[i].Position.x, P[i].Position.y, P[i].Position.z);
			//glVertex3d(P[i].Position.x, P[i].Position.y, P[i].Position.z);
			glutSolidSphere(0.05, 10, 10);
			glPopMatrix();



			//glm::vec3 AirResistance = (d*P[i].Velocity) / m;
			//glm::vec3 WindVelocity = glm::vec3(0.0f, 0.0f, 0.0f);
			//glm::vec3 Wind = (d*WindVelocity) / m;
			glm::vec3 Gravity = glm::vec3(0.0f, 0.0f, 0.0f);
			glm::vec3 Acceleration = glm::vec3(0.0f, 0.0f, 00.0f);
			glm::vec3 AvoidanceAcceleration = glm::vec3(0.0f, 0.0f, 00.0f);

			P[i].NetAcceleration = Acceleration + Gravity;

			for (int j = 0; j < TotalParticles; j++) {

				if (j == i) {
					continue;
				}

				float dis = glm::distance(P[j].Position, P[i].Position); //distanceBetweenBoids

				if ((dis > 0) && (dis < 10)) {

					glm::vec3 VectorP = glm::vec3(P[j].Position - P[i].Position);
					glm::vec3 UnitVectorP = glm::normalize(VectorP);

					// Seperation
					double ka = 0.5;
					glm::vec3 CollisionAvoidance = -ka * glm::vec3(UnitVectorP) / dis;

					//Alignment
					double kv = 0.1;
					glm::vec3 Velocitymatching = kv * glm::vec3(P[j].Velocity - P[i].Velocity);

					//Cohesion
					double kc = 0.3;
					glm::vec3 Centering = kc * glm::vec3(P[j].Position - P[i].Position);

					// Obstacle avoidance 
					glm::vec3 DisVector = glm::vec3(P[i].Position - PointOfSphere);
					double AvoidanceDis = glm::length(DisVector);
					glm::vec3 UnitVectorDis = (glm::vec3(DisVector)) / AvoidanceDis;
					double Magnitude = RadiusOfObstacle - AvoidanceDis;
					double Ps = 2;
					double residue = pow(Magnitude, Ps);
					double Gs = 0.003;
					glm::vec3 AvoidanceAcceleration = -Gs * (glm::vec3(UnitVectorDis)) / residue;


					P[i].NetAcceleration += Centering + Velocitymatching + CollisionAvoidance + AvoidanceAcceleration;
				}
			}

			glm::vec3 NewVelocity = P[i].Velocity + P[i].NetAcceleration * h;
			glm::vec3 NewPosition = P[i].Position + P[i].Velocity * h;

			P[i].Age = P[i].Age + 1;
			if (false) {
				P[i].Velocity = glm::vec3(0, 0, 0);
				P[i].NetAcceleration = glm::vec3(0, 0, 0);
				//P[i].active = false;
				P[i].active = true;

			}


			P[i].Velocity = NewVelocity;
			P[i].Position = NewPosition;

		}
	}

	glPopMatrix();
	glutSwapBuffers();

	//simulate();
}

void init(void)
{

	glClearColor(0.0, 0.0, 0.0, 0.0);
	// Enable Z-buffering, backface culling, and lighting
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	//glShadeModel(GL_SMOOTH);
	//glEnable(GL_LIGHTING);
	//glEnable(GL_LIGHT0);
	//glEnable(GL_LIGHT1);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(60.0, 1.0, 1, 600);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	// Set eye point and lookat point
	//gluLookAt(0, 225, 300, 0, 0, 0, 0, 1, 0);
	gluLookAt(0, 0, 10, 0, 0, 0, 0, 1, 0);

	// Set up lights
	GLfloat light0color[] = { 1.0, 1.0, 1.0 };
	GLfloat light0pos[] = { 0, 500, 300 };
	GLfloat light1color[] = { 1.0, 1.0, 1.0 };
	GLfloat light1pos[] = { 300, 300, 300 };
	glLightfv(GL_LIGHT0, GL_POSITION, light0pos);
	glLightfv(GL_LIGHT0, GL_AMBIENT, light0color);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, light0color);
	glLightfv(GL_LIGHT0, GL_SPECULAR, light0color);
	glLightfv(GL_LIGHT1, GL_POSITION, light1pos);
	glLightfv(GL_LIGHT1, GL_AMBIENT, light1color);
	glLightfv(GL_LIGHT1, GL_DIFFUSE, light1color);
	glLightfv(GL_LIGHT1, GL_SPECULAR, light1color);


	for (int i = 0; i < TotalParticles; ++i) {
		mystack.push(i);
	}

}

void reshapeFunc(GLint newWidth, GLint newHeight)
{
	if (newWidth > newHeight) // Keep a square viewport
		glViewport((newWidth - newHeight) / 2, 0, newHeight, newHeight);
	else
		glViewport(0, (newHeight - newWidth) / 2, newWidth, newWidth);
	init();
	glutPostRedisplay();
}

void rotateview(void)
{
	if (rotateon) {
		spin += xchange / 5000.0;
		if (spin >= 360.0) spin -= 360.0;
		if (spin < 0.0) spin += 360.0;
		spinup -= ychange / 250.0;
		if (spinup > 89.0) spinup = 89.0;
		if (spinup < -89.0) spinup = -89.0;
	}
	glutPostRedisplay();
}

void mouseclick(int button, int state, int mouse_x, int mouse_y)
{
	int const window_width = glutGet(GLUT_WINDOW_WIDTH);
	int const window_height = glutGet(GLUT_WINDOW_HEIGHT);
	float const window_aspect = (float)window_width / (float)window_height;

	v2f_t const sc = {
		(window_aspect > 1.0 ? window_aspect : 1.)*
		(((float)mouse_x / (float)window_width) * 2. - 1.),

		(window_aspect < 1.0 ? 1. / window_aspect : 1.)*
		(-((float)mouse_y / (float)window_height) * 2. + 1.)
	};
	sphere_centers.push_back(sc);

	glutPostRedisplay();
}

void mouse(int button, int state, int x, int y)
{
	switch (button) {
	case GLUT_LEFT_BUTTON:
		if (state == GLUT_DOWN) {
			lastx = x;
			lasty = y;
			xchange = 0;
			ychange = 0;
			rotateon = 1;
		}
		else if (state == GLUT_UP) {
			xchange = 0;
			ychange = 0;
			rotateon = 0;
		}
		break;

	default:
		break;
	}
}

void motion(int x, int y)
{
	xchange = x - lastx;
	ychange = y - lasty;
}

int main(int argc, char** argv)
{

	if (!glfwInit())
		return -1;

	window = glfwCreateWindow(640, 480, "UI", NULL, NULL);
	if (!window)
	{
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);

	ImGui::CreateContext();
	ImGui_ImplGlfwGL3_Init(window, true);
	ImGui::StyleColorsDark();


	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
	glutInitWindowSize(1000, 1000);
	glutInitWindowPosition(800, 100);
	glutCreateWindow("Flocking");
	init();
	rotateon = 0;



	//glutSpecialFunc(SpecialInput);
	glutDisplayFunc(display);
	glutMouseFunc(mouse);
	glutMouseFunc(mouseclick);
	glutMotionFunc(motion);
	glutIdleFunc(rotateview);
	glutReshapeFunc(reshapeFunc);
	//update(0);
	glutMainLoop();

	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();
		ImGui_ImplGlfwGL3_NewFrame();

		ImGui::Begin("my window");
			static float f = 0.0f;
			static int counter = 0;
			ImGui::Text("Flocking");                           // Display some text (you can use a format string too)
			ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f    
			ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color
			if (ImGui::Button("Button"))                            // Buttons return true when clicked (NB: most widgets return true when edited/activated)
				counter++;
			ImGui::SameLine();
		ImGui::Text("counter = %d", counter);

		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
		
		ImGui::End();
		int display_w, display_h;
		glfwGetFramebufferSize(window, &display_w, &display_h);
		glViewport(0, 0, display_w, display_h);
		glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
		glClear(GL_COLOR_BUFFER_BIT);
		ImGui::Render();
		ImGui_ImplGlfwGL3_RenderDrawData(ImGui::GetDrawData());
		glfwSwapBuffers(window);
	}

	ImGui_ImplGlfwGL3_Shutdown();
	ImGui::DestroyContext();
	glfwTerminate();


	return 0;
}
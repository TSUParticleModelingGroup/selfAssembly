// gcc saViewConfig.c -o view -lglut -lm -lGLU -lGL
#include <GL/glut.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define XWindowSize 1000
#define YWindowSize 1000

#define PI 3.141592654
#define NO 0
#define YES 1
#define DIAMETER_PS 1.0 // Diameter of polystyrene spheres 1 micron
#define DIAMETER_NIPAM 0.08 // Diameter of polyNIPAM microgel particles 80 nanometers

//typedefs to make .c code more compatable .cu
typedef struct{	
		float x;
		float y;
		float z;
		float w;
		} float4; 
		
typedef struct{	float x;
		float y;
		float z;
		} float3;
		
//globals
int g_total_runs, g_totalBodies;
int g_selected_run;
FILE *g_data_file_in;
const float dth = 0.0001;
float eye = 2.0;
float4 *pos;
int g_win;

void rest_center_of_mass(float4 *pos)
{
	int i;
	float x_center, y_center, z_center, total_mass;

	x_center = 0.0;
	y_center = 0.0;
	z_center = 0.0;
	total_mass = 0.0;

	for(i = 0; i < g_totalBodies; i++)
	{
		x_center += pos[i].x*pos[i].w;
		y_center += pos[i].y*pos[i].w;
		z_center += pos[i].z*pos[i].w;
		total_mass += pos[i].w;
	}

	x_center /= total_mass;
	y_center /= total_mass;
	z_center /= total_mass;

	for(i = 0; i < g_totalBodies; i++)
	{
		pos[i].x -= x_center;
		pos[i].y -= y_center;
		pos[i].z -= z_center;
	}
}

void get_positions(float4 *pos)
{
	int i,j;
	int run;
	
	glClear(GL_COLOR_BUFFER_BIT);
	glClear(GL_DEPTH_BUFFER_BIT);
	for(i = 0;i < g_total_runs;i++)
	{
		fscanf(g_data_file_in,"%d", &run);
		
		for(j = 0;j < g_totalBodies;j++)
		{
			fscanf(g_data_file_in,"%f %f %f %f", &pos[j].x, &pos[j].y, &pos[j].z, &pos[j].w);
		}
		if(run == g_selected_run) break;
	}
	printf("\n Selected Run %d\n", run);
}

void draw_picture(float4 *pos)
{
	int j, k;
	float dx,dy,dz;
	
	rest_center_of_mass(pos);
	glColor3f (0.2,0.2,1.0);
	for(j = 0; j < g_totalBodies ; j++)
	{
		glPushMatrix();
		glTranslatef(pos[j].x, pos[j].y, pos[j].z);
		glutSolidSphere(0.2,20,20);
		glPopMatrix();
	}
	
	glColor3f (1.0,1.0,1.0);
	for(j = 0; j < g_totalBodies - 1; j++)
	{
		for(k = j + 1; k < g_totalBodies; k++)
		{
			dx = pos[k].x - pos[j].x;
			dy = pos[k].y - pos[j].y;
			dz = pos[k].z - pos[j].z;
			if(sqrt(dx*dx + dy*dy + dz*dz) < DIAMETER_PS + DIAMETER_NIPAM)
			{
				glBegin(GL_LINES);
				glVertex3f(pos[j].x, pos[j].y, pos[j].z);
				glVertex3f(pos[k].x, pos[k].y, pos[k].z);		
				glEnd();
			}
		}
	}
}

void reshape(int w, int h)
{
	glViewport(0, 0, (GLsizei) w, (GLsizei) h);

	glMatrixMode(GL_PROJECTION);

	glLoadIdentity();

	glFrustum(-0.2, 0.2, -0.2, 0.2, 0.2, 50.0);
	//gluLookAt(2.0, 2.0, 2.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0);

	glMatrixMode(GL_MODELVIEW);
}

void Display(void)
{
	glClear(GL_COLOR_BUFFER_BIT);
	glClear(GL_DEPTH_BUFFER_BIT);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	draw_picture(pos);
	glutSwapBuffers();
}

void KeyPressed(unsigned char key, int x, int y)
{
	if(key == 'q')
	{
		glutDestroyWindow(g_win);
		exit(0);
	}
	if(key == 'i')
	{
		glRotatef(10.0, 1.0, 0.0, 0.0);
		Display();
	}
	if(key == 'm')
	{
		glRotatef(-10.0, 1.0, 0.0, 0.0);
		Display();
	}
	if(key == 'k')
	{
		glRotatef(10.0, 0.0, 1.0, 0.0);
		Display();
	}
	if(key == 'j')
	{
		glRotatef(-10.0, 0.0, 1.0, 0.0);
		Display();
	}
	if(key == '8')
	{
		glTranslatef(0.0, 0.0, 0.1);
		Display();
	}
	if(key == '2')
	{
		glTranslatef(0.0, 0.0, -0.1);
		Display();
	}
	if(key == '4')
	{
		glTranslatef(0.0, 0.1, 0.0);
		Display();
	}
	if(key == '6')
	{
		glTranslatef(0.0, -0.1, 0.0);
		Display();
	}
	if(key == '9')
	{
		glTranslatef(0.1, 0.0, 0.0);
		Display();
	}
	if(key == '1')
	{
		glTranslatef(-0.1, 0.0, 0.0);
		Display();
	}
}

int main(int argc, char* argv[])
{
	g_selected_run = atoi(argv[1]);
	printf("g_selected_run %d\n", g_selected_run);
	
	char buffer[50];
	sprintf(buffer, "Configuration %d",g_selected_run);
	
	g_data_file_in = fopen("final_positions_out","r");
	if(g_data_file_in == NULL) 
	{
		printf("error opening positions_out\n");
		exit(0);
	}
	
	fseek(g_data_file_in, 0, SEEK_SET);
	fscanf(g_data_file_in,"%d %d", &g_total_runs, &g_totalBodies);
	pos = (float4*)malloc(g_totalBodies*sizeof(float4));
	get_positions(pos);
	fclose(g_data_file_in);
	
	
	glutInit(&argc,argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_DEPTH | GLUT_RGB);
	glutInitWindowSize(XWindowSize,YWindowSize);
	glutInitWindowPosition(0,0);
	g_win = glutCreateWindow(buffer);
	GLfloat light_position[] = {1.0, 1.0, 1.0, 0.0};
	GLfloat light_ambient[]  = {0.0, 0.0, 0.0, 1.0};
	GLfloat light_diffuse[]  = {1.0, 1.0, 1.0, 1.0};
	GLfloat light_specular[] = {1.0, 1.0, 1.0, 1.0};
	GLfloat lmodel_ambient[] = {0.2, 0.2, 0.2, 1.0};
	GLfloat mat_specular[]   = {1.0, 1.0, 1.0, 1.0};
	GLfloat mat_shininess[]  = {10.0};
	glClearColor(0.0, 0.0, 0.0, 0.0);
	glShadeModel(GL_SMOOTH);
	glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
	glLightfv(GL_LIGHT0, GL_POSITION, light_position);
	glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
	glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular);
	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, lmodel_ambient);
	glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
	glMaterialfv(GL_FRONT, GL_SHININESS, mat_shininess);
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glEnable(GL_COLOR_MATERIAL);
	glEnable(GL_DEPTH_TEST);
	//glFrustum(-0.2, 0.2, -0.2, 0.2, 0.2, 50.0);
	gluLookAt(2.0, 2.0, 2.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0);
	//glFrustum(-0.2, 0.2, -0.2, 0.2, 0.2, 50.0);
	glutKeyboardFunc(KeyPressed);
	//glutMouseFunc(Mouse);
	glutDisplayFunc(Display);
	glutReshapeFunc(reshape);
	glutMainLoop();
	return 0;
}







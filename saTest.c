// gcc saTest.c -o test -lglut -lm -lGLU -lGL
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

// function prototypes
void KeyPressed(unsigned char key, int x, int y);
void Display(void);

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
		
		
//constants globals
const int NUMBER_OF_BODIES = 6;
const int NUMBER_OF_RUNS = 100;

const int ITERATIONS_BETWEEN_VIEWS = 100;
const float STOP_TIME = 25.0;
const float DT = 0.0001;

const float CENTRAL_ATRACTION_FORCE  = 0.1;
const float REPULSIVE_SLOPE = 50000.0;

const float DIAMETER_PS = 1.0; // Diameter of polystyrene spheres 1 micron
const float DIAMETER_NIPAM = 0.08; // Diameter of polyNIPAM microgel particles 80 nanometers

const float MASS = 1.0; //estimate with density 1.05g per cm cubed

const float START_RADIUS_OF_INVIRONMENT = 5.0;
const float MAX_INITIAL_VELOCITY = 1.0;
const float START_SEPERATION_TOL = 1.1;

const float NUMBER_OF_STANDARD_DEVIATIONS = 10.0;
 
//globals 
float g_radius_of_invironment;
float g_zero_force_distance;
float g_drag;
float g_Max_brownian_normal_hieght;
float g_brownian_width;
float g_max_attraction;
static int g_win;

void initialize_constants()
{
	float brownian_standard_deviation;
	
	g_max_attraction = 376.5; //should be a function of temperature
	g_drag = 15.25714286; //should be a function of temperature
	g_Max_brownian_normal_hieght = 1.0/(sqrt(4.0*PI)*g_drag*DT); //should be a function of temperature
	brownian_standard_deviation = 1.0; // Need to get proper value for this
	g_brownian_width = NUMBER_OF_STANDARD_DEVIATIONS*brownian_standard_deviation;

	g_zero_force_distance = (REPULSIVE_SLOPE*DIAMETER_PS - g_max_attraction)/REPULSIVE_SLOPE;
	printf("\nThe Zero force distance is %f\n",g_zero_force_distance);
}

void set_initail_conditions(float4 *pos, float3 *vel)
{
	int i,j,ok_config;
	float mag, angle1, angle2, rad, dx, dy, dz;

	g_radius_of_invironment = START_RADIUS_OF_INVIRONMENT + 1.0;
	
	ok_config = NO;
	while(ok_config == NO)
	{
		for(i = 0; i < NUMBER_OF_BODIES; i++)
		{
			rad = START_RADIUS_OF_INVIRONMENT*(float)rand()/RAND_MAX;
			angle1 = PI*(float)rand()/RAND_MAX;
			angle2 = 2.0*PI*(float)rand()/RAND_MAX;
			pos[i].x = rad*sinf(angle1)*cosf(angle2);
			pos[i].y = rad*sinf(angle1)*sinf(angle2);
			pos[i].z = rad*cosf(angle1);
		}
			
		ok_config = YES;
		for(i = 0; i < NUMBER_OF_BODIES - 1; i++)
		{
			for(j = i + 1; j < NUMBER_OF_BODIES; j++)
			{
				dx = pos[i].x-pos[j].x;
				dy = pos[i].y-pos[j].y;
				dz = pos[i].z-pos[j].z;
				if(sqrt(dx*dx + dy*dy + dz*dz) <= START_SEPERATION_TOL) ok_config = NO;
			}
		}
	}

	for(i = 0; i < NUMBER_OF_BODIES; i++)
	{
		rad = MAX_INITIAL_VELOCITY*(float)rand()/RAND_MAX;
		angle1 = PI*(float)rand()/RAND_MAX;
		angle2 = 2.0*PI*(float)rand()/RAND_MAX;
		vel[i].x = rad*sinf(angle1)*cosf(angle2);
		vel[i].y = rad*sinf(angle1)*sinf(angle2);
		vel[i].z = rad*cosf(angle1);
	}
		
	for(i = 0; i < NUMBER_OF_BODIES; i++)
	{
		pos[i].w = MASS;		
	}		
}

void reset_center_of_mass(float4 *pos)
{
	int i;
	float x_center, y_center, z_center, total_mass;

	x_center = 0.0;
	y_center = 0.0;
	z_center = 0.0;
	total_mass = 0.0;

	for(i = 0; i < NUMBER_OF_BODIES; i++)
	{
		x_center += pos[i].x*pos[i].w;
		y_center += pos[i].y*pos[i].w;
		z_center += pos[i].z*pos[i].w;
		total_mass += pos[i].w;
	}

	x_center /= total_mass;
	y_center /= total_mass;
	z_center /= total_mass;

	for(i = 0; i < NUMBER_OF_BODIES; i++)
	{
		pos[i].x -= x_center;
		pos[i].y -= y_center;
		pos[i].z -= z_center;
	}
}

void brownian_motion(float3 *force)
{
	int i,under_normal_curve;
	float mag, angle1, angle2;
	float x,y,normal_hieght,temp;
	
	temp = 4.0*g_drag*DT;
	under_normal_curve = NO;
	
	while(under_normal_curve == NO)
	{
		x = 2.0*g_brownian_width*(float)rand()/RAND_MAX - g_brownian_width;
		y = g_Max_brownian_normal_hieght*(float)rand()/RAND_MAX;
		normal_hieght = g_Max_brownian_normal_hieght*exp(-x*x/temp);
		if(y <= normal_hieght)
		{
			mag = x;
			under_normal_curve = YES;
		}
	}	
	
	for(i = 0; i < NUMBER_OF_BODIES; i++)
	{
		angle1 = PI*(float)rand()/RAND_MAX;
		angle2 = 2.0*PI*(float)rand()/RAND_MAX;
		force[i].x += mag*sinf(angle1)*cosf(angle2);
		force[i].y += mag*sinf(angle1)*sinf(angle2);
		force[i].z += mag*cosf(angle1);
	}
}

void draw_picture(float4 *pos)
{
	int i,j;
	float dx,dy,dz;
	
	reset_center_of_mass(pos);
	glClear(GL_COLOR_BUFFER_BIT);
	glClear(GL_DEPTH_BUFFER_BIT);
	glColor3d(0.0,0.5,1.0);
	for(i = 0; i < NUMBER_OF_BODIES ; i++)
	{
		glPushMatrix();
		glTranslatef(pos[i].x, pos[i].y, pos[i].z);
		glutSolidSphere(0.5,20,20);
		glPopMatrix();
	}
	
	glColor3f (1.0,1.0,1.0);
	for(i = 0; i < NUMBER_OF_BODIES - 1; i++)
	{
		for(j = i + 1; j < NUMBER_OF_BODIES; j++)
		{
			dx = pos[j].x - pos[i].x;
			dy = pos[j].y - pos[i].y;
			dz = pos[j].z - pos[i].z;
			
			if(sqrt(dx*dx + dy*dy + dz*dz) < DIAMETER_PS + DIAMETER_NIPAM)
			{
				glBegin(GL_LINES);
					glVertex3f(pos[i].x, pos[i].y, pos[i].z);
					glVertex3f(pos[j].x, pos[j].y, pos[j].z);		
				glEnd();
			}
		}
	}
	glutSwapBuffers();
}

void get_forces(float4 *pos, float3 *force)
{
	int i,j;
	float dx,dy,dz,r,r2,total_force,d;
	
	for(i = 0; i < NUMBER_OF_BODIES - 1; i++)
	{
		for(j = i + 1; j < NUMBER_OF_BODIES; j++)
		{
			dx = pos[j].x - pos[i].x;
			dy = pos[j].y - pos[i].y;
			dz = pos[j].z - pos[i].z;
				
			r2 = dx*dx + dy*dy + dz*dz;
			r = sqrt(r2);
			
			if(r < DIAMETER_PS)
			{
				total_force =  REPULSIVE_SLOPE*r - REPULSIVE_SLOPE*DIAMETER_PS + g_max_attraction;
			}
			else if (r < DIAMETER_PS + DIAMETER_NIPAM)
			{
				total_force =  -(g_max_attraction/DIAMETER_NIPAM)*r + (g_max_attraction/DIAMETER_NIPAM)*(DIAMETER_PS + DIAMETER_NIPAM);
			}
			else total_force = 0.0;

			force[i].x += total_force*dx/r;
			force[i].y += total_force*dy/r;
			force[i].z += total_force*dz/r;
			force[j].x -= total_force*dx/r;
			force[j].y -= total_force*dy/r;
			force[j].z -= total_force*dz/r;
		}
	}
	
	for(i = 0; i < NUMBER_OF_BODIES; i++)
	{
		d = sqrt(pos[i].x*pos[i].x + pos[i].y*pos[i].y + pos[i].z*pos[i].z);
		force[i].x += -CENTRAL_ATRACTION_FORCE*g_max_attraction*pos[i].x/d;
		force[i].y += -CENTRAL_ATRACTION_FORCE*g_max_attraction*pos[i].y/d;
		force[i].z += -CENTRAL_ATRACTION_FORCE*g_max_attraction*pos[i].z/d;
	}
}

void update_positions_and_velocities(float4 *pos, float3 *vel, float3 *force)
{
	int i;
	float r,temp;
	
	for(i = 0; i < NUMBER_OF_BODIES; i++)
	{
		vel[i].x += DT*(force[i].x - g_drag*vel[i].x)/pos[i].w;
		vel[i].y += DT*(force[i].y - g_drag*vel[i].y)/pos[i].w;
		vel[i].z += DT*(force[i].z - g_drag*vel[i].z)/pos[i].w;

		pos[i].x += DT*vel[i].x;
		pos[i].y += DT*vel[i].y;
		pos[i].z += DT*vel[i].z;
	}
}

void nbody()
{
	int i,j,tdraw, run;
	float dx, dy, dz, time, total_body_to_body_distance;
	float4 *pos;
	float3 *vel, *force;
	
	pos = (float4*)malloc(NUMBER_OF_BODIES*sizeof(float4));
	vel = (float3*)malloc(NUMBER_OF_BODIES*sizeof(float3));
	force = (float3*)malloc(NUMBER_OF_BODIES*sizeof(float3));
	
	initialize_constants();
	
	for(run = 0; run < NUMBER_OF_RUNS; run++)
	{
		tdraw = 0;
		time = 0.0;
		set_initail_conditions(pos, vel);
		while(time < STOP_TIME)
		{
			for(i = 0; i < NUMBER_OF_BODIES; i++)
			{
				force[i].x = 0.0; force[i].y = 0.0; force[i].z = 0.0;
			}
		
			get_forces(pos, force);
			
			brownian_motion(force);
		
			update_positions_and_velocities(pos, vel, force);
			
			time += DT;
			tdraw++;
			if(tdraw == ITERATIONS_BETWEEN_VIEWS)
			{
				//reset_center_of_mass(pos);
				draw_picture(pos);
				tdraw = 0;
				//printf("Time = %f\n", time);
			}
		}

		total_body_to_body_distance = 0.0;
		for(i = 0; i < NUMBER_OF_BODIES - 1; i++)
		{
			for(j = i + 1; j < NUMBER_OF_BODIES; j++)
			{
				dx = pos[j].x - pos[i].x;
				dy = pos[j].y - pos[i].y;
				dz = pos[j].z - pos[i].z;
				
				total_body_to_body_distance += sqrt(dx*dx + dy*dy + dz*dz);
			}
		}
		
		printf("%d %f\n",run,total_body_to_body_distance);
	}
	printf("DONE \n");
}

void KeyPressed(unsigned char key, int x, int y)
{
	if(key == 'q')
	{
		glutDestroyWindow(g_win);
		exit(0);
	}
}

void Display(void)
{
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glFrustum(-0.2, 0.2, -0.2, 0.2, 0.2, 50.0);
	glMatrixMode(GL_MODELVIEW);
	//glutSwapBuffers();
	srand( (unsigned int)time(NULL));
	nbody();
}

int main(int argc, char** argv)
{
	glutInit(&argc,argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_DEPTH | GLUT_RGB);
	glutInitWindowSize(XWindowSize,YWindowSize);
	glutInitWindowPosition(0,0);
	g_win = glutCreateWindow("Self-Assembly");
	glutKeyboardFunc(KeyPressed);
	glutDisplayFunc(Display);
	
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
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glClear(GL_COLOR_BUFFER_BIT);
	glClear(GL_DEPTH_BUFFER_BIT);
	gluLookAt(3.0,3.0, 3.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0);
	//glFrustum(-0.2, 0.2, -0.2, 0.2, 0.2, 50.0);

	glutMainLoop();
	return 0;
}







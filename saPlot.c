//gcc saPlot.c -o plot -lglut -lm -lGLU -lpthread -lGL
#include <GL/glut.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/syscall.h>
#include <pthread.h>

#define PI 3.14159265359 

#define X_WINDOW 1000
#define Y_WINDOW 700

#define X_MAX 100.0
#define X_MIN 0.0
#define X_SCALE 10.0

#define Y_MAX 20.0
#define Y_MIN 0.0
#define Y_SCALE 1.0

// globals
double x_world, y_world;
double present_x, present_y;
int g_mouse_down_count = 0;
float g_lower_tol, g_upper_tol;
int g_totalruns;
int g_win;
int g_selected_run = -1;
pthread_t  thread;

FILE *g_data_file_in;

/*	Takes machine x and y which start in the upper left corner and go from zero to X_WINDOW
	left to right and form zero to Y_WINDOW top to bottom and transslates this into screen 
	points which are a -1 to 1, -1 to 1 window.
*/
double x_machine_to_x_screen(int x)
{
	return( (2.0*x)/X_WINDOW-1.0 );
}

double y_machine_to_y_screen(int y)
{
	return( -(2.0*y)/Y_WINDOW+1.0 );
}

/*	Takes machine x and y which start in the upper left corner and go from zero to X_WINDOW
	left to right and form zero to Y_WINDOW top to bottom and transslates this into world 
	points which are a X_MIN to X_MAX, Y_MIN to Y_MAX window.
*/
double x_machine_to_x_world(int x)
{
	double range;
	range = X_MAX - X_MIN;
	return( (range/X_WINDOW)*x + X_MIN );
}

double y_machine_to_y_world(int y)
{
	double range;
	range = Y_MAX - Y_MIN;
	return(-((range/Y_WINDOW)*y - Y_MAX));
}

/*	Take world  points to screen points 
*/
double x_world_to_x_screen(double x)
{
	double range;
	range = X_MAX - X_MIN;
	return( -1.0 + 2.0*(x - X_MIN)/range );
}

double y_world_to_y_screen(double y)
{
	double range;
	range = Y_MAX - Y_MIN;
	return( -1.0 + 2.0*(y - Y_MIN)/range );
}

void place_axis()
{
	glColor3f(1.0,0.0,1.0);

	glBegin(GL_LINE_LOOP);
		glVertex2f(x_world_to_x_screen(X_MIN),y_world_to_y_screen(0.0));
		glVertex2f(x_world_to_x_screen(X_MAX),y_world_to_y_screen(0.0));
	glEnd();

	glColor3f(1.0,0.0,0.0);
	glBegin(GL_LINE_LOOP);
		glVertex2f(x_world_to_x_screen(0.0),y_world_to_y_screen(Y_MIN));
		glVertex2f(x_world_to_x_screen(0.0),y_world_to_y_screen(Y_MAX));
	glEnd();
}

void place_hash_marks()
{
	double x,y,dx,dy;

	glColor3f(1.0,1.0,1.0);

	dx = X_SCALE;
	dy = Y_SCALE;

	x = X_MIN;
	while(x <= X_MAX)
	{
		glBegin(GL_LINE_LOOP);
			glVertex2f(x_world_to_x_screen(x), 0.005+y_world_to_y_screen(0));
			glVertex2f(x_world_to_x_screen(x),-0.005+y_world_to_y_screen(0));
		glEnd();

		x = x + dx;
	}

	y = Y_MIN;
	while(y <= Y_MAX)
	{
		glBegin(GL_LINE_LOOP);
			glVertex2f( 0.005+x_world_to_x_screen(0),y_world_to_y_screen(y));
			glVertex2f(-0.005+x_world_to_x_screen(0),y_world_to_y_screen(y));
		glEnd();

		y = y + dy;
	}
}
	
void graph()
{
	int i;
	double range,dx;
	int run;
	double temp;
	
	fseek(g_data_file_in, 0, SEEK_SET);
	
	fscanf(g_data_file_in,"%d", &g_totalruns);
	
	place_axis();

	place_hash_marks();

	range = X_MAX - X_MIN;
	dx = range/g_totalruns;

	glPointSize(3.0);

	for(i=0;i<g_totalruns;i++)
	{
		fscanf(g_data_file_in,"%d %lf", &run, &temp);
		if(run == g_selected_run) glColor3f(1.0,0.0,0.0);
		else glColor3f(1.0,1.0,0.0);
				
				
		glBegin(GL_POINTS);
			glVertex2f(x_world_to_x_screen(run*dx),y_world_to_y_screen(temp));
		glEnd();
	}
}

void count_hits()
{
	int i,run, hits;
	float temp;
	
	fseek(g_data_file_in, 0, SEEK_SET);
	
	fscanf(g_data_file_in,"%d", &g_totalruns);
	
	hits = 0;
	for(i=0;i<g_totalruns;i++)
	{
		fscanf(g_data_file_in,"%d %f", &run, &temp);
		if(g_lower_tol <= temp && temp <= g_upper_tol) hits++; 
	}
	
	printf("\nUpper Tolerance = %lf\n",g_upper_tol);
	printf("lower Tolerance = %lf\n",g_lower_tol);
	printf("Total Run  = %d\n",g_totalruns);
	printf("Total HITS = %d\n",hits);
	printf("Ratio      = %lf\n",(float)hits/(float)g_totalruns);
}

void* spawn_view(void* arg)
{
	char buffer[33];
	int *run = ((int*)(arg));

	sprintf(buffer, "./view %d",*run);
	system(buffer);
}

void* spawn_run(void* arg)
{
	char buffer[33];
	int *run = ((int*)(arg));

	sprintf(buffer, "./watch %d",*run);
	system(buffer);
}

void mymouse(int button, int state, int x, int y)
{	
	float temp, x_run, range, dx;
	int i,run;
	int *selected_run = (int*)malloc(sizeof(int));

	if(state == GLUT_DOWN)
	{
		if(button == GLUT_LEFT_BUTTON)
		{
			if(g_mouse_down_count == 0)
			{
				glColor3f(1.0,0.0,0.0);
				g_lower_tol = y_machine_to_y_world(y);
				glBegin(GL_LINE_LOOP);
					glVertex2f(x_world_to_x_screen(X_MIN),y_machine_to_y_screen(y));
					glVertex2f(x_world_to_x_screen(X_MAX),y_machine_to_y_screen(y));
				glEnd();
				g_mouse_down_count++;
			}
			else if(g_mouse_down_count == 1)
			{
				glColor3f(1.0,0.0,0.0);
				g_upper_tol = y_machine_to_y_world(y);
				glBegin(GL_LINE_LOOP);
					glVertex2f(x_world_to_x_screen(X_MIN),y_machine_to_y_screen(y));
					glVertex2f(x_world_to_x_screen(X_MAX),y_machine_to_y_screen(y));
				glEnd();
				g_mouse_down_count++;
				
				if(g_upper_tol < g_lower_tol) 
				{
					temp = g_upper_tol;
					g_upper_tol = g_lower_tol;
					g_lower_tol = temp;
				}
				count_hits();
			}
			else
			{
				glClear(GL_COLOR_BUFFER_BIT);
				place_axis();
				place_hash_marks();
				graph();
				glColor3f(1.0,0.0,0.0);
				g_lower_tol = y_machine_to_y_world(y);
				glBegin(GL_LINE_LOOP);
					glVertex2f(x_world_to_x_screen(X_MIN),y_machine_to_y_screen(y));
					glVertex2f(x_world_to_x_screen(X_MAX),y_machine_to_y_screen(y));
				glEnd();

				g_mouse_down_count = 1;
			}
			glutSwapBuffers();
		}
		else if(button == GLUT_RIGHT_BUTTON)
		{
			g_mouse_down_count = 0;
			x_run = x_machine_to_x_world(x);
			range = X_MAX - X_MIN;
			dx = range/g_totalruns;
			g_selected_run = (int)((x_run/range)*(float)g_totalruns);
			*selected_run = g_selected_run;
			printf("\nSelected Run = %d Total Body to Body Distance =%f\n",run, temp);
			glClear(GL_COLOR_BUFFER_BIT);
			place_axis();
			place_hash_marks();
			graph();
			glutSwapBuffers();
			pthread_create(&thread, NULL, spawn_view, (void*)selected_run);
		}
	}
}

void display()
{
	glClear(GL_COLOR_BUFFER_BIT);
	place_axis();
	place_hash_marks();
	graph();
	glutSwapBuffers();
}

void KeyPressed(unsigned char key, int x, int y)
{
	printf("in KeyPressed\n");
	int *selected_run = (int*)malloc(sizeof(int));
	*selected_run = g_selected_run;
	if(key == 'q')
	{
		glutDestroyWindow(g_win);
		exit(0);
	}
	if(key == 'r')
	{
		*selected_run = g_selected_run;
		pthread_create(&thread, NULL, spawn_run, (void*)selected_run);
	}
	if(key == 'v')
	{
		*selected_run = g_selected_run;
		pthread_create(&thread, NULL, spawn_view, (void*)selected_run);
	}
}

int main(int argc, char** argv)
{
	g_data_file_in = fopen("total_body_to_body_distance_out","r");
	if(g_data_file_in == NULL) printf("error opening distance file\n");
	
	glutInit(&argc,argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_DEPTH | GLUT_RGB);
	glutInitWindowSize(X_WINDOW,Y_WINDOW);
	glutInitWindowPosition(0,0);
	g_win = glutCreateWindow("Self Assembly");
	
	glutDisplayFunc(display);
	glutKeyboardFunc(KeyPressed);
	glutMouseFunc(mymouse);

	glClear(GL_COLOR_BUFFER_BIT);
	glClear(GL_DEPTH_BUFFER_BIT);
	
	printf("\nPress q to quit\n");
	glutMainLoop();
}



#include "texLoad.h"

/* wave structure with appropriate parameters for Gerstner Waves */
struct wave {
	double dx,dy;	//  x and y components of the wave's directional vector
	double qi;		//  steepness factor
	double l;		//	wavelength
	double a;		//  amplitude
	double w;		//	frequency
	double s;		//	speed the crest is moving forward in m/s
	double p_const;	//	phase constant used in Gerstner Wave calculations
};

/* Globals */
int mode=1;       //  Projection mode
int mesh=0;		  //  Display water as a quad mesh
int day=1;		  //  daytime(1) vs nighttime(0)
int fov=55;       //  Field of view (for perspective)
double dim=100.0;   //  Size of world

double Ex;
double Ey;
double Ez;

double fx=0.0;
double fy=5.5;		//  eye position for first person mode
double fz=0.0;

double lx=0.0;
double ly=5.5;		//  position we are looking at
double lz=100.0;

const char *daysides[6] = {"textures/sky_right.bmp","textures/sky_left.bmp","textures/sky_top.bmp","textures/sky_bottom.bmp","textures/sky_back.bmp","textures/sky_front.bmp"};
const char *nightsides[6] = {"textures/nightsky_right.bmp","textures/nightsky_left.bmp","textures/nightsky_top.bmp","textures/nightsky_bot.bmp","textures/nightsky_back.bmp","textures/nightsky_front.bmp"};
unsigned int texture[7];  //  Texture names
unsigned int shader[2];	  //  Shaders

int th=0;
int ph=0;
double asp=1.0;					// aspect ratio for viewport
double q=.1;					// steepness factor for waves
double g=9.8;					// gravity 9.8 m/s^2
struct wave waves[8];			// array of wave structs used in animation
double xmap[200][200];				// array to hold adjusted x coordinates for gerstner waves
double ymap[200][200];				// array to hold adjusted y coordinates for gerstner waves
double zmap[200][200];				// array to hold height mapping for x,y gerstner wave coordinates
double xnorm[200][200];				// array to hold x compponent of normal vectors
double ynorm[200][200];				// array to hold y compponent of normal vectors
double znorm[200][200];				// array to hold z compponent of normal vectors
double qstep=2;					//	units between subsequently drawn quads on mesh
double t=0;						// elapsed time in seconds


int lzh       =  15;  // Light azimuth
float ylight  =  70;  // Elevation of light
int ambient   =  60;  // Ambient intensity (%)
int diffuse   = 100;  // Diffuse intensity (%)
int specular  =   90;  // Specular intensity (%)
int distance  = 220;  // Light distance
int local     =   0;  // Local Viewer Model
float shiny   =   1;  // Shininess (value)

/*
 *  Set projection
 */
/* FUNCTION ADAPTED FROM IN CLASS EXAMPLE 9 */
static void Project()
{
   //  Tell OpenGL we want to manipulate the projection matrix
   glMatrixMode(GL_PROJECTION);
   //  Undo previous transformations
   glLoadIdentity();
   //  Overhead Perspective projection
   if (mode==1)
      gluPerspective(fov,asp,dim/4,6*dim);
   //  First person perspective projection
   else if (mode==2)
      gluPerspective(fov,asp,1,dim*4);
   //  Switch to manipulating the model matrix
   glMatrixMode(GL_MODELVIEW);
   //  Undo previous transformations
   glLoadIdentity();
}

/* Utility function for adding a wave to our wave array */
struct wave addWave(double d, double l, double a) {
	double w = sqrt(g*2*PI/l);							//	set frequency of wave dispersion relation for water, ignoring higher-order terms
														//  see https://developer.download.nvidia.com/books/HTML/gpugems/gpugems_ch01.html for additional info
	double qi = q/(w * a * 8);
	double s = l*w;  									//  set speed of the wave according to speed = wavelength * frequency
	double p_const = s*w;								//	set phase constant of the wave
	double dx = Cos(d);									//	x component of wave direction
	double dy = Sin(d);									//	y component of wave direction
	struct wave newWave = {dx,dy,qi,l,a,w,s,p_const};
	return newWave;
} 

static void computeHeights() {
	double dot_term,q_term;
	double rX=0;
	double rY=0;
	double rZ=0;
	double x,y;
	int i,xindex,yindex;
	for (x=-dim;x<dim;x+=qstep) {
		for (y=-dim;y<dim;y+=qstep) {
			for (i=0;i<8;i++) {
				dot_term = waves[i].w*waves[i].dx*x + waves[i].w*waves[i].dy*y + waves[i].p_const*t;
				q_term = waves[i].qi*waves[i].a;
				rX += q_term * waves[i].dx * Cos(dot_term);
				rY += q_term * waves[i].dy * Cos(dot_term);
				rZ += waves[i].a * Sin(dot_term);
			}
			xindex = (x+dim)/qstep;
			yindex = (y+dim)/qstep;
			xmap[xindex][yindex] = x+rX;
			ymap[xindex][yindex] = y+rY;
			zmap[xindex][yindex] = rZ;
			rX = rY = rZ = 0;
		}
	}
}

void computeNorms() {
	double dot_term,w_term;
	double rX=0;
	double rY=0;
	double rZ=0;
	double x,y;
	int i,xindex,yindex;
	for (x=-dim;x<dim;x+=qstep) {
		for (y=-dim;y<dim;y+=qstep) {
			xindex = (x+dim)/qstep;
			yindex = (y+dim)/qstep;
			for (i=0;i<8;i++) {
				w_term = waves[i].w*waves[i].a;
				dot_term = waves[i].w*(waves[i].dx*xmap[xindex][yindex] + waves[i].dy*ymap[xindex][yindex]) + waves[i].p_const*t;
				rX += waves[i].dx * w_term * Cos(dot_term);
				rY += waves[i].dy * w_term * Cos(dot_term);
				rZ += waves[i].qi * w_term * Sin(dot_term);
			}
			xnorm[xindex][yindex] = -rX;
			ynorm[xindex][yindex]  = -rY;
			znorm[xindex][yindex]  = 1.0-rZ;
			rX = rY = rZ = 0;
		}
	}
}

/*
 *  Draw vertex in polar coordinates
 */
/* FUNCTION ADAPTED FROM IN CLASS EXAMPLE 8 */
static void Vertex(double th,double ph)
{
   double x = Sin(th)*Cos(ph);
   double y = Cos(th)*Cos(ph);
   double z =         Sin(ph);
   glNormal3d(x,y,z);
   glVertex3d(x,y,z);
}

/*
 *  Draw a sphere
 *     at (x,y,z)
 *     radius (r)
 */
/* FUNCTION ADAPTED FROM IN CLASS EXAMPLE 8 */
static void sphere(double x,double y,double z,double r)
{
   const int d=5;
   int th,ph;

   //  Save transformation
   glPushMatrix();
   //  Offset and scale
   glTranslated(x,y,z);
   glScaled(r,r,r);

   //  Latitude bands
   for (ph=-90;ph<90;ph+=d)
   {
      glBegin(GL_QUAD_STRIP);
      for (th=0;th<=360;th+=d)
      {
         Vertex(th,ph);
         Vertex(th,ph+d);
      }
      glEnd();
   }

   //  Undo transformations
   glPopMatrix();
}

/* 
 *  Draw sky box
 */
static void Sky(double D)
{
   glColor3f(1,1,1);
   glEnable(GL_TEXTURE_2D);

   //  Sides
   glBindTexture(GL_TEXTURE_2D,day ? texture[0] : texture[2]);

   glBegin(GL_QUADS);
   glTexCoord2f(0.00,1.0/3.0); glVertex3f(-D,-D,-D);
   glTexCoord2f(0.25,1.0/3.0); glVertex3f(+D,-D,-D);
   glTexCoord2f(0.25,2.0/3.0); glVertex3f(+D,+D,-D);
   glTexCoord2f(0.00,2.0/3.0); glVertex3f(-D,+D,-D);

   glTexCoord2f(0.25,1.0/3.0); glVertex3f(+D,-D,-D);
   glTexCoord2f(0.50,1.0/3.0); glVertex3f(+D,-D,+D);
   glTexCoord2f(0.50,2.0/3.0); glVertex3f(+D,+D,+D);
   glTexCoord2f(0.25,2.0/3.0); glVertex3f(+D,+D,-D);

   glTexCoord2f(0.50,1.0/3.0); glVertex3f(+D,-D,+D);
   glTexCoord2f(0.75,1.0/3.0); glVertex3f(-D,-D,+D);
   glTexCoord2f(0.75,2.0/3.0); glVertex3f(-D,+D,+D);
   glTexCoord2f(0.50,2.0/3.0); glVertex3f(+D,+D,+D);

   glTexCoord2f(0.75,1.0/3.0); glVertex3f(-D,-D,+D);
   glTexCoord2f(1.00,1.0/3.0); glVertex3f(-D,-D,-D);
   glTexCoord2f(1.00,2.0/3.0); glVertex3f(-D,+D,-D);
   glTexCoord2f(0.75,2.0/3.0); glVertex3f(-D,+D,+D); 

   //  Top and bottom
   glTexCoord2f(0.25,2.0/3.0); glVertex3f(+D,+D,-D);
   glTexCoord2f(0.5,2.0/3.0); glVertex3f(+D,+D,+D);
   glTexCoord2f(0.5,1); glVertex3f(-D,+D,+D);
   glTexCoord2f(0.25,1); glVertex3f(-D,+D,-D);

   glTexCoord2f(0.5,0); glVertex3f(-D,-D,+D);
   glTexCoord2f(0.5,1.0/3.0); glVertex3f(+D,-D,+D);
   glTexCoord2f(0.25,1.0/3.0); glVertex3f(+D,-D,-D);
   glTexCoord2f(0.25,0); glVertex3f(-D,-D,-D);


   glEnd(); 

   glDisable(GL_TEXTURE_2D);
}

/*-------------------------------------------------------------------------------------------------------
										Shader Program Functions
-------------------------------------------------------------------------------------------------------*/

/*
 *  Read text file
 */
char* ReadText(char *file)
{
   int   n;
   char* buffer;
   //  Open file
   FILE* f = fopen(file,"rt");
   if (!f) Fatal("Cannot open text file %s\n",file);
   //  Seek to end to determine size, then rewind
   fseek(f,0,SEEK_END);
   n = ftell(f);
   rewind(f);
   //  Allocate memory for the whole file
   buffer = (char*)malloc(n+1);
   if (!buffer) Fatal("Cannot allocate %d bytes for text file %s\n",n+1,file);
   //  Snarf the file
   if (fread(buffer,n,1,f)!=1) Fatal("Cannot read %d bytes for text file %s\n",n,file);
   buffer[n] = 0;
   //  Close and return
   fclose(f);
   return buffer;
}

/*
 *  Print Shader Log
 */
void PrintShaderLog(int obj,char* file)
{
   int len=0;
   glGetShaderiv(obj,GL_INFO_LOG_LENGTH,&len);
   if (len>1)
   {
      int n=0;
      char* buffer = (char *)malloc(len);
      if (!buffer) Fatal("Cannot allocate %d bytes of text for shader log\n",len);
      glGetShaderInfoLog(obj,len,&n,buffer);
      fprintf(stderr,"%s:\n%s\n",file,buffer);
      free(buffer);
   }
   glGetShaderiv(obj,GL_COMPILE_STATUS,&len);
   if (!len) Fatal("Error compiling %s\n",file);
}

/*
 *  Print Program Log
 */
void PrintProgramLog(int obj)
{
   int len=0;
   glGetProgramiv(obj,GL_INFO_LOG_LENGTH,&len);
   if (len>1)
   {
      int n=0;
      char* buffer = (char *)malloc(len);
      if (!buffer) Fatal("Cannot allocate %d bytes of text for program log\n",len);
      glGetProgramInfoLog(obj,len,&n,buffer);
      fprintf(stderr,"%s\n",buffer);
   }
   glGetProgramiv(obj,GL_LINK_STATUS,&len);
   if (!len) Fatal("Error linking program\n");
}

/*
 *  Create Shader
 */
int CreateShader(GLenum type,char* file)
{
   //  Create the shader
   int shader = glCreateShader(type);
   //  Load source code from file
   char* source = ReadText(file);
   glShaderSource(shader,1,(const char**)&source,NULL);
   free(source);
   //  Compile the shader
   fprintf(stderr,"Compile %s\n",file);
   glCompileShader(shader);
   //  Check for errors
   PrintShaderLog(shader,file);
   //  Return name
   return shader;
}

/*
 *  Create Shader Program
 */
int CreateShaderProg(char* VertFile,char* FragFile)
{
   //  Create program
   int prog = glCreateProgram();
   //  Create and compile vertex shader
   int vert = CreateShader(GL_VERTEX_SHADER  ,VertFile);
   //  Create and compile fragment shader
   int frag = CreateShader(GL_FRAGMENT_SHADER,FragFile);
   //  Attach vertex shader
   glAttachShader(prog,vert);
   //  Attach fragment shader
   glAttachShader(prog,frag);
   //  Link program
   glLinkProgram(prog);
   //  Check for errors
   PrintProgramLog(prog);
   //  Return name
   return prog;
}


/*-------------------------------------------------------------------------------------------------------
									End ofShader Program Functions
-------------------------------------------------------------------------------------------------------*/


/*
 *  GLUT calls this routine when the window is resized
 */
/* FUNCTION ADAPTED FROM IN CLASS EXAMPLE 9*/
void reshape(int width,int height)
{
   //  Ratio of the width to the height of the window
   asp = (height>0) ? (double)width/height : 1;
   //  Set the viewport to the entire window
   glViewport(0,0, width,height);

}

/* FUNCTION ADAPTED FROM IN CLASS EXAMPLE 1-5 */
void idle()
{
   //  Get elapsed (wall) time in seconds
   t = glutGet(GLUT_ELAPSED_TIME)/1000.0;

   Project();

   //  Request display update
   glutPostRedisplay();
}

/*
 *  GLUT calls this routine when an arrow key is pressed
 */
/* FUNCTION ADAPTED FROM IN CLASS EXAMPLE 9*/
void special(int key,int x,int y)
{

   		//  Look Right
   		if (key == GLUT_KEY_RIGHT) {
   			th -= 5;
      		lx = fx+50.0*Sin(th)*Cos(ph);
      		ly = fy+50.0*Sin(ph);
      		lz = fz+50.0*Cos(th)*Cos(ph);
      	}
   		//  Look Left
   		else if (key == GLUT_KEY_LEFT) {
   			th += 5;
      		lx = fx+50.0*Sin(th)*Cos(ph);
      		ly = fy+50.0*Sin(ph);
      		lz = fz+50.0*Cos(th)*Cos(ph);
      	}
   		//  Look Up
   		else if (key == GLUT_KEY_UP) {
      		ph += 5;
      		lx = fx+50.0*Sin(th)*Cos(ph);
      		ly = fy+50.0*Sin(ph);
      		lz = fz+50.0*Cos(th)*Cos(ph);
   		}
   		//  Look Down
   		else if (key == GLUT_KEY_DOWN){
      		ph -= 5;
      		lx = fx+50.0*Sin(th)*Cos(ph);
      		ly = fy+50.0*Sin(ph);
      		lz = fz+50.0*Cos(th)*Cos(ph);
   		}
   	
   th %= 360;
   ph %= 360;

   //  Update projection
   Project();
   //  Tell GLUT it is necessary to redisplay the scene
   glutPostRedisplay();
}

/*
 *  GLUT calls this routine when a key is pressed
 */
/* FUNCTION ADAPTED FROM IN CLASS EXAMPLE 9*/
void key(unsigned char ch,int x,int y)
{
   //  Exit on ESC
   if (ch == 27)
      	exit(0);
   //  Reset view angle
   else if (ch == '0') {
      	th = ph = 0;
      	fx=0.0;
		fy=5.5;
		fz=0.0;
      	lx=0.0;
		ly=5.5;
		lz=50.0;
	}
   //  Toggle between day and night
   else if (ch == 'd') {
      day = 1-day;
  	  if (day) {
  	  	 ambient   = 60;  
         diffuse   = 100; 
         specular  = 90; 
  	  }
  	  else {
  	  	 ambient   = 50;  
         diffuse   = 50; 
         specular  = 50;	
  	  }
  	}
   //  Toggle between mesh grid and quads
   else if (ch == 'm')
      mesh = 1-mesh;
   //  Switch display mode
   else if (ch == '1')
   		mode = 1;
   else if (ch == '2')
   		mode = 2;
   //  Change field of view angle
   else if (ch == '-' && ch>1)
      	fov--;
   else if (ch == '+' && ch<179)
      	fov++;
   else if (ch == 'w') {
   		fx += Sin(th)*Cos(ph);
   		fy += Sin(ph);
   		fz += Cos(th)*Cos(ph);
   		lx += Sin(th)*Cos(ph);
   		ly += Sin(ph);
   		lz += Cos(th)*Cos(ph);
   }
   else if (ch == 's') {
   		fx -= Sin(th)*Cos(ph);
   		fy -= Sin(ph);
   		fz -= Cos(th)*Cos(ph);
   		lx -= Sin(th)*Cos(ph);
   		ly -= Sin(ph);
   		lz -= Cos(th)*Cos(ph);
   }

   //  Reproject
   Project();
   //  Tell GLUT it is necessary to redisplay the scene
   glutPostRedisplay();
}


void display() {
	//  Clear the image
   	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
   	//  Enable Z-buffering in OpenGL
    glEnable(GL_DEPTH_TEST);
   	//  Reset previous transforms
   	glLoadIdentity();

   	//  Overhead perspective
   	if (mode == 1)
   	{
   		glColor3f(1,1,1);
    	glWindowPos2i(5,5);
    	Print("th=%d ph=%d, mode: Overhead perspective",th,ph);
   		Ex = -2*dim*Sin(th)*Cos(ph);
      	Ey = +2*dim        *Sin(ph);
      	Ez = +2*dim*Cos(th)*Cos(ph);
      	gluLookAt(Ex,Ey,Ez , 0,0,0 , 0,Cos(ph),0);
   	}
   	//  First person perspective
   	else if (mode == 2){
   		glColor3f(1,1,1);
    	glWindowPos2i(5,5);
    	Print("th=%d ph=%d, mode: First Person Perspective",th,ph);
   		gluLookAt(fx,fy,fz,  lx,ly,lz,  0,Cos(ph),0);
   	}

   	glShadeModel(GL_SMOOTH);

   	if (!day) {
   		glPushMatrix();
   		glRotatef(-90,1,0,0);
   		Sky(2*dim);
   		glPopMatrix();
   	}
   	else
   		Sky(2*dim);


  	//  Translate intensity to color vectors
	float Ambient[]   = {0.01*ambient ,0.01*ambient ,0.01*ambient ,1.0};
	float Diffuse[]   = {0.01*diffuse ,0.01*diffuse ,0.01*diffuse ,1.0};
	float Specular[]  = {0.01*specular,0.01*specular,0.01*specular,1.0};
	float Emission[]  = {0.0,0.0,0.0,1.0};
	float Shinyness[] = {16};
	//  Light position
	float Position[]  = {distance*Cos(lzh),ylight,distance*Sin(lzh),1.0};
	//  Draw light position as ball (still no lighting here)
	glColor3f(1,1,1);
	sphere(Position[0],Position[1],Position[2] , 5);
	//  OpenGL should normalize normal vectors
	glEnable(GL_NORMALIZE);
	//  Enable lighting
	glEnable(GL_LIGHTING);
	//  Location of viewer for specular calculations
	glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER,local);
	//  glColor sets ambient and diffuse color materials
	glColorMaterial(GL_FRONT_AND_BACK,GL_AMBIENT_AND_DIFFUSE);
	glEnable(GL_COLOR_MATERIAL);
	//  Enable light 0
	glEnable(GL_LIGHT0);
	//  Set ambient, diffuse, specular components and position of light 0
	glLightfv(GL_LIGHT0,GL_AMBIENT ,Ambient);
	glLightfv(GL_LIGHT0,GL_DIFFUSE ,Diffuse);
	glLightfv(GL_LIGHT0,GL_SPECULAR,Specular);
	glLightfv(GL_LIGHT0,GL_POSITION,Position);

	
	glRotatef(-90,1,0,0);		// orient z axis upward 


	//  Set materials
   glMaterialfv(GL_FRONT_AND_BACK,GL_SHININESS,Shinyness);
   glMaterialfv(GL_FRONT_AND_BACK,GL_SPECULAR,Specular);
   glMaterialfv(GL_FRONT_AND_BACK,GL_EMISSION,Emission);

  	glUseProgram(shader[1]);

   	computeHeights();
   	computeNorms();

   	glEnable(GL_TEXTURE_CUBE_MAP);
   	glActiveTexture(GL_TEXTURE0);
   	glBindTexture(GL_TEXTURE_CUBE_MAP,day ? texture[1] : texture[3]);

   	int x,y;
   	for (x=0;x<(2*dim)/qstep-1;x++) {
   		for (y=0;y<(2*dim)/qstep-1;y++) {
   			glBegin(mesh ? GL_LINE_STRIP : GL_QUAD_STRIP);
   			glColor3f(0,0,1);
 
   			glNormal3f(xnorm[x][y], ynorm[x][y], znorm[x][y]);
   			glVertex3d(xmap[x][y], ymap[x][y], zmap[x][y]);

   			glNormal3f(xnorm[x][y+1], ynorm[x][y+1], znorm[x][y+1]);
   			glVertex3d(xmap[x][y+1], ymap[x][y+1], zmap[x][y+1]);

   			glNormal3f(xnorm[x+1][y], ynorm[x+1][y], znorm[x+1][y]);
   			glVertex3d(xmap[x+1][y], ymap[x+1][y], zmap[x+1][y]);

   			glNormal3f(xnorm[x+1][y+1], ynorm[x+1][y+1], znorm[x+1][y+1]);
   			glVertex3d(xmap[x+1][y+1], ymap[x+1][y+1], zmap[x+1][y+1]);

   			glEnd();
   		}
   	} 

  	glUseProgram(0);
  	glDisable(GL_TEXTURE_CUBE_MAP);
   	glDisable(GL_LIGHTING);


   	glFlush();
   	glutSwapBuffers();


}

int main(int argc,char* argv[]) {
  	//  Inittialize GLUT
	glutInit(&argc,argv);
  	//  Request double buffered, true color window with Z buffering
   	glutInitDisplayMode(GLUT_RGB | GLUT_DEPTH | GLUT_DOUBLE);
	//  Create the window
	glutInitWindowSize(500,500);
	glutCreateWindow("Final Project - Water Surface Simulation: Alex Thompson");

  	glutDisplayFunc(display);
  	glutReshapeFunc(reshape);
  	glutSpecialFunc(special);
  	glutKeyboardFunc(key);
  	glutIdleFunc(idle);

 	waves[0] = addWave(232,.2,.3);
	waves[1] = addWave(106,.1,.2);
	waves[2] = addWave(16,.3,.1);
	waves[3] = addWave(338,.1,.3);
	waves[4] = addWave(56,.0005,.01);
	waves[5] = addWave(176,.001,.02);
	waves[6] = addWave(89,.002,.03);
	waves[7] = addWave(202,.004,.04);

	texture[0] = LoadTexBMP("textures/sky_cube.bmp");	
  	texture[1] = LoadCubeTexBMP(daysides);
  	texture[2] = LoadTexBMP("textures/nightsky.bmp");
  	texture[3] = LoadCubeTexBMP(nightsides);
  	shader[1] = CreateShaderProg("pixlight.vert","pixlight.frag");

  	//  Pass control to GLUT so it can interact with the user
  	glutMainLoop();
  	//  Return code
  	return 0;
}
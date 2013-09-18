#include "SDL.h"
#include "SDL_opengl.h"
#include "SDL_image.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define WIDTH  640
#define HEIGHT 480

static const char * IMAGE_FILE = "example.png";

GLfloat yaw;
GLfloat pitch;
int level;

static void
subdivide (GLfloat point0[3], GLfloat point1[3], GLfloat point2[3], int level)
{
  int coord;
  GLfloat midpoint[3][3];
  
  /* Don't subdivide any further; just draw the triangle */
  if (level==0)
  {
    glColor3fv (point0);
    glVertex3fv (point0);
    glColor3fv (point1);
    glVertex3fv (point1);
    glColor3fv (point2);
    glVertex3fv (point2);
    return;
  }

  /* Calculate a midpoint on each edge of the triangle */
  for (coord = 0; coord<3; coord++)
  {
    midpoint[0][coord] = (point0[coord] + point1[coord])*0.5;
    midpoint[1][coord] = (point1[coord] + point2[coord])*0.5;
    midpoint[2][coord] = (point2[coord] + point0[coord])*0.5;
  }

  /* Subdivide each triangle into three more */  /*   .    */
  level--;                     /*  /X\   */
  subdivide (point0,midpoint[0],midpoint[2],level); /*   /xxx\  */
  subdivide (point1,midpoint[1],midpoint[0],level); /*  /X\ /X\   */
  subdivide (point2,midpoint[2],midpoint[1],level); /* /XXXVXXX\  */
}

static void
paint_triangles ()
{
  int i;

  /* Coordinates of the 6 vertices of the octahedron */
  static GLfloat point[6][3] = { 
    {1.0f,0.0f,0.0f},{-1.0f,0.0f,0.0f},
    {0.0f,1.0f,0.0f},{0.0f,-1.0f,0.0f},
    {0.0f,0.0f,1.0f},{0.0f,0.0f,-1.0f}
  };

  /* indices of the vertices of the triangles which make up each of 
   * the 8 faces of the octahedron */
  static int triangle[8][3] = { 
    {2,4,0},{2,0,5},{2,5,1},{2,1,4},{3,0,4},{3,5,0},{3,1,5},{3,4,1}
  };
   
  /* Rotate the object */
  glRotatef (pitch, 1.0f, 0.0f, 0.0f);
  glRotatef (yaw, 0.0f, 1.0f, 0.0f);

  /* Draw the triangles which make up the object */
  glBegin (GL_TRIANGLES);

  for (i=0; i<8; i++)
  {
    subdivide (point[triangle[i][0]],point[triangle[i][1]],point[triangle[i][2]],level);
  }
  
  glEnd ();

  /* increment the rotation every frame */
  yaw = yaw + 0.05;
}

static void
paint_begin ()
{
  /* Clear the color plane and the z-buffer */
  glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glLoadIdentity ();

  /* Move the object 2 units away from the camera */
  glTranslatef (0.0f, 0.0f, -2.0f);
}


static void
paint_end ()
{
  /* finally, swap the back and front buffers */
  SDL_GL_SwapBuffers ();
}

static void
setup_sdl () 
{
  const SDL_VideoInfo* video;
  int img_flags;
  int img_initted;

  if (SDL_Init (SDL_INIT_VIDEO) < 0 )
  {
    fprintf (stderr, "Couldn't initialize SDL: %s\n", SDL_GetError ());
    exit (1);
  }
    
  /* Quit SDL properly on exit */
  atexit (SDL_Quit);

  /* Get the current video information */
  video = SDL_GetVideoInfo ( );
  if ( video == NULL )
  {
    fprintf (stderr, "Couldn't get video information: %s\n", SDL_GetError ());
    exit (1);
  }

  /* Set the minimum requirements for the OpenGL window */
  SDL_GL_SetAttribute ( SDL_GL_RED_SIZE, 5 );
  SDL_GL_SetAttribute ( SDL_GL_GREEN_SIZE, 5 );
  SDL_GL_SetAttribute ( SDL_GL_BLUE_SIZE, 5 );
  SDL_GL_SetAttribute ( SDL_GL_DEPTH_SIZE, 16 );
  SDL_GL_SetAttribute ( SDL_GL_DOUBLEBUFFER, 1 );

  /* Note the SDL_DOUBLEBUF flag is not required to enable double 
   * buffering when setting an OpenGL video mode. 
   * Double buffering is enabled or disabled using the 
   * SDL_GL_DOUBLEBUFFER attribute.
   */
  if ( SDL_SetVideoMode ( WIDTH, HEIGHT, video->vfmt->BitsPerPixel, SDL_OPENGL ) == 0 )
  {
    fprintf (stderr, "Couldn't set video mode: %s\n", SDL_GetError ());
    exit (1);
  }

  img_flags = IMG_INIT_JPG | IMG_INIT_PNG;
  img_initted = IMG_Init (img_flags);
  if (img_initted & img_flags != img_flags)
  {
    printf ("IMG_Init: Failed to init required jpg and png support!\n");
    printf ("IMG_Init: %s\n", IMG_GetError ());
    exit (1);
  }
}

static void
setup_opengl ()
{
  float aspect = (float)WIDTH / (float)HEIGHT;

  /* Make the viewport cover the whole window */
  glViewport (0, 0, WIDTH, HEIGHT);

  /* Set the camera projection matrix:
   * field of view: 90 degrees
   * near clipping plane at 0.1
   * far clipping plane at 100.0
   */
  glMatrixMode (GL_PROJECTION);
  glLoadIdentity ();
  
  gluPerspective (60.0, aspect, 0.1, 100.0);
  /* We're done with the camera, now matrix operations 
   * will affect the modelview matrix
   * */
  glMatrixMode (GL_MODELVIEW);

  /* set the clear color to gray */
  glClearColor (0.5, 0.5 ,0.5, 0);
  
  /* We want z-buffer tests enabled*/
  glEnable (GL_DEPTH_TEST);

  /* Do draw back-facing polygons*/
  glDisable (GL_CULL_FACE);
}

static void
EnableTransparency ()
{
  glEnable (GL_BLEND);
  glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

static GLuint
LoadTexture(const char *filename, int *textw, int *texth)
{
  SDL_Surface *surface;
  GLuint textureid;
  int mode;

  surface = IMG_Load (filename);

  // Or if you don't use SDL_image you can use SDL_LoadBMP here instead:
  // surface = SDL_LoadBMP(filename);

  // could not load filename
  if (! surface)
  {
    return 0;
  }

  // work out what format to tell glTexImage2D to use...
  if (surface->format->BytesPerPixel == 3)
  { // RGB 24bit
    mode = GL_RGB;
  }
  else if (surface->format->BytesPerPixel == 4)
  { // RGBA 32bit
    mode = GL_RGBA;
  }
  else
  {
    SDL_FreeSurface (surface);
    return 0;
  }

  *textw = surface->w;
  *texth = surface->h;
  // create one texture name
  glGenTextures (1, &textureid);

  // tell opengl to use the generated texture name
  glBindTexture (GL_TEXTURE_2D, textureid);

  // this reads from the sdl surface and puts it into an opengl texture
  glTexImage2D (GL_TEXTURE_2D, 0, mode, surface->w, surface->h, 0, mode, GL_UNSIGNED_BYTE, surface->pixels);

  // these affect how this texture is drawn later on...
  glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  // clean up
  SDL_FreeSurface (surface);

  printf ("Loaded image %s %dx%d\n", filename, *textw, *texth);

  return textureid;
}

static void
DrawTexture(int x, int y, GLuint textureid, int textw, int texth)
{
  // tell opengl to use the generated texture name
  glBindTexture (GL_TEXTURE_2D, textureid);
  glEnable (GL_TEXTURE_2D);

  // make a rectangle
  glBegin (GL_QUADS);

  // top left
  glTexCoord2i (0, 0);
  glVertex3f (x, y, 0);

  // top right
  glTexCoord2i (1, 0);
  glVertex3f (x + textw, y, 0);

  // bottom right
  glTexCoord2i (1, 1);
  glVertex3f (x + textw, y + texth, 0);

  // bottom left
  glTexCoord2i (0, 1);
  glVertex3f (x, y + texth, 0);

  glEnd ();
  
  glDisable (GL_TEXTURE_2D );
}

static void
main_loop () 
{
  SDL_Event event;

  GLuint texture;
  int textw, texth;

  printf ("Loading texture %s\n", IMAGE_FILE);
  texture = LoadTexture (IMAGE_FILE, &textw, &texth);

  while (1)
  {
    /* process pending events */
    while (SDL_PollEvent (&event))
    {
      switch (event.type)
      {
      case SDL_KEYDOWN:
        switch (event.key.keysym.sym)
        {
        case SDLK_ESCAPE:
          exit (0);
          break;

        case SDLK_KP_PLUS:
          level++;
          if (level > 5)
            level=5;
          break;
      
        case SDLK_KP_MINUS:
          level--;
          if (level < 0)
            level=0;
          break;

        default:
           //no default key processing
           // (stops compiler warnings for unhandled SDL keydefs
           break;
        }
      break;

      case SDL_MOUSEMOTION:
        pitch += event.motion.yrel;
        if (pitch < -70)
          pitch = -70;
        if (pitch > 70)
          pitch = 70;
        break;

       case SDL_QUIT:
        exit (0);
        break;
      }
    }

    /* update the screen */  
    paint_begin ();
    paint_triangles ();
    glColor3f (1.0, 1.0, 1.0);
    DrawTexture (0, 0, texture, textw, texth);
    paint_end ();

    /* Wait 50ms to avoid using up all the CPU time */
    SDL_Delay (50);
  }
}


int
main (int argc, char* argv[])
{
  setup_sdl ();
  
  setup_opengl ();

  yaw = 45;
  pitch = 0;
  level = 2;
  
  main_loop ();
    
  return 0;
}

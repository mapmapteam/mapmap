//========================================================================
// Simple GLFW example
// Copyright (c) Camilla Berglund <elmindreda@elmindreda.org>
//
// This software is provided 'as-is', without any express or implied
// warranty. In no event will the authors be held liable for any damages
// arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
//
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would
//    be appreciated but is not required.  //
// 2. Altered source versions must be plainly marked as such, and must not
//    be misrepresented as being the original software.
//
// 3. This notice may not be removed or altered from any source
//    distribution.
//
//========================================================================
//! [code]

#include <GL/glfw3.h>
#include <stdlib.h>
#include <stdio.h>
#include <SOIL/SOIL.h>

typedef struct _Quad
{
  int x1;
  int x2;
  int x3;
  int x4;
  int y1;
  int y2;
  int y3;
  int y4;
} Quad;

static Quad src;
static Quad dst;

static void error_callback(int error, const char* description)
{
    fputs(description, stderr);
}

// static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
// {
//     if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
//         glfwSetWindowShouldClose(window, GL_TRUE);
// }
//

static void
move_point (int index, int x, int y)
{
  printf ("move_point (index=%d, x=%d, y=%d)\n", index, x, y);
  switch (index)
  {
    case 1:
      src.x1 += x;
      src.y1 += y;
      break;
    case 2:
      src.x2 += x;
      src.y2 += y;
      break;
    case 3:
      src.x3 += x;
      src.y3 += y;
      break;
    case 4:
      src.x4 += x;
      src.y4 += y;
      break;
    case 5:
      dst.x1 += x;
      dst.y1 += y;
      break;
    case 6:
      dst.x2 += x;
      dst.y2 += y;
      break;
    case 7:
      dst.x3 += x;
      dst.y3 += y;
      break;
    case 8:
      dst.x4 += x;
      dst.y4 += y;
      break;
  }
}

static void 
key_callback (GLFWwindow *window, int key, int scancode, int action, int mods)
{
  static int current = 0;

  //printf ("key_callback (window=%p, key=%d, scancode=%d, action=%d, mods=%d)\n",
  //  window, key, scancode, action, mods);

  if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    glfwSetWindowShouldClose(window, GL_TRUE);
  else if (key == GLFW_KEY_TAB && action == GLFW_PRESS)
  {
    current = (current + 1) % 8;
    printf ("Current = %d\n", current);
  }
  else if (action == GLFW_PRESS || action == GLFW_REPEAT)
  {
    if (key == GLFW_KEY_UP)
      move_point (current + 1, 0, 1);
    else if (key == GLFW_KEY_DOWN)
      move_point (current + 1, 0, -1);
    else if (key == GLFW_KEY_LEFT)
      move_point (current + 1, -1, 0);
    else if (key == GLFW_KEY_RIGHT)
      move_point (current + 1, 1, 0);
    else
      printf ("Unhandled key = %d\n", key);
  }
}

GLuint load_image (const char *imagepath)
{
  int width, height;
  GLuint textureID = 0;

  unsigned char * data =
    SOIL_load_image(imagepath, &width, &height, 0, SOIL_LOAD_RGB );
  textureID = SOIL_create_OGL_texture (
    data, width, height, 3, textureID, 0);

  // TODO: free data?

  return textureID;
}

int main(void)
{
    GLFWwindow* window;
    GLuint texture;
    const int wrap = 1;

    const float image_width = 320;
    const float image_height = 240;

    //source
    src.x1 = 0;
    src.y1 = 0;

    src.x2 = 320;
    src.y2 = 0;

    src.x3 = 320;
    src.y3 = 240;

    src.x4 = 0;
    src.y4 = 240;

    //destination
    dst.x1 = 0;
    dst.y1 = 0;

    dst.x2 = 320;
    dst.y2 = 0;

    dst.x3 = 320;
    dst.y3 = 240;

    dst.x4 = 0;
    dst.y4 = 240;

    glfwSetErrorCallback (error_callback);

    if (! glfwInit ())
      exit(EXIT_FAILURE);

    window = glfwCreateWindow (640, 480, "MapMap prototype", NULL, NULL);
    if (! window)
    {
        glfwTerminate ();
        exit (EXIT_FAILURE);
    }

    glfwMakeContextCurrent (window);

    texture = load_image ("example.png");

    glfwSetKeyCallback (window, (GLFWkeyfun) key_callback);

    while (! glfwWindowShouldClose (window))
    {
        float ratio;
        int width, height;

        //glfwGetFramebufferSize(window, &width, &height);
        // XXX aalex I commented the line above and added the two lines below
        width = 640;
        height = 480;

        ratio = width / (float) height;

        glViewport (0, 0, width, height);
        glClear (GL_COLOR_BUFFER_BIT);

        glMatrixMode (GL_PROJECTION);
        glLoadIdentity ();
        glOrtho (-ratio, ratio, -1.f, 1.f, 1.f, -1.f);
        glMatrixMode (GL_MODELVIEW);

        glLoadIdentity ();

        // DRAW THE TEXTURE
        glPushMatrix ();
        glDisable (GL_LIGHTING);
        glColor3f (1, 1, 1);
        glEnable (GL_TEXTURE_2D);
        glBindTexture (GL_TEXTURE_2D, texture);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glBegin (GL_QUADS);

        glTexCoord2f (src.x1 / image_width, src.y1 / image_height);
        glVertex3f (dst.x1 / image_width, dst.y1 / image_height, 0);

        glTexCoord2f (src.x2 / image_width, src.y2 / image_height);
        glVertex3f (dst.x2 / image_width, dst.y2 / image_height, 0);

        glTexCoord2f (src.x3 / image_width, src.y3 / image_height);
        glVertex3f (dst.x3 / image_width, dst.y3 / image_height, 0);

        glTexCoord2f (src.x4 / image_width, src.y4 / image_height);
        glVertex3f (dst.x4 / image_width, dst.y4 / image_height, 0);

        glEnd ();
        glDisable (GL_TEXTURE_2D);
        glPopMatrix ();

        glfwSwapBuffers (window);
        glfwPollEvents ();
    }

    glfwDestroyWindow (window);

    glfwTerminate ();
    exit (EXIT_SUCCESS);
}

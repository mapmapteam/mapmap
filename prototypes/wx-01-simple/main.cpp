// NOTE: To run, it is recommended not to be in Compiz or Beryl, they have shown some instability.
// Needs:  libwxgtk2.8-dev libsoil-dev
// g++ main.cpp -o run `wx-config --libs --cxxflags --gl-libs` -lSOIL
 
#include <wx/wx.h>
#include <wx/glcanvas.h>
#include <stdlib.h>
#include <stdio.h>
#include <SOIL/SOIL.h>
 
#ifndef WIN32
#include <unistd.h> // FIXME: ¿This work/necessary in Windows?
                    //Not necessary, but if it was, it needs to be replaced by process.h AND io.h
#endif

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

static void
move_point (Quad *src, Quad *dst, int index, int x, int y)
{
  printf ("move_point (index=%d, x=%d, y=%d)\n", index, x, y);
  switch (index)
  {
    case 1:
      src->x1 += x;
      src->y1 += y;
      break;
    case 2:
      src->x2 += x;
      src->y2 += y;
      break;
    case 3:
      src->x3 += x;
      src->y3 += y;
      break;
    case 4:
      src->x4 += x;
      src->y4 += y;
      break;
    case 5:
      dst->x1 += x;
      dst->y1 += y;
      break;
    case 6:
      dst->x2 += x;
      dst->y2 += y;
      break;
    case 7:
      dst->x3 += x;
      dst->y3 += y;
      break;
    case 8:
      dst->x4 += x;
      dst->y4 += y;
      break;
  }
}

GLuint load_image (const char *imagepath, float *image_width, float *image_height)
{
  int width, height;
  GLuint textureID = 0;

  unsigned char * data =
    SOIL_load_image(imagepath, &width, &height, 0, SOIL_LOAD_RGB );
  textureID = SOIL_create_OGL_texture (
    data, width, height, 3, textureID, 0);

  // TODO: free data?

  (* image_width) = width;
  (* image_height) = height;

  return textureID;
}
 
class wxGLCanvasSubClass: public wxGLCanvas
{
        void Render();
    public:
        wxGLCanvasSubClass(wxFrame* parent);
        void Paintit(wxPaintEvent& event);
        void movePoint(int index, int x, int y);
    protected:
        DECLARE_EVENT_TABLE()
    private:
        GLuint texture;
        float image_width;
        float image_height;
        Quad src;
        Quad dst;

        void OnChar(wxKeyEvent & event);

        void OnMouseEvent(wxMouseEvent& event);
        void setup_texture();
};
 
BEGIN_EVENT_TABLE(wxGLCanvasSubClass, wxGLCanvas)
  EVT_PAINT    (wxGLCanvasSubClass::Paintit)
  EVT_KEY_DOWN     (wxGLCanvasSubClass::OnChar)
  EVT_MOUSE_EVENTS(wxGLCanvasSubClass::OnMouseEvent)
//  EVT_ENTER_WINDOW(wxGLCanvasSubClass::OnMouseEnter)
END_EVENT_TABLE()
 
wxGLCanvasSubClass::wxGLCanvasSubClass(wxFrame *parent)
:wxGLCanvas(parent, wxID_ANY,  wxDefaultPosition, wxDefaultSize, 0, wxT("GLCanvas"))
{
    int argc = 1;
    char* argv[1] = { wxString((wxTheApp->argv)[0]).char_str() };
}
 
void wxGLCanvasSubClass::Paintit(wxPaintEvent& WXUNUSED(event))
{
    Render();
}

void wxGLCanvasSubClass::OnChar(wxKeyEvent & event)
{
    static int current = 0;
    printf("hello");
    switch (event.GetKeyCode())
    {
        case WXK_TAB:
          current = (current + 1) % 8;
          printf ("Current = %d\n", current);
          break;
        case WXK_UP:
          move_point (&src, &dst, current + 1, 0, 1);
          break;
        case WXK_DOWN:
          move_point (&src, &dst, current + 1, 0, -1);
          break;
        case WXK_LEFT:
          move_point (&src, &dst, current + 1, -1, 0);
          break;
        case WXK_RIGHT:
          move_point (&src, &dst, current + 1, 1, 0);
          break;
        default:
          printf ("Unhandled key");
          break;
    }
    Render();
}

void wxGLCanvasSubClass::OnMouseEvent(wxMouseEvent& event)
{
    printf("x=%d y=%d LeftIsDown=%d\n", event.GetX(), event.GetY(), (int)event.LeftIsDown());
    SetFocus();
}

void wxGLCanvasSubClass::setup_texture()
{
    texture = load_image ("example.png", &image_width, &image_height);

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
    dst.x1 = -320;
    dst.y1 = -240;

    dst.x2 = 0;
    dst.y2 = -240;

    dst.x3 = 0;
    dst.y3 = 0;

    dst.x4 = -320;
    dst.y4 = 0; // 240;
}
 
void wxGLCanvasSubClass::Render()
{
    static bool texture_is_set = false;
    float ratio;

    SetCurrent();
    wxPaintDC dc(this);

    if (! texture_is_set)
    {
        this->setup_texture();
        texture_is_set = true;
    }
    ratio = (float) GetSize().x / (float) GetSize().y;

    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT);
//    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glViewport(0, 0, (GLint) GetSize().x, (GLint) GetSize().y);

    glMatrixMode (GL_PROJECTION);
    glLoadIdentity ();
    glOrtho (-ratio, ratio, -1.f, 1.f, 1.f, -1.f);
    glMatrixMode (GL_MODELVIEW);

    glLoadIdentity ();

    // Now, draw
    // DRAW THE TEXTURE
    glPushMatrix ();

    // Enable blending mode (for alphas).
    glEnable (GL_BLEND);
    glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);


    glDisable (GL_LIGHTING);
    glEnable (GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);


    // Draw source texture (not moving).
    glColor4f (1, 1, 1, 0.5f);
    glBegin (GL_QUADS);
    {
      glTexCoord2f (0, 0);
      glVertex3f (0, 0, 0);

      glTexCoord2f (1, 0);
      glVertex3f (1, 0, 0);

      glTexCoord2f (1, 1);
      glVertex3f (1, 1, 0);

      glTexCoord2f (0, 1);
      glVertex3f (0, 1, 0);
    }
    glEnd ();

    // Project source texture and sent it to destination.
    glColor4f (1, 1, 1, 1.0f);
    glBegin (GL_QUADS);
    {
      glTexCoord2f (src.x1 / image_width, src.y1 / image_height);
      glVertex3f (dst.x1 / image_width, dst.y1 / image_height, 0);

      glTexCoord2f (src.x2 / image_width, src.y2 / image_height);
      glVertex3f (dst.x2 / image_width, dst.y2 / image_height, 0);

      glTexCoord2f (src.x3 / image_width, src.y3 / image_height);
      glVertex3f (dst.x3 / image_width, dst.y3 / image_height, 0);

      glTexCoord2f (src.x4 / image_width, src.y4 / image_height);
      glVertex3f (dst.x4 / image_width, dst.y4 / image_height, 0);
    }
    glEnd ();

    glDisable(GL_TEXTURE_2D);

    glColor4f (1, 1, 1, 1);

    // Source quad.
    glLineWidth(5);
    glBegin (GL_LINE_STRIP);
    {
      glVertex3f(src.x1 / image_width, src.y1 / image_height, 0);
      glVertex3f(src.x2 / image_width, src.y2 / image_height, 0);
      glVertex3f(src.x3 / image_width, src.y3 / image_height, 0);
      glVertex3f(src.x4 / image_width, src.y4 / image_height, 0);
      glVertex3f(src.x1 / image_width, src.y1 / image_height, 0);
    }
    glEnd ();

    glColor4f (1, 0, 0, 1);

    // Destination quad.
    glBegin (GL_LINE_STRIP);
    {
      glVertex3f(dst.x1 / image_width, dst.y1 / image_height, 0);
      glVertex3f(dst.x2 / image_width, dst.y2 / image_height, 0);
      glVertex3f(dst.x3 / image_width, dst.y3 / image_height, 0);
      glVertex3f(dst.x4 / image_width, dst.y4 / image_height, 0);
      glVertex3f(dst.x1 / image_width, dst.y1 / image_height, 0);
    }
    glEnd ();

    glDisable (GL_TEXTURE_2D);
    glPopMatrix ();

    // Done drawing

    glFlush();
    SwapBuffers();
}

void wxGLCanvasSubClass::movePoint(int index, int x, int y)
{
    move_point (&src, &dst, index, x, y);
}
 
class MyApp: public wxApp
{
    private:
        virtual bool OnInit();
        wxGLCanvasSubClass * MyGLCanvas;
        int FilterEvent(wxEvent& event);
};

int MyApp::FilterEvent(wxEvent& event)
{
    static int current = 0;
    if ((event.GetEventType() == wxEVT_KEY_DOWN))
    {
        switch (((wxKeyEvent&)event).GetKeyCode())
        {
            case WXK_TAB:
              current = (current + 1) % 8;
              printf ("Current = %d\n", current);
              return true;
              break;
            case WXK_UP:
              MyGLCanvas->movePoint(current + 1, 0, 1);
              return true;
              break;
            case WXK_DOWN:
              MyGLCanvas->movePoint(current + 1, 0, -1);
              return true;
              break;
            case WXK_LEFT:
              MyGLCanvas->movePoint(current + 1, -1, 0);
              return true;
              break;
            case WXK_RIGHT:
              MyGLCanvas->movePoint(current + 1, 1, 0);
              return true;
              break;
            default:
              printf ("Unhandled key");
              break;
        }
    }
    return -1;
}
 
IMPLEMENT_APP(MyApp)
 
bool MyApp::OnInit()
{
    wxFrame *frame = new wxFrame((wxFrame *) NULL, -1,  wxT("Hello GL World"), wxPoint(50, 50), wxSize(640, 480));
    MyGLCanvas = new wxGLCanvasSubClass(frame);
 
//    frame->SetWindowStyle(wxWANTS_CHARS);
    frame->Show(TRUE);
    return TRUE;
}

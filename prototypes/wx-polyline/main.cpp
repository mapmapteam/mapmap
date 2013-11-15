// NOTE: To run, it is recommended not to be in Compiz or Beryl, they have shown some instability.
// Needs:  libwxgtk2.8-dev
// g++ main.cpp -o run `wx-config --libs --cxxflags --gl-libs`
 
#include <wx/wx.h>
#include <wx/glcanvas.h>
#include <stdlib.h>
#include <stdio.h>
#include <vector>
 
#ifndef WIN32
#include <unistd.h> // FIXME: ¿This work/necessary in Windows?
          //Not necessary, but if it was, it needs to be replaced by process.h AND io.h
#endif

/*
 * Left: place point
 * Right: remove previous point
 * Middle: invisible line. (no line)
 */
class Point
{
  public:
    Point(float x, float y)
    {
      this->x = x;
      this->y = y;
    }
    float x;
    float y;
};

class PolyLine
{
  public:
    void addPoint(Point point);
    void removeLastPoint();
    void draw();
  private:
    std::vector<Point> points;
};

void PolyLine::addPoint(Point point)
{
  points.push_back(point);
}
void PolyLine::removeLastPoint()
{
  if (points.size() >= 1)
    points.erase(points.end());
}

void PolyLine::draw()
{
  std::vector<Point>::const_iterator iter;

  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glEnable(GL_BLEND);
  glDisable (GL_TEXTURE_2D);
  glEnable(GL_LINE_SMOOTH);
  glLineWidth(3.0f);

  glPushMatrix ();
  glDisable (GL_LIGHTING);
  glColor3f (0.2f, 1.0f, 0.2f);

  glBegin (GL_LINE_STRIP);

  for (iter = points.begin(); iter != points.end(); iter++)
  {
    glVertex3f ((*iter).x, (*iter).y, 0.0f);
  }

  glEnd ();
  glPopMatrix ();
}

class wxGLCanvasSubClass: public wxGLCanvas
{
    void Render();
  public:
    wxGLCanvasSubClass(wxFrame* parent);
    void Paintit(wxPaintEvent& event);
  protected:
    DECLARE_EVENT_TABLE()
  private:
    void OnMouseEvent (wxMouseEvent& event);
    void setup_polyline ();
    void drawAllLines ();
    std::vector<PolyLine> lines;
};
 
BEGIN_EVENT_TABLE(wxGLCanvasSubClass, wxGLCanvas)
  EVT_PAINT  (wxGLCanvasSubClass::Paintit)
  EVT_MOUSE_EVENTS(wxGLCanvasSubClass::OnMouseEvent)
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

void wxGLCanvasSubClass::OnMouseEvent(wxMouseEvent& event)
{
  //printf("x=%d y=%d LeftIsDown=%d\n", event.GetX(), event.GetY(), (int) event.LeftIsDown());
  bool should_render = false;

  if (event.LeftIsDown())
  {
    lines[0].addPoint(Point((float) event.GetX(), (float) event.GetY()));
    should_render = true;
  }
  else if (event.RightIsDown())
  {
    lines[0].removeLastPoint();
    should_render = true;
  }
  else
  {
    lines[0].removeLastPoint();
    lines[0].addPoint(Point((float) event.GetX(), (float) event.GetY()));
    should_render = true;
  }
  if (should_render)
    Render();
}

void wxGLCanvasSubClass::drawAllLines()
{
  std::vector<PolyLine>::iterator iter;

  for (iter = lines.begin(); iter != lines.end(); iter++)
  {
    (*iter).draw();
  }
}

void wxGLCanvasSubClass::setup_polyline()
{
  lines.clear();
  lines.push_back(PolyLine());
}

void wxGLCanvasSubClass::Render()
{
  static bool polyline_is_set = false;

  SetCurrent();
  wxPaintDC(this);

  if (! polyline_is_set)
  {
    setup_polyline();
    polyline_is_set = true;
  }

  glClearColor(0.0, 0.0, 0.0, 0.0);
  glClear(GL_COLOR_BUFFER_BIT);
  glViewport(0, 0, (GLint) GetSize().x, (GLint) GetSize().y);

  glMatrixMode (GL_PROJECTION);
  glLoadIdentity ();
  glOrtho (
    0.0f, (float) GetSize().x, // left, right
    (float) GetSize().y, 0.0f, // bottom, top
    -1.0, 1.0f);
  glMatrixMode (GL_MODELVIEW);

  glLoadIdentity ();


  drawAllLines();

  glFlush();
  SwapBuffers();
}

class MyApp: public wxApp
{
  private:
    virtual bool OnInit();
    wxGLCanvasSubClass * MyGLCanvas;
};
 
IMPLEMENT_APP(MyApp)
 
bool MyApp::OnInit()
{
  wxFrame *frame = new wxFrame((wxFrame *) NULL, -1,  wxT("Hello GL World"), wxPoint(50, 50), wxSize(640, 480));
  //frame->SetWindowStyle(wxWANTS_CHARS);
  MyGLCanvas = new wxGLCanvasSubClass(frame);
 
  frame->Show(TRUE);
  return TRUE;
}

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

typedef enum
{
  LINES = 0,
  POLYGON = 1
} Style;

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
    void clear();
    void draw();
    unsigned int size();
    void setStyle(Style style);
  private:
    std::vector<Point> points;
    Style style;
};

void PolyLine::setStyle(Style style)
{
  this->style = style;
}

unsigned int PolyLine::size()
{
  return points.size();
}

void PolyLine::addPoint(Point point)
{
  points.push_back(point);
}

void PolyLine::removeLastPoint()
{
  if (points.size() >= 1)
    points.erase(points.end());
}
void PolyLine::clear()
{
  points.clear();
}

void PolyLine::draw()
{
  std::vector<Point>::const_iterator iter;

  glPushMatrix ();
  if (style == LINES)
    glBegin (GL_LINE_STRIP);
  else if (style == POLYGON)
    glBegin (GL_POLYGON);
  for (iter = points.begin(); iter != points.end(); iter++)
    glVertex3f ((*iter).x, (*iter).y, 0.0f);
  glEnd ();
  glPopMatrix ();
}

class Manager
{
  public:
    void addPoint(Point point);
    void addPolyLine(Point point);
    void removeLastPoint();
    void draw();
    void clear();
    void setStyle(Style style);
    Style getStyle();
  private:
    std::vector<PolyLine> lines;
    unsigned int current;
    Style style;
};

void Manager::setStyle(Style style)
{
  this->style = style;
}

Style Manager::getStyle()
{
  return style;
}

void Manager::addPoint(Point point)
{
  if (lines.size() == 0)
  {
    lines.push_back(PolyLine());
    current = 0;
  }
  lines[current].addPoint(point);
}

void Manager::addPolyLine(Point point)
{
  lines.push_back(PolyLine());
  current++;
  lines[current].addPoint(point);
}

void Manager::removeLastPoint()
{
  // TODO: return bool
  if (lines.size() > 0)
  {
    if (lines[current].size() == 0)
      if (current == 0)
        lines.clear();
      else
      {
        lines.erase(lines.end());
        current--;
      }
    else
      lines[current].removeLastPoint();
  }
}

void Manager::draw()
{
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glEnable(GL_BLEND);
  glDisable (GL_TEXTURE_2D);
  glEnable(GL_LINE_SMOOTH);
  glLineWidth(3.0f);
  glDisable (GL_LIGHTING);
  glColor3f (0.2f, 1.0f, 0.2f);

  std::vector<PolyLine>::iterator iter;
  for (iter = lines.begin(); iter != lines.end(); iter++)
  {
    (*iter).setStyle(style);
    (*iter).draw();
  }
}

void Manager::clear()
{
  lines.clear();
}

void draw_marquee(float x, float y)
{
  const int length = 30;
  const int distance = 10;

  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glEnable(GL_BLEND);
  glDisable (GL_TEXTURE_2D);
  glEnable(GL_LINE_SMOOTH);
  glLineWidth(3.0f);
  glDisable (GL_LIGHTING);
  glColor3f (1.0f, 0.0f, 0.0f);

  glBegin(GL_LINES);
  glVertex3f(x - (length + distance), y, 0.0f);
  glVertex3f(x - distance, y, 0.0f);
  glEnd();

  glBegin(GL_LINES);
  glVertex3f(x + (length + distance), y, 0.0f);
  glVertex3f(x + distance, y, 0.0f);
  glEnd();

  glBegin(GL_LINES);
  glVertex3f(x, y - (length + distance), 0.0f);
  glVertex3f(x, y - distance, 0.0f);
  glEnd();

  glBegin(GL_LINES);
  glVertex3f(x, y + (length + distance), 0.0f);
  glVertex3f(x, y + distance, 0.0f);
  glEnd();
}

class MyCanvas: public wxGLCanvas
{
    void Render();
  public:
    MyCanvas(wxFrame* parent);
    void evt_paint_cb(wxPaintEvent& event);
  protected:
    DECLARE_EVENT_TABLE()
  private:
    void evt_mouse_events_cb (wxMouseEvent& event);
    void evt_key_down_cb (wxKeyEvent& event);
    void setup_polyline ();
    Manager manager;
    float mousex;
    float mousey;
};
 
BEGIN_EVENT_TABLE(MyCanvas, wxGLCanvas)
  EVT_PAINT  (MyCanvas::evt_paint_cb)
  EVT_KEY_DOWN    (MyCanvas::evt_key_down_cb)
  EVT_MOUSE_EVENTS(MyCanvas::evt_mouse_events_cb)
END_EVENT_TABLE()
 
MyCanvas::MyCanvas(wxFrame *parent)
:wxGLCanvas(parent, wxID_ANY,  wxDefaultPosition, wxDefaultSize, 0, wxT("GLCanvas"))
{
  int argc = 1;
  char* argv[1] = { wxString((wxTheApp->argv)[0]).char_str() };
}
 
void MyCanvas::evt_paint_cb(wxPaintEvent& WXUNUSED(event))
{
  Render();
}

void MyCanvas::evt_mouse_events_cb(wxMouseEvent& event)
{
  //printf("x=%d y=%d LeftIsDown=%d\n", event.GetX(), event.GetY(), (int) event.LeftIsDown());
  bool should_render = false;
  Point point = Point(mousex, mousey);
  mousex = (float) event.GetX();
  mousey = (float) event.GetY();

  should_render = true;
  if (event.LeftIsDown())
  {
    manager.addPoint(point);
    should_render = true;
  }
  else if (event.RightIsDown())
  {
    // FIXME
    manager.removeLastPoint();
    manager.removeLastPoint();
    should_render = true;
  }
  else if (event.MiddleIsDown())
  {
    manager.addPolyLine(point);
    should_render = true;
  }
  else
  {
    manager.removeLastPoint();
    manager.addPoint(point);
    should_render = true;
  }
  // if (event.Leaving()) { }
  // else if (event.Entering()) { }

  if (should_render)
    Render();
  // To catch keyboard events
  SetFocus();
}

void MyCanvas::evt_key_down_cb(wxKeyEvent & event)
{
  bool should_render = false;
  switch (event.GetKeyCode())
  {
    case WXK_TAB:
      if (manager.getStyle() == POLYGON)
        manager.setStyle(LINES);
      else
        manager.setStyle(POLYGON);
      should_render = true;
      break;
    case WXK_UP:
      break;
    case WXK_DOWN:
      break;
    case WXK_LEFT:
      break;
    case WXK_RIGHT:
      break;
    default:
      break;
  }
  if (should_render)
    Render();
}

void MyCanvas::setup_polyline()
{
  manager.clear();
}

void MyCanvas::Render()
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
  glLoadIdentity (); // FIXME? is this needed here?

  manager.draw();
  draw_marquee(mousex, mousey);

  glFlush();
  SwapBuffers();
}

class MyApp: public wxApp
{
  private:
    virtual bool OnInit();
    MyCanvas * MyGLCanvas;
};
 
IMPLEMENT_APP(MyApp)
 
bool MyApp::OnInit()
{
  wxFrame *frame = new wxFrame((wxFrame *) NULL, -1,  wxT("LibreMapping - PolyLine Prototype"), wxPoint(50, 50), wxSize(640, 480));
  //frame->SetWindowStyle(wxWANTS_CHARS);
  MyGLCanvas = new MyCanvas(frame);
 
  frame->Show(TRUE);
  return TRUE;
}

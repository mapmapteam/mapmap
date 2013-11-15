/*
 * MapperGLCanvas.cpp
 *
 * (c) 2013 Sofian Audry -- info(@)sofianaudry(.)com
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "MapperGLCanvas.h"

wxGLContext* MapperGLCanvas::sharedContext = 0;

BEGIN_EVENT_TABLE (MapperGLCanvas, wxGLCanvas)
  EVT_PAINT       (MapperGLCanvas::Paintit)
  EVT_KEY_DOWN    (MapperGLCanvas::OnChar)
  EVT_MOUSE_EVENTS(MapperGLCanvas::OnMouseEvent)
//  EVT_ENTER_WINDOW(MapperGLCanvas::OnMouseEnter)
END_EVENT_TABLE()


MapperGLCanvas::MapperGLCanvas(wxFrame *parent) :
    wxGLCanvas(parent, sharedContext, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0,
        wxT("GLCanvas")) {
  if (!sharedContext) {
    sharedContext = GetContext();
  }
  //  int argc = 1;
  //  char* argv[1] = { wxString((wxTheApp ->argv)[0]).char_str() };
}

void MapperGLCanvas::Paintit(wxPaintEvent& WXUNUSED(event)) {
  Render();
}

void MapperGLCanvas::OnChar(wxKeyEvent & event) {
  static int current = 0;
  int xMove = 0;
  int yMove = 0;
  switch (event.GetKeyCode()) {
  case WXK_TAB:
    current = (current + 1) % 4;
    break;
  case WXK_UP:
    yMove = +1;
    break;
  case WXK_DOWN:
    yMove = -1;
    break;
  case WXK_LEFT:
    xMove = -1;
    break;
  case WXK_RIGHT:
    xMove = +1;
    break;
  default:
    printf("Unhandled key");
    break;
  }

  Quad& quad = getQuad();
  Point p = quad.getVertex(current);
  p.x += xMove;
  p.y += yMove;
  quad.setVertex(current, p);

  Render();
}

void MapperGLCanvas::OnMouseEvent(wxMouseEvent& event) {
  printf("x=%d y=%d LeftIsDown=%d\n", event.GetX(), event.GetY(),
      (int) event.LeftIsDown());
  SetFocus();
}

void MapperGLCanvas::Render() {
  if (!sharedContext) {
    sharedContext = GetContext();
  }
  SetCurrent(*sharedContext);
  wxPaintDC dc(this);

  enterRender();
  doRender();
  exitRender();
}

void MapperGLCanvas::enterRender() {
  float ratio = (float) GetSize().x / (float) GetSize().y;

  glClearColor(0, 0, 0, 0);
  glClear (GL_COLOR_BUFFER_BIT);
//    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glViewport(0, 0, (GLint) GetSize().x, (GLint) GetSize().y);

  glMatrixMode (GL_PROJECTION);
  glLoadIdentity();
  glOrtho(-ratio, ratio, -1.f, 1.f, 1.f, -1.f);
  glMatrixMode (GL_MODELVIEW);
}

void MapperGLCanvas::exitRender() {
  glFlush();
  SwapBuffers();
}



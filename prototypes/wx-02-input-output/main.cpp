// NOTE: To run, it is recommended not to be in Compiz or Beryl, they have shown some instability.
 
#include <wx/wx.h>
#include "Common.h"
#include "DestinationGLCanvas.h"
#include "SourceGLCanvas.h"

#include <stdlib.h>
#include <stdio.h>
 
#ifndef WIN32
#include <unistd.h> // FIXME: ¿This work/necessary in Windows?
                    //Not necessary, but if it was, it needs to be replaced by process.h AND io.h
#endif
 
class MyApp: public wxApp
{
    virtual bool OnInit();
    SourceGLCanvas * sourceCanvas;
    DestinationGLCanvas * destinationCanvas;
//public:
//    int FilterEvent(wxEvent& event);
};
 
IMPLEMENT_APP(MyApp)
 
bool MyApp::OnInit()
{
  Common::initializeLibremapper();
  wxFrame *frame = new wxFrame((wxFrame *) NULL, -1,  wxT("Libremapping"), wxPoint(50, 50), wxSize(640, 480));

  wxBoxSizer* topSizer = new wxBoxSizer( wxHORIZONTAL );
  sourceCanvas = new SourceGLCanvas(frame);
  destinationCanvas = new DestinationGLCanvas(frame);

  topSizer->Add(sourceCanvas, 1, wxEXPAND);
  topSizer->Add(destinationCanvas, 1, wxEXPAND);


//    frame->SetWindowStyle(wxWANTS_CHARS);
  frame->SetSizer(topSizer);
  frame->Show(TRUE);

  return TRUE;
}
//
//int MyApp::FilterEvent(wxEvent& event) {
//  printf("Event: %d %d\n", event.GetEventType(), wxEVT_KEY_DOWN);
//
//  if (event.GetEventType() == wxEVT_CHAR) {
//    printf("Char: %d\n", ((wxKeyEvent&)event).GetKeyCode());
//    return true;
//  }
//
//  return -1;
//}
//


#include <MyGUI.h>
#include <OIS/OIS.h>

#include "events.hpp"

using namespace MyGUI;
using namespace OIS;
using namespace OEngine::GUI;

void EventInjector::event(Type type, int index, const void *p)
{
  if(enabled) return;

  KeyEvent *key = (KeyEvent*)p;
  MouseEvent *mouse = (MouseEvent*)p;
  MouseButtonID id = (MouseButtonID)index;

  switch(type)
    {
    case EV_KeyDown:    gui->injectKeyPress(key); break;
    case EV_KeyUp:      gui->injectKeyRelease(key); break;
    case EV_MouseDown:  gui->injectMousePress(mouse, id); break;
    case EV_MouseUp:    gui->injectMouseRelease(mouse, id); break;
    case EV_MouseMove:  gui->injectMouseMove(mouse); break;
    }
}

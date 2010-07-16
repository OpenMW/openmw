#include <MyGUI.h>
#include <OIS/OIS.h>

#include "events.hpp"

using namespace OIS;
using namespace OEngine::GUI;

void EventInjector::event(Type type, int index, const void *p)
{
  if(!enabled) return;

  if(type & EV_Keyboard)
    {
      KeyEvent *key = (KeyEvent*)p;
      MyGUI::KeyCode code = MyGUI::KeyCode::Enum(key->key);
      if(type == EV_KeyDown)
        {
          /*
            This is just a first approximation. Apparently, OIS sucks
            to such a degree that it's unable to provide any sort of
            reliable unicode character on all platforms and for all
            keys. At least that's what I surmise from the amount of
            workaround that the MyGUI folks have put in place for
            this. See Common/Input/OIS/InputManager.cpp in the MyGUI
            sources for details. If this is indeed necessary (I
            haven't tested that it is, although I have had dubious
            experinces with OIS events in the past), then we should
            probably adapt all that code here. Or even better,
            directly into the OIS input manager in Mangle.

            Note that all this only affects the 'text' field, and
            should thus only affect typed text in input boxes (which
            is still pretty significant.)
          */
          MyGUI::Char text = (MyGUI::Char)key->text;
          gui->injectKeyPress(code,text);
        }
      else
        {
          gui->injectKeyRelease(code);
        }
    }
  else if(type & EV_Mouse)
    {
      MouseEvent *mouse = (MouseEvent*)p;
      MyGUI::MouseButton id = MyGUI::MouseButton::Enum(index);

      // I'm not sure these should be used directly, MyGUI demo code
      // use local mouse position variables.
      int mouseX = mouse->state.X.abs;
      int mouseY = mouse->state.Y.abs;

      if(type == EV_MouseDown)
        gui->injectMousePress(mouseX, mouseY, id);
      else if(type == EV_MouseUp)
        gui->injectMouseRelease(mouseX, mouseY, id);
      else
        gui->injectMouseMove(mouseX, mouseY, mouse->state.Z.abs);
    }
}

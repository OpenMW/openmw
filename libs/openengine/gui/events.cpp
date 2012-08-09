#include <MyGUI.h>
#include <OIS/OIS.h>
#include <cassert>

#include "events.hpp"

using namespace OIS;
using namespace OEngine::GUI;

EventInjector::EventInjector(MyGUI::Gui *g)
  : gui(g), enabled(true)
  , mMouseX(0)
  , mMouseY(0)
{
  assert(gui);
  const MyGUI::IntSize& viewSize = MyGUI::RenderManager::getInstance().getViewSize();
  mMouseX = viewSize.width/2;
  mMouseY = viewSize.height/2;
}

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
            This is just a first approximation. Apparently, OIS is
            unable to provide reliable unicode characters on all
            platforms. At least that's what I surmise from the amount
            of workaround that the MyGUI folks have put in place for
            this. See Common/Input/OIS/InputManager.cpp in the MyGUI
            sources for details.

            If the work they have done there is indeed necessary (I
            haven't tested that it is, although I have had dubious
            experinces with OIS events in the past), then we should
            probably adapt all that code here. Or even better,
            directly into the OIS input manager in Mangle.

            Note that all this only affects the 'text' field, and
            should thus only affect typed text in input boxes (which
            is still pretty significant.)
          */
          MyGUI::Char text = (MyGUI::Char)key->text;
          MyGUI::InputManager::getInstance().injectKeyPress(code,text);
        }
      else
        {
          MyGUI::InputManager::getInstance().injectKeyRelease(code);
        }
    }
  else if(type & EV_Mouse)
    {
      MouseEvent *mouse = (MouseEvent*)p;
      MyGUI::MouseButton id = MyGUI::MouseButton::Enum(index);

      const MyGUI::IntSize& viewSize = MyGUI::RenderManager::getInstance().getViewSize();

      // Update mouse position
      mMouseX += mouse->state.X.rel;
      mMouseY += mouse->state.Y.rel;
      mMouseX = std::max(0, std::min(mMouseX, viewSize.width));
      mMouseY = std::max(0, std::min(mMouseY, viewSize.height));

      if(type == EV_MouseDown)
        MyGUI::InputManager::getInstance().injectMousePress(mMouseX, mMouseY, id);
      else if(type == EV_MouseUp)
        MyGUI::InputManager::getInstance().injectMouseRelease(mMouseX, mMouseY, id);
      else
        MyGUI::InputManager::getInstance().injectMouseMove(mMouseX, mMouseY, mouse->state.Z.abs);
    }
}

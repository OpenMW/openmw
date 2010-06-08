#ifndef _INPUT_MWINPUTMANAGER_H
#define _INPUT_MWINPUTMANAGER_H

#include "input/listener.hpp"
#include "input/dispatcher.hpp"
#include "boost/bind.hpp"

namespace MWInput
{
  enum Actions
    {
      A_Quit,           // Exit the program

      A_LAST            // Marker for the last item
    };

  // Class that handles all input and key bindings for OpenMW
  class MWInputManager
  {
    Input::Dispatcher disp;
    Input::OISManager input;
    Input::InputListener listener;

  public:
    MWInputManager(Render::OgreRenderer &ogre)
      : disp(A_LAST),
        input(ogre),
        listener(ogre, input, disp)
    {
      using namespace Input;
      using namespace OIS;

      // Bind MW-specific functions
      disp.funcs.bind(A_Quit,
                      boost::bind(&InputListener::exitNow, &listener),
                      "Quit program");

      // Key bindings
      disp.bind(KC_Q, A_Quit);
      disp.bind(KC_ESCAPE, A_Quit);
    }
  };
}
#endif

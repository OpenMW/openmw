#ifndef _MWINPUT_MWINPUTMANAGER_H
#define _MWINPUT_MWINPUTMANAGER_H

#include "input/listener.hpp"
#include "input/dispatcher.hpp"
#include "input/poller.hpp"
#include "boost/bind.hpp"
#include "game/mwrender/playerpos.hpp"

namespace MWInput
{
  enum Actions
    {
      A_Quit,           // Exit the program

      A_MoveLeft,       // Move player left / right
      A_MoveRight,
      A_MoveUp,         // Move up / down
      A_MoveDown,
      A_MoveForward,    // Forward / Backward
      A_MoveBackward,

      A_LAST            // Marker for the last item
    };

  // Class that handles all input and key bindings for OpenMW
  class MWInputManager : public Ogre::FrameListener
  {
    // Note: the order here is important. The OISManager must be
    // initialized before poller and listener.
    Input::Dispatcher disp;
    Input::OISManager input;
    Input::Poller poller;
    Input::InputListener listener;
    MWRender::PlayerPos &player;

  public:
    MWInputManager(Render::OgreRenderer &ogre,
                   MWRender::PlayerPos &_player)
      : disp(A_LAST),
        input(ogre),
        poller(input),
        listener(ogre, input, disp),
        player(_player)
    {
      using namespace Input;
      using namespace OIS;

      // Bind MW-specific functions
      disp.funcs.bind(A_Quit,
                      boost::bind(&InputListener::exitNow, &listener),
                      "Quit program");

      // Add ourselves as a frame listener, to catch movement keys
      ogre.getRoot()->addFrameListener(this);

      // Tell the input listener about the camera
      listener.setCamera(player.getCamera());

      // Key bindings
      disp.bind(KC_Q, A_Quit);
      disp.bind(KC_ESCAPE, A_Quit);

      // Key bindings for polled keys

      // Arrow keys
      poller.bind(A_MoveLeft, KC_LEFT);
      poller.bind(A_MoveRight, KC_RIGHT);
      poller.bind(A_MoveForward, KC_UP);
      poller.bind(A_MoveBackward, KC_DOWN);

      // WASD keys
      poller.bind(A_MoveLeft, KC_A);
      poller.bind(A_MoveRight, KC_D);
      poller.bind(A_MoveForward, KC_W);
      poller.bind(A_MoveBackward, KC_S);

      // Use shift and ctrl for up and down
      poller.bind(A_MoveUp, KC_LSHIFT);
      poller.bind(A_MoveDown, KC_LCONTROL);
    }

    // Used to check for movement keys
    bool frameStarted(const Ogre::FrameEvent &evt)
    {
      float speed = 300 * evt.timeSinceLastFrame;
      float moveX = 0, moveY = 0, moveZ = 0;

      using namespace std;

      if(poller.isDown(A_MoveLeft)) moveX -= speed;
      if(poller.isDown(A_MoveRight)) moveX += speed;
      if(poller.isDown(A_MoveForward)) moveZ -= speed;
      if(poller.isDown(A_MoveBackward)) moveZ += speed;

      // TODO: These should be enabled for floating modes (like
      // swimming and levitation) and disabled for everything else.
      if(poller.isDown(A_MoveUp)) moveY += speed;
      if(poller.isDown(A_MoveDown)) moveY -= speed;

      if(moveX == 0 && moveY == 0 && moveZ == 0)
        return true;

      player.moveRel(moveX, moveY, moveZ);

      return true;
    }
  };
}
#endif

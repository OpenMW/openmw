#ifndef _MWINPUT_MWINPUTMANAGER_H
#define _MWINPUT_MWINPUTMANAGER_H

#include "input/listener.hpp"
#include "input/dispatcher.hpp"
#include "input/poller.hpp"
#include "boost/bind.hpp"
#include "game/mwrender/playerpos.hpp"
#include "platform/strings.h"

namespace MWInput
{
  enum Actions
    {
      A_Quit,           // Exit the program

      A_Screenshot,     // Take a screenshot

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
    Render::OgreRenderer &ogre;
    Input::OISManager input;
    Input::Poller poller;
    Input::InputListener listener;
    MWRender::PlayerPos &player;

    // Count screenshots. TODO: We should move this functionality to
    // OgreRender or somewhere else.
    int shotCount;

    // Write screenshot to file.
    void screenshot()
    {
      // Find the first unused filename.
      //
      char buf[50];      
      do
      {
        snprintf(buf, 50, "screenshot%03d.png", shotCount++);
      } while (boost::filesystem::exists(buf));

      ogre.screenshot(buf);
    }

  public:
    MWInputManager(Render::OgreRenderer &_ogre,
                   MWRender::PlayerPos &_player)
      : disp(A_LAST),
        ogre(_ogre),
        input(_ogre),
        poller(input),
        listener(_ogre, input, disp),
        player(_player),
        shotCount(0)
    {
      using namespace Input;
      using namespace OIS;

      // Bind MW-specific functions
      disp.funcs.bind(A_Quit, boost::bind(&InputListener::exitNow, &listener),
                      "Quit program");
      disp.funcs.bind(A_Screenshot, boost::bind(&MWInputManager::screenshot, this),
                      "Screenshot");

      // Add ourselves as a frame listener, to catch movement keys
      ogre.getRoot()->addFrameListener(this);

      // Tell the input listener about the camera
      listener.setCamera(player.getCamera());

      // Key bindings
      disp.bind(KC_Q, A_Quit);
      disp.bind(KC_ESCAPE, A_Quit);
      disp.bind(KC_SYSRQ, A_Screenshot);

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

      if(poller.isDown(A_MoveLeft)) moveX -= speed;
      if(poller.isDown(A_MoveRight)) moveX += speed;
      if(poller.isDown(A_MoveForward)) moveZ -= speed;
      if(poller.isDown(A_MoveBackward)) moveZ += speed;

      // TODO: These should be enabled for floating modes (like
      // swimming and levitation) and disabled for everything else.
      if(poller.isDown(A_MoveUp)) moveY += speed;
      if(poller.isDown(A_MoveDown)) moveY -= speed;

      if(moveX != 0 || moveY != 0 || moveZ != 0)
        player.moveRel(moveX, moveY, moveZ);

      return true;
    }
  };
}
#endif

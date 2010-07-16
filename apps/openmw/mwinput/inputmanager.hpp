#ifndef _MWINPUT_MWINPUTMANAGER_H
#define _MWINPUT_MWINPUTMANAGER_H

#include <openengine/input/dispatcher.hpp>
#include <openengine/input/poller.hpp>

#include <openengine/ogre/exitlistener.hpp>
#include <openengine/ogre/mouselook.hpp>
#include <openengine/ogre/renderer.hpp>

#include <mangle/input/servers/ois_driver.hpp>
#include <mangle/input/filters/eventlist.hpp>

#include <libs/platform/strings.h>
#include <boost/bind.hpp>
#include "../mwrender/playerpos.hpp"

#include <OgreRoot.h>

#include <OIS/OIS.h>

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
    OEngine::Input::DispatcherPtr disp;
    OEngine::Render::OgreRenderer &ogre;
    OEngine::Render::ExitListener exit;
    Mangle::Input::OISDriver input;
    OEngine::Input::Poller poller;
    OEngine::Render::MouseLookEventPtr mouse;
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
    MWInputManager(OEngine::Render::OgreRenderer &_ogre,
                   MWRender::PlayerPos &_player, bool debug)
      : ogre(_ogre),
        exit(ogre.getWindow()),
        input(ogre.getWindow(), !debug),
        poller(input),
        player(_player),
        shotCount(0)
    {
      using namespace OEngine::Input;
      using namespace OEngine::Render;
      using namespace Mangle::Input;
      using namespace OIS;

      disp = DispatcherPtr(new Dispatcher(A_LAST));

      // Bind MW-specific functions
      disp->funcs.bind(A_Quit, boost::bind(&ExitListener::exitNow, &exit),
                      "Quit program");
      disp->funcs.bind(A_Screenshot, boost::bind(&MWInputManager::screenshot, this),
                      "Screenshot");


      // Add the exit listener
      ogre.getRoot()->addFrameListener(&exit);
      // Add ourselves as a frame listener to catch movement keys
      ogre.getRoot()->addFrameListener(this);

      // Set up the mouse handler and tell it about the player camera
      mouse = MouseLookEventPtr(new MouseLookEvent(player.getCamera()));

      // Hook 'mouse' and 'disp' up as event handlers into 'input'
      // (the OIS driver and event source.) We do this through an
      // EventList which dispatches the event to multiple handlers for
      // us.
      {
        EventList *lst = new EventList;
        input.setEvent(EventPtr(lst));
        lst->add(mouse,Event::EV_MouseMove);
        lst->add(disp,Event::EV_KeyDown);
      }

      // Key bindings
      disp->bind(A_Quit, KC_Q);
      disp->bind(A_Quit, KC_ESCAPE);
      disp->bind(A_Screenshot, KC_SYSRQ);

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
      // Tell OIS to handle all input events
      input.capture();

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

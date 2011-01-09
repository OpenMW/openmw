#include "inputmanager.hpp"

#include <openengine/input/dispatcher.hpp>
#include <openengine/input/poller.hpp>

#include <openengine/gui/events.hpp>

#include <openengine/ogre/exitlistener.hpp>
#include <openengine/ogre/mouselook.hpp>
#include <openengine/ogre/renderer.hpp>

#include "../mwgui/window_manager.hpp"

#include <mangle/input/servers/ois_driver.hpp>
#include <mangle/input/filters/eventlist.hpp>

#include <libs/platform/strings.h>
#include "../mwrender/playerpos.hpp"

#include "../engine.hpp"

#include <boost/bind.hpp>
#include <boost/filesystem.hpp>
#include <OgreRoot.h>
#include <OIS/OIS.h>

namespace MWInput
{
  enum Actions
    {
      A_Quit,           // Exit the program

      A_Screenshot,     // Take a screenshot

      A_Inventory,      // Toggle inventory screen

      A_Console,        // Toggle console screen

      A_MoveLeft,       // Move player left / right
      A_MoveRight,
      A_MoveUp,         // Move up / down
      A_MoveDown,
      A_MoveForward,    // Forward / Backward
      A_MoveBackward,

      A_Activate,

      A_Use,		//Use weapon, spell, etc.
      A_Jump,
      A_AutoMove, 	//Toggle Auto-move forward
      A_Rest, 		//Rest
      A_Journal,	//Journal
      A_Weapon,		//Draw/Sheath weapon
      A_Spell,		//Ready/Unready Casting
      A_AlwaysRun,	//Toggle Always Run
      A_CycleSpellLeft, //cycling through spells
      A_CycleSpellRight,
      A_CycleWeaponLeft,//Cycling through weapons
      A_CycleWeaponRight,

      A_QuickSave,
      A_QuickLoad,
      A_QuickMenu,
      A_GameMenu,

      A_LAST            // Marker for the last item
    };

  // Class that handles all input and key bindings for OpenMW
  class InputImpl : public Ogre::FrameListener
  {
    OEngine::Input::DispatcherPtr disp;
    OEngine::Render::OgreRenderer &ogre;
    OEngine::Render::ExitListener exit;
    Mangle::Input::OISDriver input;
    OEngine::Input::Poller poller;
    OEngine::Render::MouseLookEventPtr mouse;
    OEngine::GUI::EventInjectorPtr guiEvents;
    MWRender::PlayerPos &player;
    MWGui::WindowManager &windows;
    OMW::Engine& mEngine;

    // Count screenshots.
    int shotCount;

    // Write screenshot to file.
    void screenshot()
    {
      
      // Find the first unused filename with a do-while
      char buf[50];
      do
      {
        snprintf(buf, 50, "screenshot%03d.png", shotCount++);
      } while (boost::filesystem::exists(buf));

      ogre.screenshot(buf);
    }

    // Called when the user presses the button to toggle the inventory
    // screen.
    void toggleInventory()
    {
      using namespace MWGui;

      GuiMode mode = windows.getMode();

      // Toggle between game mode and inventory mode
      if(mode == GM_Game)
        setGuiMode(GM_Inventory);
      else if(mode == GM_Inventory)
        setGuiMode(GM_Game);

      // .. but don't touch any other mode.
    }

    // Toggle console
    void toggleConsole()
    {
      using namespace MWGui;

      GuiMode mode = windows.getMode();

      // Switch to console mode no matter what mode we are currently
      // in, except of course if we are already in console mode
      if(mode == GM_Console)
        setGuiMode(GM_Game);
      else setGuiMode(GM_Console);
    }

    void activate()
    {
        mEngine.activate();
    }

    // Exit program now button (which is disabled in GUI mode)
    void exitNow()
    {
      if(!windows.isGuiMode())
        exit.exitNow();
    }

  public:
    InputImpl(OEngine::Render::OgreRenderer &_ogre,
                   MWRender::PlayerPos &_player,
                   MWGui::WindowManager &_windows,
                   bool debug,
                   OMW::Engine& engine)
      : ogre(_ogre),
        exit(ogre.getWindow()),
        input(ogre.getWindow(), !debug),
        poller(input),
        player(_player),
        windows(_windows),
        mEngine (engine),
        shotCount(0)
    {
      using namespace OEngine::Input;
      using namespace OEngine::Render;
      using namespace OEngine::GUI;
      using namespace Mangle::Input;
      using namespace OIS;

      disp = DispatcherPtr(new Dispatcher(A_LAST));

      // Bind MW-specific functions
      disp->funcs.bind(A_Quit, boost::bind(&InputImpl::exitNow, this),
                      "Quit program");
      disp->funcs.bind(A_Screenshot, boost::bind(&InputImpl::screenshot, this),
                      "Screenshot");
      disp->funcs.bind(A_Inventory, boost::bind(&InputImpl::toggleInventory, this),
                       "Toggle inventory screen");
      disp->funcs.bind(A_Console, boost::bind(&InputImpl::toggleConsole, this),
                       "Toggle console");
      disp->funcs.bind(A_Activate, boost::bind(&InputImpl::activate, this),
                       "Activate");


      // Add the exit listener
      ogre.getRoot()->addFrameListener(&exit);
      // Add ourselves as a frame listener to catch movement keys
      ogre.getRoot()->addFrameListener(this);

      // Set up the mouse handler and tell it about the player camera
      mouse = MouseLookEventPtr(new MouseLookEvent(player.getCamera()));

      // This event handler pumps events into MyGUI
      guiEvents = EventInjectorPtr(new EventInjector(windows.getGui()));

      // Hook 'mouse' and 'disp' up as event handlers into 'input'
      // (the OIS driver and event source.) We do this through an
      // EventList which dispatches the event to multiple handlers for
      // us.
      {
        EventList *lst = new EventList;
        input.setEvent(EventPtr(lst));
        lst->add(mouse,Event::EV_MouseMove);
        lst->add(disp,Event::EV_KeyDown);
        lst->add(guiEvents,Event::EV_ALL);
      }

      // Start out in game mode
      setGuiMode(MWGui::GM_Game);

      /**********************************
        Key binding section

        The rest of this function has hard coded key bindings, and is
        intended to be replaced by user defined bindings later.
       **********************************/

      // Key bindings for keypress events

      disp->bind(A_Quit, KC_Q);
      disp->bind(A_Quit, KC_ESCAPE);
      disp->bind(A_Screenshot, KC_SYSRQ);
      disp->bind(A_Inventory, KC_I);
      disp->bind(A_Console, KC_F1);
      disp->bind(A_Activate, KC_SPACE);

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

      // Update windows/gui as a result of input events
      // For instance this could mean opening a new window/dialog,
      // by doing this after the input events are handled we
      // ensure that window/gui changes appear quickly while
      // avoiding that window/gui changes does not happen in
      // event callbacks (which may crash)
      windows.update();

      // Disable movement in Gui mode
      if(windows.isGuiMode()) return true;

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

    // Switch between gui modes. Besides controlling the Gui windows
    // this also makes sure input is directed to the right place
    void setGuiMode(MWGui::GuiMode mode)
    {
      // Tell the GUI what to show (this also takes care of the mouse
      // pointer)
      windows.setMode(mode);

      // Are we in GUI mode now?
      if(windows.isGuiMode())
        {
          // Disable mouse look
          mouse->setCamera(NULL);

          // Enable GUI events
          guiEvents->enabled = true;
        }
      else
        {
          // Start mouse-looking again. TODO: This should also allow
          // for other ways to disable mouselook, like paralyzation.
          mouse->setCamera(player.getCamera());

          // Disable GUI events
          guiEvents->enabled = false;
        }
    }
  };

  MWInputManager::MWInputManager(OEngine::Render::OgreRenderer &ogre,
                                 MWRender::PlayerPos &player,
                                 MWGui::WindowManager &windows,
                                 bool debug,
                                 OMW::Engine& engine)
  {
    impl = new InputImpl(ogre,player,windows,debug, engine);
  }

  MWInputManager::~MWInputManager()
  {
    delete impl;
  }

  void MWInputManager::setGuiMode(MWGui::GuiMode mode)
  { 
      impl->setGuiMode(mode);
  }
}

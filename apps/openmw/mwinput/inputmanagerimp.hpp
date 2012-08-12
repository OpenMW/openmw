#ifndef _MWINPUT_MWINPUTMANAGERIMP_H
#define _MWINPUT_MWINPUTMANAGERIMP_H

#include "../mwgui/mode.hpp"

#include <components/settings/settings.hpp>

#include "../mwbase/inputmanager.hpp"

namespace OEngine
{
  namespace Render
  {
    class OgreRenderer;
  }
}

namespace MWWorld
{
  class Player;
}

namespace MWBase
{
  class WindowManager;
}

namespace OMW
{
    class Engine;
}

namespace MWInput
{
  // Forward declaration of the real implementation.
  class InputImpl;

  /* Class that handles all input and key bindings for OpenMW.

     This class is just an interface. All the messy details are in
     inputmanager.cpp.
   */
  struct MWInputManager : public MWBase::InputManager
  {
    InputImpl *impl;

  public:
    MWInputManager(OEngine::Render::OgreRenderer &_ogre,
                   MWWorld::Player&_player,
                   MWBase::WindowManager &_windows,
                   bool debug,
                   OMW::Engine& engine);
    virtual ~MWInputManager();

    virtual void update();

    virtual void changeInputMode(bool guiMode);

    virtual void processChangedSettings(const Settings::CategorySettingVector& changed);

    virtual void setDragDrop(bool dragDrop);

    virtual void toggleControlSwitch (const std::string& sw, bool value);
  };
}
#endif

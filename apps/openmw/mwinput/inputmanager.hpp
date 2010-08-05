#ifndef _MWINPUT_MWINPUTMANAGER_H
#define _MWINPUT_MWINPUTMANAGER_H

namespace OEngine
{
  namespace Render
  {
    class OgreRenderer;
  }
}

namespace MWRender
{
  class PlayerPos;
}

namespace MWGui
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
  struct MWInputManager
  {
    InputImpl *impl;

  public:
    MWInputManager(OEngine::Render::OgreRenderer &_ogre,
                   MWRender::PlayerPos &_player,
                   MWGui::WindowManager &_windows,
                   bool debug,
                   OMW::Engine& engine);
    ~MWInputManager();
  };
}
#endif

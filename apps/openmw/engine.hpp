#ifndef ENGINE_H
#define ENGINE_H

#include <string>

#include <boost/filesystem.hpp>

#include <OgreFrameListener.h>

#include <openengine/ogre/renderer.hpp>
#include <components/compiler/extensions.hpp>

#include "mwworld/environment.hpp"

namespace Compiler
{
    class Context;
}

namespace MWScript
{
    class ScriptManager;
}

namespace MWSound
{
    class SoundManager;
}

namespace MWWorld
{
    class World;
}

namespace MWGui
{
    class WindowManager;
}

namespace OEngine
{
  namespace GUI
  {
    class MyGUIManager;
  }
}

namespace OMW
{
    /// \brief Main engine class, that brings together all the components of OpenMW

    class Engine : private Ogre::FrameListener
    {
            boost::filesystem::path mDataDir;
            OEngine::Render::OgreRenderer mOgre;
            std::string mCellName;
            std::string mMaster;
            bool mDebug;
            bool mVerboseScripts;
            bool mNewGame;

            MWWorld::Environment mEnvironment;
            MWScript::ScriptManager *mScriptManager;
            Compiler::Extensions mExtensions;
            Compiler::Context *mScriptContext;
            OEngine::GUI::MyGUIManager *mGuiManager;

            int focusFrameCounter;
            static const int focusUpdateFrame = 10;

            // not implemented
            Engine (const Engine&);
            Engine& operator= (const Engine&);

            /// add resources directory
            /// \note This function works recursively.
            void addResourcesDirectory (const boost::filesystem::path& path);

            /// Load all BSA files in data directory.
            void loadBSA();

            void executeLocalScripts();

            virtual bool frameStarted(const Ogre::FrameEvent& evt);

            /// Process pending commands

        public:

            Engine();

            ~Engine();

            /// Set data dir
            void setDataDir (const boost::filesystem::path& dataDir);

            /// Set start cell name (only interiors for now)
            void setCell (const std::string& cellName);

            /// Set master file (esm)
            /// - If the given name does not have an extension, ".esm" is added automatically
            /// - Currently OpenMW only supports one master at the same time.
            void addMaster (const std::string& master);

            /// Enable debug mode:
            /// - non-exclusive input
            void enableDebugMode();

            /// Enable the command server so external apps can send commands to the console.
            /// Must be set before go().

            /// Enable verbose script output
            void enableVerboseScripts();
            
            /// Start as a new game.
            void setNewGame();

            /// Initialise and enter main loop.
            void go();
    };
}

#endif

#ifndef ENGINE_H
#define ENGINE_H

#include <OgreFrameListener.h>

#include <components/compiler/extensions.hpp>
#include <components/files/collections.hpp>
#include <components/translation/translation.hpp>
#include <components/settings/settings.hpp>
#include <components/nifcache/nifcache.hpp>


#include "mwbase/environment.hpp"

#include "mwworld/ptr.hpp"

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

  namespace Render
  {
    class OgreRenderer;
  }
}

namespace Files
{
    struct ConfigurationManager;
}

namespace OMW
{
    /// \brief Main engine class, that brings together all the components of OpenMW
    class Engine : private Ogre::FrameListener
    {
            MWBase::Environment mEnvironment;
            ToUTF8::FromType mEncoding;
            ToUTF8::Utf8Encoder* mEncoder;
            Files::PathContainer mDataDirs;
            std::vector<std::string> mArchives;
            boost::filesystem::path mResDir;
            OEngine::Render::OgreRenderer *mOgre;
            std::string mCellName;
            std::vector<std::string> mContentFiles;
            bool mVerboseScripts;
            bool mSkipMenu;
            bool mUseSound;
            bool mCompileAll;
            bool mCompileAllDialogue;
            int mWarningsMode;
            std::string mFocusName;
            std::map<std::string,std::string> mFallbackMap;
            bool mScriptConsoleMode;
            std::string mStartupScript;
            int mActivationDistanceOverride;
            std::string mSaveGameFile;
            // Grab mouse?
            bool mGrab;

            bool mExportFonts;

            Compiler::Extensions mExtensions;
            Compiler::Context *mScriptContext;

            Files::Collections mFileCollections;
            bool mFSStrict;
            Translation::Storage mTranslationDataStorage;
            std::vector<std::string> mScriptBlacklist;
            bool mScriptBlacklistUse;
            bool mNewGame;

            Nif::Cache mNifCache;

            // not implemented
            Engine (const Engine&);
            Engine& operator= (const Engine&);

            /// add resources directory
            /// \note This function works recursively.
            void addResourcesDirectory (const boost::filesystem::path& path);

            /// add a .zip resource
            void addZipResource (const boost::filesystem::path& path);

            void executeLocalScripts();

            virtual bool frameRenderingQueued (const Ogre::FrameEvent& evt);
            virtual bool frameStarted (const Ogre::FrameEvent& evt);

            /// Load settings from various files, returns the path to the user settings file
            std::string loadSettings (Settings::Manager & settings);

            /// Prepare engine for game play
            void prepareEngine (Settings::Manager & settings);

        public:
            Engine(Files::ConfigurationManager& configurationManager);
            virtual ~Engine();

            /// Enable strict filesystem mode (do not fold case)
            ///
            /// \attention The strict mode must be specified before any path-related settings
            /// are given to the engine.
            void enableFSStrict(bool fsStrict);

            /// Set data dirs
            void setDataDirs(const Files::PathContainer& dataDirs);

            /// Add BSA archive
            void addArchive(const std::string& archive);

            /// Set resource dir
            void setResourceDir(const boost::filesystem::path& parResDir);

            /// Set start cell name (only interiors for now)
            void setCell(const std::string& cellName);

            /**
             * @brief addContentFile - Adds content file (ie. esm/esp, or omwgame/omwaddon) to the content files container.
             * @param file - filename (extension is required)
             */
            void addContentFile(const std::string& file);

            /// Enable or disable verbose script output
            void setScriptsVerbosity(bool scriptsVerbosity);

            /// Disable or enable all sounds
            void setSoundUsage(bool soundUsage);

            /// Skip main menu and go directly into the game
            ///
            /// \param newGame Start a new game instead off dumping the player into the game
            /// (ignored if !skipMenu).
            void setSkipMenu (bool skipMenu, bool newGame);

            void setGrabMouse(bool grab) { mGrab = grab; }

            /// Initialise and enter main loop.
            void go();

            /// Activate the focussed object.
            void activate();

            /// Write screenshot to file.
            void screenshot();

            /// Compile all scripts (excludign dialogue scripts) at startup?
            void setCompileAll (bool all);

            /// Compile all dialogue scripts at startup?
            void setCompileAllDialogue (bool all);

            /// Font encoding
            void setEncoding(const ToUTF8::FromType& encoding);

            void setFallbackValues(std::map<std::string,std::string> map);

            /// Enable console-only script functionality
            void setScriptConsoleMode (bool enabled);

            /// Set path for a script that is run on startup in the console.
            void setStartupScript (const std::string& path);

            /// Override the game setting specified activation distance.
            void setActivationDistanceOverride (int distance);

            void setWarningsMode (int mode);

            void setScriptBlacklist (const std::vector<std::string>& list);

            void setScriptBlacklistUse (bool use);

            void enableFontExport(bool exportFonts);

            /// Set the save game file to load after initialising the engine.
            void setSaveGameFile(const std::string& savegame);

        private:
            Files::ConfigurationManager& mCfgMgr;
    };
}

#endif /* ENGINE_H */

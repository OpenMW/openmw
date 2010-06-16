#ifndef ENGINE_H
#define ENGINE_H

#include <string>

#include <boost/filesystem.hpp>

#include "mwrender/mwscene.hpp"

namespace OMW
{
    /// \brief Main engine class, that brings together all the components of OpenMW

    class Engine
    {
            boost::filesystem::path mDataDir;
            Render::OgreRenderer mOgre;
            std::string mCellName;
            std::string mMaster;
            
            // not implemented
            Engine (const Engine&);
            Engine& operator= (const Engine&);
    
            /// adjust name and load bsa
            void prepareMaster();
    
        public:

            Engine();

            /// Set data dir
            void setDataDir (const boost::filesystem::path& dataDir);

            /// Set start cell name (only interiors for now)
            void setCell (const std::string& cellName);
            
            /// Set master file (esm)
            /// - If the given name does not have an extension, ".esm" is added automatically
            /// - If there is a bsa file with the same name, OpenMW will load it.
            /// - Currently OpenMW only supports one master at the same time.
            void addMaster (const std::string& master);

            /// Initialise and enter main loop.
            void go();
    };
}

#endif

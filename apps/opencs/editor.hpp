#ifndef CS_EDITOR_H
#define CS_EDITOR_H

#include <fstream>

#include <boost/interprocess/sync/file_lock.hpp>
#include <boost/program_options/variables_map.hpp>

#include <QObject>
#include <QString>
#include <QLocalServer>
#include <QLocalSocket>

#ifndef Q_MOC_RUN
#include <components/files/configurationmanager.hpp>
#endif

#include <components/files/multidircollection.hpp>

#include "model/doc/documentmanager.hpp"

#include "model/prefs/state.hpp"

#include "view/doc/viewmanager.hpp"
#include "view/doc/startup.hpp"
#include "view/doc/filedialog.hpp"
#include "view/doc/newgame.hpp"

#include "view/prefs/dialogue.hpp"

#include "view/tools/merge.hpp"

namespace CSMDoc
{
    class Document;
}

namespace CS
{
    class Editor : public QObject
    {
            Q_OBJECT

            Files::ConfigurationManager mCfgMgr;
            boost::program_options::variables_map mConfigVariables;
            CSMPrefs::State mSettingsState;
            CSMDoc::DocumentManager mDocumentManager;
            CSVDoc::StartupDialogue mStartup;
            CSVDoc::NewGameDialogue mNewGame;
            CSVPrefs::Dialogue mSettings;
            CSVDoc::FileDialog mFileDialog;
            std::filesystem::path mLocal;
            std::filesystem::path mResources;
            std::filesystem::path mPid;
            boost::interprocess::file_lock mLock;
            std::ofstream mPidFile;
            bool mFsStrict;
            CSVTools::Merge mMerge;
            CSVDoc::ViewManager* mViewManager;
            std::filesystem::path mFileToLoad;
            Files::PathContainer mDataDirs;
            std::string mEncodingName;

            boost::program_options::variables_map readConfiguration();
            ///< Calls mCfgMgr.readConfiguration; should be used before initialization of mSettingsState as it depends on the configuration.
            std::pair<Files::PathContainer, std::vector<std::string> > readConfig(bool quiet=false);
            ///< \return data paths

            // not implemented
            Editor (const Editor&);
            Editor& operator= (const Editor&);

        public:

            Editor (int argc, char **argv);
            ~Editor ();

            bool makeIPCServer();
            void connectToIPCServer();

            int run();
            ///< \return error status

        private slots:

            void createGame();
            void createAddon();
            void cancelCreateGame();
            void cancelFileDialog();

            void loadDocument();
            void openFiles (const std::filesystem::path &path, const std::vector<std::filesystem::path> &discoveredFiles = {});
            void createNewFile (const std::filesystem::path& path);
            void createNewGame (const std::filesystem::path& file);

            void showStartup();

            void showSettings();

            void documentAdded (CSMDoc::Document *document);

            void documentAboutToBeRemoved (CSMDoc::Document *document);

            void lastDocumentDeleted();

            void mergeDocument (CSMDoc::Document *document);

        private:

            QString mIpcServerName;
            QLocalServer *mServer;
            QLocalSocket *mClientSocket;
    };
}

#endif

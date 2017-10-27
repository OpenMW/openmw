#ifndef CS_EDITOR_H
#define CS_EDITOR_H

#include <boost/interprocess/sync/file_lock.hpp>
#include <boost/filesystem/fstream.hpp>

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
            CSMPrefs::State mSettingsState;
            CSMDoc::DocumentManager mDocumentManager;
            CSVDoc::ViewManager mViewManager;
            CSVDoc::StartupDialogue mStartup;
            CSVDoc::NewGameDialogue mNewGame;
            CSVPrefs::Dialogue mSettings;
            CSVDoc::FileDialog mFileDialog;
            boost::filesystem::path mLocal;
            boost::filesystem::path mResources;
            boost::filesystem::path mPid;
            boost::interprocess::file_lock mLock;
            boost::filesystem::ofstream mPidFile;
            bool mFsStrict;
            CSVTools::Merge mMerge;

            void setupDataFiles (const Files::PathContainer& dataDirs);

            std::pair<Files::PathContainer, std::vector<std::string> > readConfig(bool quiet=false);
            ///< \return data paths

            // not implemented
            Editor (const Editor&);
            Editor& operator= (const Editor&);

        public:

            Editor ();
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
            void openFiles (const boost::filesystem::path &path);
            void createNewFile (const boost::filesystem::path& path);
            void createNewGame (const boost::filesystem::path& file);

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

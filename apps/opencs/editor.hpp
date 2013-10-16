#ifndef CS_EDITOR_H
#define CS_EDITOR_H

#include <QObject>
#include <QString>
#include <QLocalServer>
#include <QLocalSocket>

#ifndef Q_MOC_RUN
#include <components/files/configurationmanager.hpp>
#endif

#include "model/settings/usersettings.hpp"
#include "model/doc/documentmanager.hpp"

#include "view/doc/viewmanager.hpp"
#include "view/doc/startup.hpp"
#include "view/doc/filedialog.hpp"
#include "view/doc/newgame.hpp"

#include "view/settings/usersettingsdialog.hpp"

namespace CS
{
    class Editor : public QObject
    {
            Q_OBJECT

            Files::ConfigurationManager mCfgMgr;
            CSMSettings::UserSettings mUserSettings;
            CSMDoc::DocumentManager mDocumentManager;
            CSVDoc::ViewManager mViewManager;
            CSVDoc::StartupDialogue mStartup;
            CSVDoc::NewGameDialogue mNewGame;
            CSVSettings::UserSettingsDialog mSettings;
            CSVDoc::FileDialog mFileDialog;

            boost::filesystem::path mLocal;

            void setupDataFiles();

            // not implemented
            Editor (const Editor&);
            Editor& operator= (const Editor&);

        public:

            Editor();

            bool makeIPCServer();
            void connectToIPCServer();

            int run(int argc, char** argv);
            ///< \return error status

        private slots:

            void createGame();
            void createAddon();

            void loadDocument();
            void openFiles();
            void createNewFile (const boost::filesystem::path& savePath);
            void createNewGame (const boost::filesystem::path& file);

            void showStartup();

            void showSettings();
	    bool parseOptions (int argc, char** argv);
	    void setResourceDir (const boost::filesystem::path& parResDir);

        private:

            QString mIpcServerName;
            QLocalServer *mServer;
            QLocalSocket *mClientSocket;
	    boost::filesystem::path mResDir;
    };
}

#endif

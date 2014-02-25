#ifndef CS_EDITOR_H
#define CS_EDITOR_H

#include <QObject>
#include <QString>
#include <QLocalServer>
#include <QLocalSocket>

#ifndef Q_MOC_RUN
#include <components/files/configurationmanager.hpp>
#endif

#include <components/files/multidircollection.hpp>

#include "model/settings/usersettings.hpp"
#include "model/doc/documentmanager.hpp"

#include "view/doc/viewmanager.hpp"
#include "view/doc/startup.hpp"
#include "view/doc/filedialog.hpp"
#include "view/doc/newgame.hpp"

#include "view/settings/usersettingsdialog.hpp"

namespace OgreInit
{
    class OgreInit;
}

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

            void setupDataFiles (const Files::PathContainer& dataDirs);

            Files::PathContainer readConfig();
            ///< \return data paths

            // not implemented
            Editor (const Editor&);
            Editor& operator= (const Editor&);

        public:

            Editor (OgreInit::OgreInit& ogreInit);

            bool makeIPCServer();
            void connectToIPCServer();

            int run();
            ///< \return error status

        private slots:

            void createGame();
            void createAddon();

            void loadDocument();
            void openFiles (const boost::filesystem::path &path);
            void createNewFile (const boost::filesystem::path& path);
            void createNewGame (const boost::filesystem::path& file);

            void showStartup();

            void showSettings();

        private:

            QString mIpcServerName;
            QLocalServer *mServer;
            QLocalSocket *mClientSocket;
    };
}

#endif

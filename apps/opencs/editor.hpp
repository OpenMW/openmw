#ifndef CS_EDITOR_H
#define CS_EDITOR_H

#include <QObject>
#include <QString>
#include <QLocalServer>
#include <QLocalSocket>

#ifndef Q_MOC_RUN
#include <components/files/configurationmanager.hpp>
#endif
#include "model/doc/documentmanager.hpp"

#include "view/doc/viewmanager.hpp"
#include "view/doc/startup.hpp"
#include "view/doc/filedialog.hpp"
#include "model/settings/usersettings.hpp"

namespace CS
{
    class Editor : public QObject
    {
            Q_OBJECT

            CSMSettings::UserSettings mUserSettings;
            CSMDoc::DocumentManager mDocumentManager;
            CSVDoc::ViewManager mViewManager;
            CSVDoc::StartupDialogue mStartup;
            FileDialog mFileDialog;

            Files::ConfigurationManager mCfgMgr;
            void setupDataFiles();

            // not implemented
            Editor (const Editor&);
            Editor& operator= (const Editor&);

        public:

            Editor();

            bool makeIPCServer();
            void connectToIPCServer();

            int run();
            ///< \return error status

        private slots:

            void createDocument();

            void loadDocument();
            void openFiles();
            void createNewFile();

            void showStartup();

        private:

            QString mIpcServerName;
            QLocalServer *mServer;
            QLocalSocket *mClientSocket;
    };
}

#endif

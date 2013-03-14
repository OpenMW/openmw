#ifndef CS_EDITOR_H
#define CS_EDITOR_H

#include <QObject>

#include <components/files/configurationmanager.hpp>

#include "model/doc/documentmanager.hpp"

#include "view/doc/viewmanager.hpp"
#include "view/doc/startup.hpp"
#include "view/doc/filedialog.hpp"

namespace CS
{
    class Editor : public QObject
    {
            Q_OBJECT

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

            int run();
            ///< \return error status

        private slots:

            void createDocument();

            void loadDocument();
            void openFiles();
            void createNewFile();
    };
}

#endif

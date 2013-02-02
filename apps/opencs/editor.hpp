#ifndef CS_EDITOR_H
#define CS_EDITOR_H

#include <QObject>

#include "model/doc/documentmanager.hpp"

#include "view/doc/viewmanager.hpp"
#include "view/doc/startup.hpp"

namespace CS
{
    class Editor : public QObject
    {
            Q_OBJECT

            int mNewDocumentIndex; ///< \todo remove when the proper new document dialogue is implemented.

            CSMDoc::DocumentManager mDocumentManager;
            CSVDoc::ViewManager mViewManager;
            CSVDoc::StartupDialogue mStartup;

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
    };
}

#endif
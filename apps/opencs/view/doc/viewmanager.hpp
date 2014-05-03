#ifndef CSV_DOC_VIEWMANAGER_H
#define CSV_DOC_VIEWMANAGER_H

#include <vector>

#include <QObject>

#include "loader.hpp"

namespace CSMDoc
{
    class Document;
    class DocumentManager;
}

namespace CSVWorld
{
    class CommandDelegateFactoryCollection;
}

namespace CSVDoc
{
    class View;

    class ViewManager : public QObject
    {
            Q_OBJECT

            CSMDoc::DocumentManager& mDocumentManager;
            std::vector<View *> mViews;
            CSVWorld::CommandDelegateFactoryCollection *mDelegateFactories;
            bool mExitOnSaveStateChange;
            bool mUserWarned;
            Loader mLoader;

            // not implemented
            ViewManager (const ViewManager&);
            ViewManager& operator= (const ViewManager&);

            void updateIndices();
            bool notifySaveOnClose (View *view = 0);
            bool showModifiedDocumentMessageBox (View *view);
            bool showSaveInProgressMessageBox (View *view);

        public:

            ViewManager (CSMDoc::DocumentManager& documentManager);

            virtual ~ViewManager();

            View *addView (CSMDoc::Document *document);
            ///< The ownership of the returned view is not transferred.

            int countViews (const CSMDoc::Document *document) const;
            ///< Return number of views for \a document.

            bool closeRequest (View *view);

        signals:

            void newGameRequest();

            void newAddonRequest();

            void loadDocumentRequest();

            void closeMessageBox();

            void editSettingsRequest();

        public slots:

            void exitApplication (CSVDoc::View *view);

        private slots:

            void documentStateChanged (int state, CSMDoc::Document *document);

            void progress (int current, int max, int type, int threads, CSMDoc::Document *document);

            void onExitWarningHandler(int state, CSMDoc::Document* document);
    };

}

#endif

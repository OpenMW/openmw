#ifndef CSV_DOC_VIEWMANAGER_H
#define CSV_DOC_VIEWMANAGER_H

#include <vector>

#include <QObject>

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
            std::vector<View *>::iterator mCloseMeOnSaveStateChange;
            bool mUserWarned;

            // not implemented
            ViewManager (const ViewManager&);
            ViewManager& operator= (const ViewManager&);

            void updateIndices();
            bool showModifiedDocumentMessageBox (std::vector<View*>::iterator view);
            bool showSaveInProgressMessageBox (std::vector<View*>::iterator view);

        public:

            ViewManager (CSMDoc::DocumentManager& documentManager);

            virtual ~ViewManager();

            View *addView (CSMDoc::Document *document);
            ///< The ownership of the returned view is not transferred.

            int countViews (const CSMDoc::Document *document) const;
            ///< Return number of views for \a document.

            bool closeRequest (View *view);

        signals:

            void newDocumentRequest();

            void loadDocumentRequest();

            void closeMessageBox();

            void exitApplication();

        private slots:

            void documentStateChanged (int state, CSMDoc::Document *document);

            void progress (int current, int max, int type, int threads, CSMDoc::Document *document);

            void onExitWarningHandler(int state, CSMDoc::Document* document);
    };

}

#endif

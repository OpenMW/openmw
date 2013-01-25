#ifndef CSV_DOC_VIEWMANAGER_H
#define CSV_DOC_VIEWMANAGER_H

#include <vector>

#include <QObject>

namespace CSMDoc
{
    class Document;
    class DocumentManager;
}

namespace CSVDoc
{
    class View;

    class ViewManager : public QObject
    {
            Q_OBJECT

            CSMDoc::DocumentManager& mDocumentManager;
            std::vector<View *> mViews;

            // not implemented
            ViewManager (const ViewManager&);
            ViewManager& operator= (const ViewManager&);

            void updateIndices();

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

        private slots:

            void documentStateChanged (int state, CSMDoc::Document *document);

            void progress (int current, int max, int type, int threads, CSMDoc::Document *document);
    };

}

#endif
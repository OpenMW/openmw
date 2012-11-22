#ifndef CSV_DOC_VIEWMANAGER_H
#define CSV_DOC_VIEWMANAGER_H

#include <vector>

namespace CSMDoc
{
    class Document;
    class DocumentManager;
}

namespace CSVDoc
{
    class View;

    class ViewManager
    {
            CSMDoc::DocumentManager& mDocumentManager;
            std::vector<View *> mViews;
            std::vector<View *> mClosed;

            // not implemented
            ViewManager (const ViewManager&);
            ViewManager& operator= (const ViewManager&);

        public:

            ViewManager (CSMDoc::DocumentManager& documentManager);

            ~ViewManager();

            View *addView (CSMDoc::Document *document);
            ///< The ownership of the returned view is not transferred.

            int countViews (const CSMDoc::Document *document) const;
            ///< Return number of views for \a document.

            bool closeRequest (View *view);



    };

}

#endif
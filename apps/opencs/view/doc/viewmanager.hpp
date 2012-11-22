#ifndef CSV_DOC_VIEWMANAGER_H
#define CSV_DOC_VIEWMANAGER_H

#include <vector>

namespace CSMDoc
{
    class Document;
}

namespace CSVDoc
{
    class View;

    class ViewManager
    {
            std::vector<View *> mViews;

            // not implemented
            ViewManager (const ViewManager&);
            ViewManager& operator= (const ViewManager&);

        public:

            ViewManager();

            ~ViewManager();

            View *addView (CSMDoc::Document *document);
            ///< The ownership of the returned view is not transferred.



    };

}

#endif
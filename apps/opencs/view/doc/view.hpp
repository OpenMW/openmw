#ifndef CSV_DOC_VIEW_H
#define CSV_DOC_VIEW_H

#include <QWidget>

namespace CSMDoc
{
    class Document;
}

namespace CSVDoc
{
    class View : public QWidget
    {
            Q_OBJECT

            CSMDoc::Document *mDocument;

            // not implemented
            View (const View&);
            View& operator= (const View&);

        public:

            View (CSMDoc::Document *document);
            ///< The ownership of \a document is not transferred to *this.
    };
}

#endif
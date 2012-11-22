#ifndef CSV_DOC_VIEW_H
#define CSV_DOC_VIEW_H

#include <QMainWindow>

class QAction;

namespace CSMDoc
{
    class Document;
}

namespace CSVDoc
{
    class ViewManager;

    class View : public QMainWindow
    {
            Q_OBJECT

            ViewManager& mViewManager;
            CSMDoc::Document *mDocument;
            int mViewIndex;
            int mViewTotal;

            // not implemented
            View (const View&);
            View& operator= (const View&);

        private:

            void closeEvent (QCloseEvent *event);

            void setupEditMenu();

            void setupViewMenu();

            void setupUi();

            void updateTitle();

        public:

            View (ViewManager& viewManager, CSMDoc::Document *document, int totalViews);
            ///< The ownership of \a document is not transferred to *this.

            const CSMDoc::Document *getDocument() const;

            CSMDoc::Document *getDocument();

            void setIndex (int viewIndex, int totalViews);

            void updateDocumentState();

        private slots:

            void newView();

            void test();
    };
}

#endif
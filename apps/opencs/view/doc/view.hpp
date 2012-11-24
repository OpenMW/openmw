#ifndef CSV_DOC_VIEW_H
#define CSV_DOC_VIEW_H

#include <vector>

#include <QMainWindow>

class QAction;

namespace CSMDoc
{
    class Document;
}

namespace CSMWorld
{
    class UniversalId;
}

namespace CSVDoc
{
    class ViewManager;
    class Operations;

    class View : public QMainWindow
    {
            Q_OBJECT

            ViewManager& mViewManager;
            CSMDoc::Document *mDocument;
            int mViewIndex;
            int mViewTotal;
            QAction *mUndo;
            QAction *mRedo;
            QAction *mSave;
            QAction *mVerify;
            std::vector<QAction *> mEditingActions;
            Operations *mOperations;

            // not implemented
            View (const View&);
            View& operator= (const View&);

        private:

            void closeEvent (QCloseEvent *event);

            void setupFileMenu();

            void setupEditMenu();

            void setupViewMenu();

            void setupWorldMenu();

            void setupUi();

            void updateTitle();

            void updateActions();

        public:

            View (ViewManager& viewManager, CSMDoc::Document *document, int totalViews);
            ///< The ownership of \a document is not transferred to *this.

            const CSMDoc::Document *getDocument() const;

            CSMDoc::Document *getDocument();

            void setIndex (int viewIndex, int totalViews);

            void updateDocumentState();

            void updateProgress (int current, int max, int type, int threads);

            void addSubView (const CSMWorld::UniversalId& id);

        signals:

            void newDocumentRequest();

        private slots:

            void newView();

            void test();

            void save();

            void verify();

            void addTestSubView(); ///< \todo remove
    };
}

#endif
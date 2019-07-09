#ifndef CSV_DOC_VIEW_H
#define CSV_DOC_VIEW_H

#include <vector>
#include <map>

#include <QMainWindow>

#include "subviewfactory.hpp"

class QAction;
class QDockWidget;
class QScrollArea;

namespace CSMDoc
{
    class Document;
}

namespace CSMWorld
{
    class UniversalId;
}

namespace CSMPrefs
{
    class Setting;
}

namespace CSVDoc
{
    class ViewManager;
    class Operations;
    class GlobalDebugProfileMenu;

    class View : public QMainWindow
    {
            Q_OBJECT

            ViewManager& mViewManager;
            CSMDoc::Document *mDocument;
            int mViewIndex;
            int mViewTotal;
            QList<SubView *> mSubViews;
            QAction *mUndo;
            QAction *mRedo;
            QAction *mSave;
            QAction *mVerify;
            QAction *mShowStatusBar;
            QAction *mStopDebug;
            QAction *mMerge;
            std::vector<QAction *> mEditingActions;
            Operations *mOperations;
            SubViewFactoryManager mSubViewFactory;
            QMainWindow mSubViewWindow;
            GlobalDebugProfileMenu *mGlobalDebugProfileMenu;
            QScrollArea *mScroll;
            bool mScrollbarOnly;


            // not implemented
            View (const View&);
            View& operator= (const View&);

        private:

            void closeEvent (QCloseEvent *event);

            QAction* createMenuEntry(CSMWorld::UniversalId::Type type, QMenu* menu, const char* shortcutName);
            QAction* createMenuEntry(const std::string& title, const std::string& iconName, QMenu* menu, const char* shortcutName);

            void setupFileMenu();

            void setupEditMenu();

            void setupViewMenu();

            void setupWorldMenu();

            void setupMechanicsMenu();

            void setupCharacterMenu();

            void setupAssetsMenu();

            void setupDebugMenu();

            void setupHelpMenu();

            void setupUi();

            void setupShortcut(const char* name, QAction* action);

            void updateActions();

            void exitApplication();

            /// User preference function
            void resizeViewWidth (int width);

            /// User preference function
            void resizeViewHeight (int height);

            void updateScrollbar();
            void updateWidth(bool isGrowLimit, int minSubViewWidth);
            void createScrollArea();
        public:

            View (ViewManager& viewManager, CSMDoc::Document *document, int totalViews);

            ///< The ownership of \a document is not transferred to *this.

            virtual ~View();

            const CSMDoc::Document *getDocument() const;

            CSMDoc::Document *getDocument();

            void setIndex (int viewIndex, int totalViews);

            void updateDocumentState();

            void updateProgress (int current, int max, int type, int threads);

            void toggleStatusBar(bool checked);

            Operations *getOperations() const;

        signals:

            void newGameRequest();

            void newAddonRequest();

            void loadDocumentRequest();

            void exitApplicationRequest (CSVDoc::View *view);

            void editSettingsRequest();

            void mergeDocument (CSMDoc::Document *document);

            void requestFocus (const std::string& id);

        public slots:

            void addSubView (const CSMWorld::UniversalId& id, const std::string& hint = "");
            ///< \param hint Suggested view point (e.g. coordinates in a 3D scene or a line number
            /// in a script).

            void abortOperation (int type);

            void updateTitle();

            // called when subviews are added or removed
            void updateSubViewIndices (SubView *view = nullptr);

        private slots:

            void settingChanged (const CSMPrefs::Setting *setting);

            void undoActionChanged();

            void redoActionChanged();

            void newView();

            void save();

            void exit();

            void infoAbout();

            void infoAboutQt();

            void verify();

            void addGlobalsSubView();

            void addGmstsSubView();

            void addSkillsSubView();

            void addClassesSubView();

            void addFactionsSubView();

            void addRacesSubView();

            void addSoundsSubView();

            void addScriptsSubView();

            void addRegionsSubView();

            void addBirthsignsSubView();

            void addSpellsSubView();

            void addCellsSubView();

            void addReferenceablesSubView();

            void addReferencesSubView();

            void addRegionMapSubView();

            void addFiltersSubView();

            void addTopicsSubView();

            void addJournalsSubView();

            void addTopicInfosSubView();

            void addJournalInfosSubView();

            void addEnchantmentsSubView();

            void addBodyPartsSubView();

            void addSoundGensSubView();

            void addMagicEffectsSubView();

            void addMeshesSubView();

            void addIconsSubView();

            void addMusicsSubView();

            void addSoundsResSubView();

            void addTexturesSubView();

            void addVideosSubView();

            void addDebugProfilesSubView();

            void addRunLogSubView();

            void addLandsSubView();

            void addLandTexturesSubView();

            void addPathgridSubView();

            void addStartScriptsSubView();

            void addSearchSubView();

            void addMetaDataSubView();

            void toggleShowStatusBar (bool show);

            void loadErrorLog();

            void run (const std::string& profile, const std::string& startupInstruction = "");

            void stop();

            void closeRequest (SubView *subView);

            void moveScrollBarToEnd(int min, int max);

            void merge();

            void onRequestFocus (const std::string& id);
    };
}

#endif

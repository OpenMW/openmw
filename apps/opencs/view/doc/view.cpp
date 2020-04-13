#include "view.hpp"

#include <sstream>
#include <stdexcept>

#include <QCloseEvent>
#include <QMenuBar>
#include <QMessageBox>
#include <QMdiArea>
#include <QDockWidget>
#include <QApplication>
#include <QDesktopWidget>
#include <QScrollArea>
#include <QHBoxLayout>
#include <QDesktopWidget>
#include <QScrollBar>

#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
#include <QScreen>
#endif

#include "../../model/doc/document.hpp"
#include "../../model/prefs/state.hpp"
#include "../../model/prefs/shortcut.hpp"

#include "../../model/world/idtable.hpp"

#include "../world/subviews.hpp"
#include "../world/scenesubview.hpp"
#include "../world/tablesubview.hpp"

#include "../tools/subviews.hpp"

#include <components/version/version.hpp>

#include "viewmanager.hpp"
#include "operations.hpp"
#include "subview.hpp"
#include "globaldebugprofilemenu.hpp"
#include "runlogsubview.hpp"
#include "subviewfactoryimp.hpp"

void CSVDoc::View::closeEvent (QCloseEvent *event)
{
    if (!mViewManager.closeRequest (this))
        event->ignore();
    else
    {
        // closeRequest() returns true if last document
        mViewManager.removeDocAndView(mDocument);
    }
}

void CSVDoc::View::setupFileMenu()
{
    QMenu *file = menuBar()->addMenu (tr ("File"));

    QAction* newGame = createMenuEntry("New Game", ":./menu-new-game.png", file, "document-file-newgame");
    connect (newGame, SIGNAL (triggered()), this, SIGNAL (newGameRequest()));

    QAction* newAddon = createMenuEntry("New Addon", ":./menu-new-addon.png", file, "document-file-newaddon");
    connect (newAddon, SIGNAL (triggered()), this, SIGNAL (newAddonRequest()));

    QAction* open = createMenuEntry("Open", ":./menu-open.png", file, "document-file-open");
    connect (open, SIGNAL (triggered()), this, SIGNAL (loadDocumentRequest()));

    QAction* save = createMenuEntry("Save", ":./menu-save.png", file, "document-file-save");
    connect (save, SIGNAL (triggered()), this, SLOT (save()));
    mSave = save;

    QAction* verify = createMenuEntry("Verify", ":./menu-verify.png", file, "document-file-verify");
    connect (verify, SIGNAL (triggered()), this, SLOT (verify()));
    mVerify = verify;

    QAction* merge = createMenuEntry("Merge", ":./menu-merge.png", file, "document-file-merge");
    connect (merge, SIGNAL (triggered()), this, SLOT (merge()));
    mMerge = merge;

    QAction* loadErrors = createMenuEntry("Error Log", ":./error-log.png", file, "document-file-errorlog");
    connect (loadErrors, SIGNAL (triggered()), this, SLOT (loadErrorLog()));

    QAction* meta = createMenuEntry(CSMWorld::UniversalId::Type_MetaDatas, file, "document-file-metadata");
    connect (meta, SIGNAL (triggered()), this, SLOT (addMetaDataSubView()));

    QAction* close = createMenuEntry("Close", ":./menu-close.png", file, "document-file-close");
    connect (close, SIGNAL (triggered()), this, SLOT (close()));

    QAction* exit = createMenuEntry("Exit", ":./menu-exit.png", file, "document-file-exit");
    connect (exit, SIGNAL (triggered()), this, SLOT (exit()));

    connect (this, SIGNAL(exitApplicationRequest(CSVDoc::View *)), &mViewManager, SLOT(exitApplication(CSVDoc::View *)));
}

namespace
{

    void updateUndoRedoAction(QAction *action, const std::string &settingsKey)
    {
        QKeySequence seq;
        CSMPrefs::State::get().getShortcutManager().getSequence(settingsKey, seq);
        action->setShortcut(seq);
    }

}

void CSVDoc::View::undoActionChanged()
{
    updateUndoRedoAction(mUndo, "document-edit-undo");
}

void CSVDoc::View::redoActionChanged()
{
    updateUndoRedoAction(mRedo, "document-edit-redo");
}

void CSVDoc::View::setupEditMenu()
{
    QMenu *edit = menuBar()->addMenu (tr ("Edit"));

    mUndo = mDocument->getUndoStack().createUndoAction (this, tr("Undo"));
    setupShortcut("document-edit-undo", mUndo);
    connect(mUndo, SIGNAL (changed ()), this, SLOT (undoActionChanged ()));
    mUndo->setIcon(QIcon(QString::fromStdString(":./menu-undo.png")));
    edit->addAction (mUndo);

    mRedo = mDocument->getUndoStack().createRedoAction (this, tr("Redo"));
    connect(mRedo, SIGNAL (changed ()), this, SLOT (redoActionChanged ()));
    setupShortcut("document-edit-redo", mRedo);
    mRedo->setIcon(QIcon(QString::fromStdString(":./menu-redo.png")));
    edit->addAction (mRedo);

    QAction* userSettings = createMenuEntry("Preferences", ":./menu-preferences.png", edit, "document-edit-preferences");
    connect (userSettings, SIGNAL (triggered()), this, SIGNAL (editSettingsRequest()));

    QAction* search = createMenuEntry(CSMWorld::UniversalId::Type_Search, edit, "document-edit-search");
    connect (search, SIGNAL (triggered()), this, SLOT (addSearchSubView()));
}

void CSVDoc::View::setupViewMenu()
{
    QMenu *view = menuBar()->addMenu (tr ("View"));

    QAction *newWindow = createMenuEntry("New View", ":./menu-new-window.png", view, "document-view-newview");
    connect (newWindow, SIGNAL (triggered()), this, SLOT (newView()));

    mShowStatusBar = createMenuEntry("Toggle Status Bar", ":./menu-status-bar.png", view, "document-view-statusbar");
    connect (mShowStatusBar, SIGNAL (toggled (bool)), this, SLOT (toggleShowStatusBar (bool)));
    mShowStatusBar->setCheckable (true);
    mShowStatusBar->setChecked (CSMPrefs::get()["Windows"]["show-statusbar"].isTrue());

    view->addAction (mShowStatusBar);

    QAction *filters = createMenuEntry(CSMWorld::UniversalId::Type_Filters, view, "document-mechanics-filters");
    connect (filters, SIGNAL (triggered()), this, SLOT (addFiltersSubView()));
}

void CSVDoc::View::setupWorldMenu()
{
    QMenu *world = menuBar()->addMenu (tr ("World"));

    QAction* regions = createMenuEntry(CSMWorld::UniversalId::Type_Regions, world, "document-world-regions");
    connect (regions, SIGNAL (triggered()), this, SLOT (addRegionsSubView()));

    QAction* cells = createMenuEntry(CSMWorld::UniversalId::Type_Cells, world, "document-world-cells");
    connect (cells, SIGNAL (triggered()), this, SLOT (addCellsSubView()));

    QAction* referenceables = createMenuEntry(CSMWorld::UniversalId::Type_Referenceables, world, "document-world-referencables");
    connect (referenceables, SIGNAL (triggered()), this, SLOT (addReferenceablesSubView()));

    QAction* references = createMenuEntry(CSMWorld::UniversalId::Type_References, world, "document-world-references");
    connect (references, SIGNAL (triggered()), this, SLOT (addReferencesSubView()));

    QAction *lands = createMenuEntry(CSMWorld::UniversalId::Type_Lands, world, "document-world-lands");
    connect (lands, SIGNAL (triggered()), this, SLOT (addLandsSubView()));

    QAction *landTextures = createMenuEntry(CSMWorld::UniversalId::Type_LandTextures, world, "document-world-landtextures");
    connect (landTextures, SIGNAL (triggered()), this, SLOT (addLandTexturesSubView()));

    QAction *grid = createMenuEntry(CSMWorld::UniversalId::Type_Pathgrids, world, "document-world-pathgrid");
    connect (grid, SIGNAL (triggered()), this, SLOT (addPathgridSubView()));

    world->addSeparator(); // items that don't represent single record lists follow here

    QAction *regionMap = createMenuEntry(CSMWorld::UniversalId::Type_RegionMap, world, "document-world-regionmap");
    connect (regionMap, SIGNAL (triggered()), this, SLOT (addRegionMapSubView()));
}

void CSVDoc::View::setupMechanicsMenu()
{
    QMenu *mechanics = menuBar()->addMenu (tr ("Mechanics"));

    QAction* globals = createMenuEntry(CSMWorld::UniversalId::Type_Globals, mechanics, "document-mechanics-globals");
    connect (globals, SIGNAL (triggered()), this, SLOT (addGlobalsSubView()));

    QAction* gmsts = createMenuEntry(CSMWorld::UniversalId::Type_Gmsts, mechanics, "document-mechanics-gamesettings");
    connect (gmsts, SIGNAL (triggered()), this, SLOT (addGmstsSubView()));

    QAction* scripts = createMenuEntry(CSMWorld::UniversalId::Type_Scripts, mechanics, "document-mechanics-scripts");
    connect (scripts, SIGNAL (triggered()), this, SLOT (addScriptsSubView()));

    QAction* spells = createMenuEntry(CSMWorld::UniversalId::Type_Spells, mechanics, "document-mechanics-spells");
    connect (spells, SIGNAL (triggered()), this, SLOT (addSpellsSubView()));

    QAction* enchantments = createMenuEntry(CSMWorld::UniversalId::Type_Enchantments, mechanics, "document-mechanics-enchantments");
    connect (enchantments, SIGNAL (triggered()), this, SLOT (addEnchantmentsSubView()));

    QAction* magicEffects = createMenuEntry(CSMWorld::UniversalId::Type_MagicEffects, mechanics, "document-mechanics-magiceffects");
    connect (magicEffects, SIGNAL (triggered()), this, SLOT (addMagicEffectsSubView()));

    QAction* startScripts = createMenuEntry(CSMWorld::UniversalId::Type_StartScripts, mechanics, "document-mechanics-startscripts");
    connect (startScripts, SIGNAL (triggered()), this, SLOT (addStartScriptsSubView()));
}

void CSVDoc::View::setupCharacterMenu()
{
    QMenu *characters = menuBar()->addMenu (tr ("Characters"));

    QAction* skills = createMenuEntry(CSMWorld::UniversalId::Type_Skills, characters, "document-character-skills");
    connect (skills, SIGNAL (triggered()), this, SLOT (addSkillsSubView()));

    QAction* classes = createMenuEntry(CSMWorld::UniversalId::Type_Classes, characters, "document-character-classes");
    connect (classes, SIGNAL (triggered()), this, SLOT (addClassesSubView()));

    QAction* factions = createMenuEntry(CSMWorld::UniversalId::Type_Faction, characters, "document-character-factions");
    connect (factions, SIGNAL (triggered()), this, SLOT (addFactionsSubView()));

    QAction* races = createMenuEntry(CSMWorld::UniversalId::Type_Races, characters, "document-character-races");
    connect (races, SIGNAL (triggered()), this, SLOT (addRacesSubView()));

    QAction* birthsigns = createMenuEntry(CSMWorld::UniversalId::Type_Birthsigns, characters, "document-character-birthsigns");
    connect (birthsigns, SIGNAL (triggered()), this, SLOT (addBirthsignsSubView()));

    QAction* topics = createMenuEntry(CSMWorld::UniversalId::Type_Topics, characters, "document-character-topics");
    connect (topics, SIGNAL (triggered()), this, SLOT (addTopicsSubView()));

    QAction* journals = createMenuEntry(CSMWorld::UniversalId::Type_Journals, characters, "document-character-journals");
    connect (journals, SIGNAL (triggered()), this, SLOT (addJournalsSubView()));

    QAction* topicInfos = createMenuEntry(CSMWorld::UniversalId::Type_TopicInfos, characters, "document-character-topicinfos");
    connect (topicInfos, SIGNAL (triggered()), this, SLOT (addTopicInfosSubView()));

    QAction* journalInfos = createMenuEntry(CSMWorld::UniversalId::Type_JournalInfos, characters, "document-character-journalinfos");
    connect (journalInfos, SIGNAL (triggered()), this, SLOT (addJournalInfosSubView()));

    QAction* bodyParts = createMenuEntry(CSMWorld::UniversalId::Type_BodyParts, characters, "document-character-bodyparts");
    connect (bodyParts, SIGNAL (triggered()), this, SLOT (addBodyPartsSubView()));
}

void CSVDoc::View::setupAssetsMenu()
{
    QMenu *assets = menuBar()->addMenu (tr ("Assets"));

    QAction* reload = createMenuEntry("Reload", ":./menu-reload.png", assets, "document-assets-reload");
    connect (reload, SIGNAL (triggered()), &mDocument->getData(), SLOT (assetsChanged()));

    assets->addSeparator();

    QAction* sounds = createMenuEntry(CSMWorld::UniversalId::Type_Sounds, assets, "document-assets-sounds");
    connect (sounds, SIGNAL (triggered()), this, SLOT (addSoundsSubView()));

    QAction* soundGens = createMenuEntry(CSMWorld::UniversalId::Type_SoundGens, assets, "document-assets-soundgens");
    connect (soundGens, SIGNAL (triggered()), this, SLOT (addSoundGensSubView()));

    assets->addSeparator(); // resources follow here

    QAction* meshes = createMenuEntry(CSMWorld::UniversalId::Type_Meshes, assets, "document-assets-meshes");
    connect (meshes, SIGNAL (triggered()), this, SLOT (addMeshesSubView()));

    QAction* icons = createMenuEntry(CSMWorld::UniversalId::Type_Icons, assets, "document-assets-icons");
    connect (icons, SIGNAL (triggered()), this, SLOT (addIconsSubView()));

    QAction* musics = createMenuEntry(CSMWorld::UniversalId::Type_Musics, assets, "document-assets-musics");
    connect (musics, SIGNAL (triggered()), this, SLOT (addMusicsSubView()));

    QAction* soundFiles = createMenuEntry(CSMWorld::UniversalId::Type_SoundsRes, assets, "document-assets-soundres");
    connect (soundFiles, SIGNAL (triggered()), this, SLOT (addSoundsResSubView()));

    QAction* textures = createMenuEntry(CSMWorld::UniversalId::Type_Textures, assets, "document-assets-textures");
    connect (textures, SIGNAL (triggered()), this, SLOT (addTexturesSubView()));

    QAction* videos = createMenuEntry(CSMWorld::UniversalId::Type_Videos, assets, "document-assets-videos");
    connect (videos, SIGNAL (triggered()), this, SLOT (addVideosSubView()));
}

void CSVDoc::View::setupDebugMenu()
{
    QMenu *debug = menuBar()->addMenu (tr ("Debug"));

    QAction* profiles = createMenuEntry(CSMWorld::UniversalId::Type_DebugProfiles, debug, "document-debug-profiles");
    connect (profiles, SIGNAL (triggered()), this, SLOT (addDebugProfilesSubView()));

    debug->addSeparator();

    mGlobalDebugProfileMenu = new GlobalDebugProfileMenu (
        &dynamic_cast<CSMWorld::IdTable&> (*mDocument->getData().getTableModel (
        CSMWorld::UniversalId::Type_DebugProfiles)), this);

    connect (mGlobalDebugProfileMenu, SIGNAL (triggered (const std::string&)),
        this, SLOT (run (const std::string&)));

    QAction *runDebug = debug->addMenu (mGlobalDebugProfileMenu);
    runDebug->setText (tr ("Run OpenMW"));
    setupShortcut("document-debug-run", runDebug);
    runDebug->setIcon(QIcon(QString::fromStdString(":./run-openmw.png")));

    QAction* stopDebug = createMenuEntry("Stop OpenMW", ":./stop-openmw.png", debug, "document-debug-shutdown");
    connect (stopDebug, SIGNAL (triggered()), this, SLOT (stop()));
    mStopDebug = stopDebug;

    QAction* runLog = createMenuEntry(CSMWorld::UniversalId::Type_RunLog, debug, "document-debug-runlog");
    connect (runLog, SIGNAL (triggered()), this, SLOT (addRunLogSubView()));
}

void CSVDoc::View::setupHelpMenu()
{
    QMenu *help = menuBar()->addMenu (tr ("Help"));

    QAction* about = createMenuEntry("About OpenMW-CS", ":./info.png", help, "document-help-about");
    connect (about, SIGNAL (triggered()), this, SLOT (infoAbout()));

    QAction* aboutQt = createMenuEntry("About Qt", ":./qt.png", help, "document-help-qt");
    connect (aboutQt, SIGNAL (triggered()), this, SLOT (infoAboutQt()));
}

QAction* CSVDoc::View::createMenuEntry(CSMWorld::UniversalId::Type type, QMenu* menu, const char* shortcutName)
{
    const std::string title = CSMWorld::UniversalId (type).getTypeName();
    QAction *entry = new QAction(QString::fromStdString(title), this);
    setupShortcut(shortcutName, entry);
    const std::string iconName = CSMWorld::UniversalId (type).getIcon();
    if (!iconName.empty() && iconName != ":placeholder")
        entry->setIcon(QIcon(QString::fromStdString(iconName)));

    menu->addAction (entry);

    return entry;
}

QAction* CSVDoc::View::createMenuEntry(const std::string& title, const std::string& iconName, QMenu* menu, const char* shortcutName)
{
    QAction *entry = new QAction(QString::fromStdString(title), this);
    setupShortcut(shortcutName, entry);
    if (!iconName.empty() && iconName != ":placeholder")
        entry->setIcon(QIcon(QString::fromStdString(iconName)));

    menu->addAction (entry);

    return entry;
}

void CSVDoc::View::setupUi()
{
    setupFileMenu();
    setupEditMenu();
    setupViewMenu();
    setupWorldMenu();
    setupMechanicsMenu();
    setupCharacterMenu();
    setupAssetsMenu();
    setupDebugMenu();
    setupHelpMenu();
}

void CSVDoc::View::setupShortcut(const char* name, QAction* action)
{
    CSMPrefs::Shortcut* shortcut = new CSMPrefs::Shortcut(name, this);
    shortcut->associateAction(action);
}

void CSVDoc::View::updateTitle()
{
    std::ostringstream stream;

    stream << mDocument->getSavePath().filename().string();

    if (mDocument->getState() & CSMDoc::State_Modified)
            stream << " *";

    if (mViewTotal>1)
        stream << " [" << (mViewIndex+1) << "/" << mViewTotal << "]";

    CSMPrefs::Category& windows = CSMPrefs::State::get()["Windows"];

    bool hideTitle = windows["hide-subview"].isTrue() &&
        mSubViews.size()==1 && !mSubViews.at (0)->isFloating();

    if (hideTitle)
        stream << " - " << mSubViews.at (0)->getTitle();

    setWindowTitle (QString::fromUtf8(stream.str().c_str()));
}

void CSVDoc::View::updateSubViewIndices(SubView *view)
{
    CSMPrefs::Category& windows = CSMPrefs::State::get()["Windows"];

    if(view && mSubViews.contains(view))
    {
        mSubViews.removeOne(view);

        // adjust (reduce) the scroll area (even floating), except when it is "Scrollbar Only"
        if (windows["mainwindow-scrollbar"].toString() == "Grow then Scroll")
            updateScrollbar();
    }

    bool hideTitle = windows["hide-subview"].isTrue() &&
        mSubViews.size()==1 && !mSubViews.at (0)->isFloating();

    updateTitle();

    for (SubView *subView : mSubViews)
    {
        if (!subView->isFloating())
        {
            if (hideTitle)
            {
                subView->setTitleBarWidget (new QWidget (this));
                subView->setWindowTitle (QString::fromUtf8 (subView->getTitle().c_str()));
            }
            else
            {
                delete subView->titleBarWidget();
                subView->setTitleBarWidget (nullptr);
            }
        }
    }
}

void CSVDoc::View::updateActions()
{
    bool editing = !(mDocument->getState() & CSMDoc::State_Locked);
    bool running = mDocument->getState() & CSMDoc::State_Running;

    for (std::vector<QAction *>::iterator iter (mEditingActions.begin()); iter!=mEditingActions.end(); ++iter)
        (*iter)->setEnabled (editing);

    mUndo->setEnabled (editing & mDocument->getUndoStack().canUndo());
    mRedo->setEnabled (editing & mDocument->getUndoStack().canRedo());

    mSave->setEnabled (!(mDocument->getState() & CSMDoc::State_Saving) && !running);
    mVerify->setEnabled (!(mDocument->getState() & CSMDoc::State_Verifying));

    mGlobalDebugProfileMenu->updateActions (running);
    mStopDebug->setEnabled (running);

    mMerge->setEnabled (mDocument->getContentFiles().size()>1 &&
        !(mDocument->getState() & CSMDoc::State_Merging));
}

CSVDoc::View::View (ViewManager& viewManager, CSMDoc::Document *document, int totalViews)
    : mViewManager (viewManager), mDocument (document), mViewIndex (totalViews-1),
      mViewTotal (totalViews), mScroll(nullptr), mScrollbarOnly(false)
{
    CSMPrefs::Category& windows = CSMPrefs::State::get()["Windows"];

    int width = std::max (windows["default-width"].toInt(), 300);
    int height = std::max (windows["default-height"].toInt(), 300);

    resize (width, height);

    mSubViewWindow.setDockOptions (QMainWindow::AllowNestedDocks);

    if (windows["mainwindow-scrollbar"].toString() == "Grow Only")
    {
        setCentralWidget (&mSubViewWindow);
    }
    else
    {
        createScrollArea();
    }

    mOperations = new Operations;
    addDockWidget (Qt::BottomDockWidgetArea, mOperations);

    setContextMenuPolicy(Qt::NoContextMenu);

    updateTitle();

    setupUi();

    updateActions();

    CSVWorld::addSubViewFactories (mSubViewFactory);
    CSVTools::addSubViewFactories (mSubViewFactory);

    mSubViewFactory.add (CSMWorld::UniversalId::Type_RunLog, new SubViewFactory<RunLogSubView>);

    connect (mOperations, SIGNAL (abortOperation (int)), this, SLOT (abortOperation (int)));

    connect (&CSMPrefs::State::get(), SIGNAL (settingChanged (const CSMPrefs::Setting *)),
        this, SLOT (settingChanged (const CSMPrefs::Setting *)));
}

CSVDoc::View::~View()
{
}

const CSMDoc::Document *CSVDoc::View::getDocument() const
{
        return mDocument;
}

CSMDoc::Document *CSVDoc::View::getDocument()
{
        return mDocument;
}

void CSVDoc::View::setIndex (int viewIndex, int totalViews)
{
    mViewIndex = viewIndex;
    mViewTotal = totalViews;
    updateTitle();
}

void CSVDoc::View::updateDocumentState()
{
    updateTitle();
    updateActions();

    static const int operations[] =
    {
        CSMDoc::State_Saving, CSMDoc::State_Verifying, CSMDoc::State_Searching,
        CSMDoc::State_Merging,
        -1 // end marker
    };

    int state = mDocument->getState() ;

    for (int i=0; operations[i]!=-1; ++i)
        if (!(state & operations[i]))
           mOperations->quitOperation (operations[i]);

    QList<CSVDoc::SubView *> subViews = findChildren<CSVDoc::SubView *>();

    for (QList<CSVDoc::SubView *>::iterator iter (subViews.begin()); iter!=subViews.end(); ++iter)
        (*iter)->setEditLock (state & CSMDoc::State_Locked);
}

void CSVDoc::View::updateProgress (int current, int max, int type, int threads)
{
    mOperations->setProgress (current, max, type, threads);
}

void CSVDoc::View::addSubView (const CSMWorld::UniversalId& id, const std::string& hint)
{
    CSMPrefs::Category& windows = CSMPrefs::State::get()["Windows"];

    bool isReferenceable = id.getClass() == CSMWorld::UniversalId::Class_RefRecord;

    // User setting to reuse sub views (on a per top level view basis)
    if (windows["reuse"].isTrue())
    {
        for (SubView *sb : mSubViews)
        {
            bool isSubViewReferenceable =
                sb->getUniversalId().getType() == CSMWorld::UniversalId::Type_Referenceable;

            if((isReferenceable && isSubViewReferenceable && id.getId() == sb->getUniversalId().getId())
               ||
               (!isReferenceable && id == sb->getUniversalId()))
            {
                sb->setFocus();
                if (!hint.empty())
                    sb->useHint (hint);
                return;
            }
        }
    }

    if (mScroll)
        QObject::connect(mScroll->horizontalScrollBar(),
            SIGNAL(rangeChanged(int,int)), this, SLOT(moveScrollBarToEnd(int,int)));

    // User setting for limiting the number of sub views per top level view.
    // Automatically open a new top level view if this number is exceeded
    //
    // If the sub view limit setting is one, the sub view title bar is hidden and the
    // text in the main title bar is adjusted accordingly
    if(mSubViews.size() >= windows["max-subviews"].toInt()) // create a new top level view
    {
        mViewManager.addView(mDocument, id, hint);

        return;
    }

    SubView *view = nullptr;
    if(isReferenceable)
    {
        view = mSubViewFactory.makeSubView (CSMWorld::UniversalId(CSMWorld::UniversalId::Type_Referenceable, id.getId()), *mDocument);
    }
    else
    {
        view = mSubViewFactory.makeSubView (id, *mDocument);
    }
    assert(view);
    view->setParent(this);
    view->setEditLock (mDocument->getState() & CSMDoc::State_Locked);
    mSubViews.append(view); // only after assert

    int minWidth = windows["minimum-width"].toInt();
    view->setMinimumWidth (minWidth);

    view->setStatusBar (mShowStatusBar->isChecked());

    // Work out how to deal with additional subviews
    //
    // Policy for "Grow then Scroll":
    //
    // - Increase the horizontal width of the mainwindow until it becomes greater than or equal
    //   to the screen (monitor) width.
    // - Move the mainwindow position sideways if necessary to fit within the screen.
    // - Any more additions increases the size of the mSubViewWindow (horizontal scrollbar
    //   should become visible)
    // - Move the scroll bar to the newly added subview
    //
    mScrollbarOnly = windows["mainwindow-scrollbar"].toString() == "Scrollbar Only";

    updateWidth(windows["grow-limit"].isTrue(), minWidth);

    mSubViewWindow.addDockWidget (Qt::TopDockWidgetArea, view);

    updateSubViewIndices();

    connect (view, SIGNAL (focusId (const CSMWorld::UniversalId&, const std::string&)), this,
        SLOT (addSubView (const CSMWorld::UniversalId&, const std::string&)));

    connect (view, SIGNAL (closeRequest (SubView *)), this, SLOT (closeRequest (SubView *)));

    connect (view, SIGNAL (updateTitle()), this, SLOT (updateTitle()));

    connect (view, SIGNAL (updateSubViewIndices (SubView *)),
        this, SLOT (updateSubViewIndices (SubView *)));

    CSVWorld::TableSubView* tableView = dynamic_cast<CSVWorld::TableSubView*>(view);
    if (tableView)
    {
        connect (this, SIGNAL (requestFocus (const std::string&)),
            tableView, SLOT (requestFocus (const std::string&)));
    }

    CSVWorld::SceneSubView* sceneView = dynamic_cast<CSVWorld::SceneSubView*>(view);
    if (sceneView)
    {
        connect(sceneView, SIGNAL(requestFocus(const std::string&)),
                this, SLOT(onRequestFocus(const std::string&)));
    }

    view->show();

    if (!hint.empty())
        view->useHint (hint);
}

void CSVDoc::View::moveScrollBarToEnd(int min, int max)
{
    if (mScroll)
    {
        mScroll->horizontalScrollBar()->setValue(max);

        QObject::disconnect(mScroll->horizontalScrollBar(),
            SIGNAL(rangeChanged(int,int)), this, SLOT(moveScrollBarToEnd(int,int)));
    }
}

void CSVDoc::View::settingChanged (const CSMPrefs::Setting *setting)
{
    if (*setting=="Windows/hide-subview")
        updateSubViewIndices (nullptr);
    else if (*setting=="Windows/mainwindow-scrollbar")
    {
        if (setting->toString()!="Grow Only")
        {
            if (mScroll)
            {
                if (setting->toString()=="Scrollbar Only")
                {
                    mScrollbarOnly = true;
                    mSubViewWindow.setMinimumWidth(0);
                }
                else if (mScrollbarOnly)
                {
                    mScrollbarOnly = false;
                    updateScrollbar();
                }
            }
            else
            {
                createScrollArea();
            }
        }
        else  if (mScroll)
        {
            mScroll->takeWidget();
            setCentralWidget (&mSubViewWindow);
            mScroll->deleteLater();
            mScroll = nullptr;
        }
    }
}

void CSVDoc::View::newView()
{
    mViewManager.addView (mDocument);
}

void CSVDoc::View::save()
{
    mDocument->save();
}

void CSVDoc::View::infoAbout()
{
    // Get current OpenMW version
    QString versionInfo = (Version::getOpenmwVersionDescription(mDocument->getResourceDir().string())+
#if defined(__x86_64__) || defined(_M_X64)
    " (64-bit)").c_str();
#else
    " (32-bit)").c_str();
#endif

    // Get current year
    time_t now = time(NULL);
    struct tm tstruct;
    char copyrightInfo[40];
    tstruct = *localtime(&now);
    strftime(copyrightInfo, sizeof(copyrightInfo), "Copyright © 2008-%Y OpenMW Team", &tstruct);

    QString aboutText = QString(
    "<p style=\"white-space: pre-wrap;\">"
    "<b><h2>OpenMW Construction Set</h2></b>"
    "%1\n\n"
    "%2\n\n"
    "%3\n\n"
    "<table>"
    "<tr><td>%4</td><td><a href=\"https://openmw.org\">https://openmw.org</a></td></tr>"
    "<tr><td>%5</td><td><a href=\"https://forum.openmw.org\">https://forum.openmw.org</a></td></tr>"
    "<tr><td>%6</td><td><a href=\"https://gitlab.com/OpenMW/openmw/issues\">https://gitlab.com/OpenMW/openmw/issues</a></td></tr>"
    "<tr><td>%7</td><td><a href=\"https://webchat.freenode.net/?channels=openmw&uio=OT10cnVlde\">irc://irc.freenode.net/#openmw</a></td></tr>"
    "</table>"
    "</p>")
    .arg(versionInfo
        , tr("OpenMW-CS is a content file editor for OpenMW, a modern, free and open source game engine.")
        , tr(copyrightInfo)
        , tr("Home Page:")
        , tr("Forum:")
        , tr("Bug Tracker:")
        , tr("IRC:"));

    QMessageBox::about(this, "About OpenMW-CS", aboutText);
}

void CSVDoc::View::infoAboutQt()
{
    QMessageBox::aboutQt(this);
}

void CSVDoc::View::verify()
{
    addSubView (mDocument->verify());
}

void CSVDoc::View::addGlobalsSubView()
{
    addSubView (CSMWorld::UniversalId::Type_Globals);
}

void CSVDoc::View::addGmstsSubView()
{
    addSubView (CSMWorld::UniversalId::Type_Gmsts);
}

void CSVDoc::View::addSkillsSubView()
{
    addSubView (CSMWorld::UniversalId::Type_Skills);
}

void CSVDoc::View::addClassesSubView()
{
    addSubView (CSMWorld::UniversalId::Type_Classes);
}

void CSVDoc::View::addFactionsSubView()
{
    addSubView (CSMWorld::UniversalId::Type_Factions);
}

void CSVDoc::View::addRacesSubView()
{
    addSubView (CSMWorld::UniversalId::Type_Races);
}

void CSVDoc::View::addSoundsSubView()
{
    addSubView (CSMWorld::UniversalId::Type_Sounds);
}

void CSVDoc::View::addScriptsSubView()
{
    addSubView (CSMWorld::UniversalId::Type_Scripts);
}

void CSVDoc::View::addRegionsSubView()
{
    addSubView (CSMWorld::UniversalId::Type_Regions);
}

void CSVDoc::View::addBirthsignsSubView()
{
    addSubView (CSMWorld::UniversalId::Type_Birthsigns);
}

void CSVDoc::View::addSpellsSubView()
{
    addSubView (CSMWorld::UniversalId::Type_Spells);
}

void CSVDoc::View::addCellsSubView()
{
    addSubView (CSMWorld::UniversalId::Type_Cells);
}

void CSVDoc::View::addReferenceablesSubView()
{
    addSubView (CSMWorld::UniversalId::Type_Referenceables);
}

void CSVDoc::View::addReferencesSubView()
{
    addSubView (CSMWorld::UniversalId::Type_References);
}

void CSVDoc::View::addRegionMapSubView()
{
    addSubView (CSMWorld::UniversalId::Type_RegionMap);
}

void CSVDoc::View::addFiltersSubView()
{
    addSubView (CSMWorld::UniversalId::Type_Filters);
}

void CSVDoc::View::addTopicsSubView()
{
    addSubView (CSMWorld::UniversalId::Type_Topics);
}

void CSVDoc::View::addJournalsSubView()
{
    addSubView (CSMWorld::UniversalId::Type_Journals);
}

void CSVDoc::View::addTopicInfosSubView()
{
    addSubView (CSMWorld::UniversalId::Type_TopicInfos);
}

void CSVDoc::View::addJournalInfosSubView()
{
    addSubView (CSMWorld::UniversalId::Type_JournalInfos);
}

void CSVDoc::View::addEnchantmentsSubView()
{
    addSubView (CSMWorld::UniversalId::Type_Enchantments);
}

void CSVDoc::View::addBodyPartsSubView()
{
    addSubView (CSMWorld::UniversalId::Type_BodyParts);
}

void CSVDoc::View::addSoundGensSubView()
{
    addSubView (CSMWorld::UniversalId::Type_SoundGens);
}

void CSVDoc::View::addMeshesSubView()
{
    addSubView (CSMWorld::UniversalId::Type_Meshes);
}

void CSVDoc::View::addIconsSubView()
{
    addSubView (CSMWorld::UniversalId::Type_Icons);
}

void CSVDoc::View::addMusicsSubView()
{
    addSubView (CSMWorld::UniversalId::Type_Musics);
}

void CSVDoc::View::addSoundsResSubView()
{
    addSubView (CSMWorld::UniversalId::Type_SoundsRes);
}

void CSVDoc::View::addMagicEffectsSubView()
{
    addSubView (CSMWorld::UniversalId::Type_MagicEffects);
}

void CSVDoc::View::addTexturesSubView()
{
    addSubView (CSMWorld::UniversalId::Type_Textures);
}

void CSVDoc::View::addVideosSubView()
{
    addSubView (CSMWorld::UniversalId::Type_Videos);
}

void CSVDoc::View::addDebugProfilesSubView()
{
    addSubView (CSMWorld::UniversalId::Type_DebugProfiles);
}

void CSVDoc::View::addRunLogSubView()
{
    addSubView (CSMWorld::UniversalId::Type_RunLog);
}

void CSVDoc::View::addLandsSubView()
{
    addSubView (CSMWorld::UniversalId::Type_Lands);
}

void CSVDoc::View::addLandTexturesSubView()
{
    addSubView (CSMWorld::UniversalId::Type_LandTextures);
}

void CSVDoc::View::addPathgridSubView()
{
    addSubView (CSMWorld::UniversalId::Type_Pathgrids);
}

void CSVDoc::View::addStartScriptsSubView()
{
    addSubView (CSMWorld::UniversalId::Type_StartScripts);
}

void CSVDoc::View::addSearchSubView()
{
    addSubView (mDocument->newSearch());
}

void CSVDoc::View::addMetaDataSubView()
{
    addSubView (CSMWorld::UniversalId (CSMWorld::UniversalId::Type_MetaData, "sys::meta"));
}

void CSVDoc::View::abortOperation (int type)
{
    mDocument->abortOperation (type);
    updateActions();
}

CSVDoc::Operations *CSVDoc::View::getOperations() const
{
    return mOperations;
}

void CSVDoc::View::exit()
{
    emit exitApplicationRequest (this);
}

void CSVDoc::View::resizeViewWidth (int width)
{
    if (width >= 0)
        resize (width, geometry().height());
}

void CSVDoc::View::resizeViewHeight (int height)
{
    if (height >= 0)
        resize (geometry().width(), height);
}

void CSVDoc::View::toggleShowStatusBar (bool show)
{
    for (QObject *view : mSubViewWindow.children())
    {
        if (CSVDoc::SubView *subView = dynamic_cast<CSVDoc::SubView *> (view))
            subView->setStatusBar (show);
    }
}

void CSVDoc::View::toggleStatusBar(bool checked)
{
    mShowStatusBar->setChecked(checked);
}

void CSVDoc::View::loadErrorLog()
{
    addSubView (CSMWorld::UniversalId (CSMWorld::UniversalId::Type_LoadErrorLog, 0));
}

void CSVDoc::View::run (const std::string& profile, const std::string& startupInstruction)
{
    mDocument->startRunning (profile, startupInstruction);
}

void CSVDoc::View::stop()
{
    mDocument->stopRunning();
}

void CSVDoc::View::closeRequest (SubView *subView)
{
    CSMPrefs::Category& windows = CSMPrefs::State::get()["Windows"];

    if (mSubViews.size()>1 || mViewTotal<=1 || !windows["hide-subview"].isTrue())
    {
        subView->deleteLater();
        mSubViews.removeOne (subView);
    }
    else if (mViewManager.closeRequest (this))
        mViewManager.removeDocAndView (mDocument);
}

void CSVDoc::View::updateScrollbar()
{
    QRect rect;
    QWidget *topLevel = QApplication::topLevelAt(pos());
    if (topLevel)
        rect = topLevel->rect();
    else
        rect = this->rect();

    int newWidth = 0;
    for (int i = 0; i < mSubViews.size(); ++i)
    {
        newWidth += mSubViews[i]->width();
    }

    int frameWidth = frameGeometry().width() - width();

    if ((newWidth+frameWidth) >= rect.width())
        mSubViewWindow.setMinimumWidth(newWidth);
    else
        mSubViewWindow.setMinimumWidth(0);
}

void CSVDoc::View::merge()
{
    emit mergeDocument (mDocument);
}

void CSVDoc::View::updateWidth(bool isGrowLimit, int minSubViewWidth)
{
    QDesktopWidget *dw = QApplication::desktop();
    QRect rect;
    if (isGrowLimit)
        rect = dw->screenGeometry(this);
    else
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
        rect = QGuiApplication::screens().at(dw->screenNumber(this))->geometry();
#else
        rect = dw->screenGeometry(dw->screen(dw->screenNumber(this)));
#endif

    if (!mScrollbarOnly && mScroll && mSubViews.size() > 1)
    {
        int newWidth = width()+minSubViewWidth;
        int frameWidth = frameGeometry().width() - width();
        if (newWidth+frameWidth <= rect.width())
        {
            resize(newWidth, height());
            // WARNING: below code assumes that new subviews are added to the right
            if (x() > rect.width()-(newWidth+frameWidth))
                move(rect.width()-(newWidth+frameWidth), y()); // shift left to stay within the screen
        }
        else
        {
            // full width
            resize(rect.width()-frameWidth, height());
            mSubViewWindow.setMinimumWidth(mSubViewWindow.width()+minSubViewWidth);
            move(0, y());
        }
    }
}

void CSVDoc::View::createScrollArea()
{
    mScroll = new QScrollArea(this);
    mScroll->setWidgetResizable(true);
    mScroll->setWidget(&mSubViewWindow);
    setCentralWidget(mScroll);
}

void CSVDoc::View::onRequestFocus (const std::string& id)
{
    if(CSMPrefs::get()["3D Scene Editing"]["open-list-view"].isTrue())
    {
        addReferencesSubView();
        emit requestFocus(id);
    }
    else
    {
        addSubView(CSMWorld::UniversalId (CSMWorld::UniversalId::Type_Reference, id));
    }
}

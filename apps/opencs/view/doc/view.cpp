#include "view.hpp"

#include <sstream>
#include <stdexcept>

#include <QCloseEvent>
#include <QMenuBar>
#include <QMdiArea>
#include <QDockWidget>
#include <QApplication>
#include <QDesktopWidget>
#include <QScrollArea>
#include <QHBoxLayout>
#include <QDesktopWidget>
#include <QScrollBar>

#include "../../model/doc/document.hpp"
#include "../../model/prefs/state.hpp"
#include "../../model/prefs/shortcut.hpp"

#include "../../model/world/idtable.hpp"

#include "../world/subviews.hpp"
#include "../world/tablesubview.hpp"

#include "../tools/subviews.hpp"

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

    QAction *newGame = new QAction (tr ("New Game"), this);
    connect (newGame, SIGNAL (triggered()), this, SIGNAL (newGameRequest()));
    setupShortcut("document-file-newgame", newGame);
    file->addAction (newGame);


    QAction *newAddon = new QAction (tr ("New Addon"), this);
    connect (newAddon, SIGNAL (triggered()), this, SIGNAL (newAddonRequest()));
    setupShortcut("document-file-newaddon", newAddon);
    file->addAction (newAddon);

    QAction *open = new QAction (tr ("Open"), this);
    connect (open, SIGNAL (triggered()), this, SIGNAL (loadDocumentRequest()));
    setupShortcut("document-file-open", open);
    file->addAction (open);

    mSave = new QAction (tr ("Save"), this);
    connect (mSave, SIGNAL (triggered()), this, SLOT (save()));
    setupShortcut("document-file-save", mSave);
    file->addAction (mSave);

    mVerify = new QAction (tr ("Verify"), this);
    connect (mVerify, SIGNAL (triggered()), this, SLOT (verify()));
    setupShortcut("document-file-verify", mVerify);
    file->addAction (mVerify);

    mMerge = new QAction (tr ("Merge"), this);
    connect (mMerge, SIGNAL (triggered()), this, SLOT (merge()));
    setupShortcut("document-file-merge", mMerge);
    file->addAction (mMerge);

    QAction *loadErrors = new QAction (tr ("Open Load Error Log"), this);
    connect (loadErrors, SIGNAL (triggered()), this, SLOT (loadErrorLog()));
    setupShortcut("document-file-errorlog", loadErrors);
    file->addAction (loadErrors);

    QAction *meta = new QAction (tr ("Meta Data"), this);
    connect (meta, SIGNAL (triggered()), this, SLOT (addMetaDataSubView()));
    setupShortcut("document-file-metadata", meta);
    file->addAction (meta);

    QAction *close = new QAction (tr ("Close Document"), this);
    connect (close, SIGNAL (triggered()), this, SLOT (close()));
    setupShortcut("document-file-close", close);
    file->addAction(close);

    QAction *exit = new QAction (tr ("Exit Application"), this);
    connect (exit, SIGNAL (triggered()), this, SLOT (exit()));
    connect (this, SIGNAL(exitApplicationRequest(CSVDoc::View *)), &mViewManager, SLOT(exitApplication(CSVDoc::View *)));
    setupShortcut("document-file-exit", exit);

    file->addAction(exit);
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
    edit->addAction (mUndo);

    mRedo = mDocument->getUndoStack().createRedoAction (this, tr("Redo"));
    connect(mRedo, SIGNAL (changed ()), this, SLOT (redoActionChanged ()));
    setupShortcut("document-edit-redo", mRedo);
    edit->addAction (mRedo);

    QAction *userSettings = new QAction (tr ("Preferences"), this);
    connect (userSettings, SIGNAL (triggered()), this, SIGNAL (editSettingsRequest()));
    setupShortcut("document-edit-preferences", userSettings);
    edit->addAction (userSettings);

    QAction *search = new QAction (tr ("Search"), this);
    connect (search, SIGNAL (triggered()), this, SLOT (addSearchSubView()));
    setupShortcut("document-edit-search", search);
    edit->addAction (search);
}

void CSVDoc::View::setupViewMenu()
{
    QMenu *view = menuBar()->addMenu (tr ("View"));

    QAction *newWindow = new QAction (tr ("New View"), this);
    connect (newWindow, SIGNAL (triggered()), this, SLOT (newView()));
    setupShortcut("document-view-newview", newWindow);
    view->addAction (newWindow);

    mShowStatusBar = new QAction (tr ("Toggle Status Bar"), this);
    mShowStatusBar->setCheckable (true);
    connect (mShowStatusBar, SIGNAL (toggled (bool)), this, SLOT (toggleShowStatusBar (bool)));
    setupShortcut("document-view-statusbar", mShowStatusBar);

    mShowStatusBar->setChecked (CSMPrefs::get()["Windows"]["show-statusbar"].isTrue());

    view->addAction (mShowStatusBar);

    QAction *filters = new QAction (tr ("Filters"), this);
    connect (filters, SIGNAL (triggered()), this, SLOT (addFiltersSubView()));
    setupShortcut("document-view-filters", filters);
    view->addAction (filters);
}

void CSVDoc::View::setupWorldMenu()
{
    QMenu *world = menuBar()->addMenu (tr ("World"));

    QAction *regions = new QAction (tr ("Regions"), this);
    connect (regions, SIGNAL (triggered()), this, SLOT (addRegionsSubView()));
    setupShortcut("document-world-regions", regions);
    world->addAction (regions);

    QAction *cells = new QAction (tr ("Cells"), this);
    connect (cells, SIGNAL (triggered()), this, SLOT (addCellsSubView()));
    setupShortcut("document-world-cells", cells);
    world->addAction (cells);

    QAction *referenceables = new QAction (tr ("Objects"), this);
    connect (referenceables, SIGNAL (triggered()), this, SLOT (addReferenceablesSubView()));
    setupShortcut("document-world-referencables", referenceables);
    world->addAction (referenceables);

    QAction *references = new QAction (tr ("Instances"), this);
    connect (references, SIGNAL (triggered()), this, SLOT (addReferencesSubView()));
    setupShortcut("document-world-references", references);
    world->addAction (references);

    QAction *lands = new QAction (tr ("Lands"), this);
    connect (lands, SIGNAL (triggered()), this, SLOT (addLandsSubView()));
    setupShortcut("document-world-lands", lands);
    world->addAction (lands);

    QAction *landTextures = new QAction (tr ("Land Textures"), this);
    connect (landTextures, SIGNAL (triggered()), this, SLOT (addLandTexturesSubView()));
    setupShortcut("document-world-landtextures", landTextures);
    world->addAction (landTextures);

    QAction *grid = new QAction (tr ("Pathgrid"), this);
    connect (grid, SIGNAL (triggered()), this, SLOT (addPathgridSubView()));
    setupShortcut("document-world-pathgrid", grid);
    world->addAction (grid);

    world->addSeparator(); // items that don't represent single record lists follow here

    QAction *regionMap = new QAction (tr ("Region Map"), this);
    connect (regionMap, SIGNAL (triggered()), this, SLOT (addRegionMapSubView()));
    setupShortcut("document-world-regionmap", regionMap);
    world->addAction (regionMap);
}

void CSVDoc::View::setupMechanicsMenu()
{
    QMenu *mechanics = menuBar()->addMenu (tr ("Mechanics"));

    QAction *globals = new QAction (tr ("Globals"), this);
    connect (globals, SIGNAL (triggered()), this, SLOT (addGlobalsSubView()));
    setupShortcut("document-mechanics-globals", globals);
    mechanics->addAction (globals);

    QAction *gmsts = new QAction (tr ("Game Settings"), this);
    connect (gmsts, SIGNAL (triggered()), this, SLOT (addGmstsSubView()));
    setupShortcut("document-mechanics-gamesettings", gmsts);
    mechanics->addAction (gmsts);

    QAction *scripts = new QAction (tr ("Scripts"), this);
    connect (scripts, SIGNAL (triggered()), this, SLOT (addScriptsSubView()));
    setupShortcut("document-mechanics-scripts", scripts);
    mechanics->addAction (scripts);

    QAction *spells = new QAction (tr ("Spells"), this);
    connect (spells, SIGNAL (triggered()), this, SLOT (addSpellsSubView()));
    setupShortcut("document-mechanics-spells", spells);
    mechanics->addAction (spells);

    QAction *enchantments = new QAction (tr ("Enchantments"), this);
    connect (enchantments, SIGNAL (triggered()), this, SLOT (addEnchantmentsSubView()));
    setupShortcut("document-mechanics-enchantments", enchantments);
    mechanics->addAction (enchantments);

    QAction *effects = new QAction (tr ("Magic Effects"), this);
    connect (effects, SIGNAL (triggered()), this, SLOT (addMagicEffectsSubView()));
    setupShortcut("document-mechanics-magiceffects", effects);
    mechanics->addAction (effects);

    QAction *startScripts = new QAction (tr ("Start Scripts"), this);
    connect (startScripts, SIGNAL (triggered()), this, SLOT (addStartScriptsSubView()));
    setupShortcut("document-mechanics-startscripts", startScripts);
    mechanics->addAction (startScripts);
}

void CSVDoc::View::setupCharacterMenu()
{
    QMenu *characters = menuBar()->addMenu (tr ("Characters"));

    QAction *skills = new QAction (tr ("Skills"), this);
    connect (skills, SIGNAL (triggered()), this, SLOT (addSkillsSubView()));
    setupShortcut("document-character-skills", skills);
    characters->addAction (skills);

    QAction *classes = new QAction (tr ("Classes"), this);
    connect (classes, SIGNAL (triggered()), this, SLOT (addClassesSubView()));
    setupShortcut("document-character-classes", classes);
    characters->addAction (classes);

    QAction *factions = new QAction (tr ("Factions"), this);
    connect (factions, SIGNAL (triggered()), this, SLOT (addFactionsSubView()));
    setupShortcut("document-character-factions", factions);
    characters->addAction (factions);

    QAction *races = new QAction (tr ("Races"), this);
    connect (races, SIGNAL (triggered()), this, SLOT (addRacesSubView()));
    setupShortcut("document-character-races", races);
    characters->addAction (races);

    QAction *birthsigns = new QAction (tr ("Birthsigns"), this);
    connect (birthsigns, SIGNAL (triggered()), this, SLOT (addBirthsignsSubView()));
    setupShortcut("document-character-birthsigns", birthsigns);
    characters->addAction (birthsigns);

    QAction *topics = new QAction (tr ("Topics"), this);
    connect (topics, SIGNAL (triggered()), this, SLOT (addTopicsSubView()));
    setupShortcut("document-character-topics", topics);
    characters->addAction (topics);

    QAction *journals = new QAction (tr ("Journals"), this);
    connect (journals, SIGNAL (triggered()), this, SLOT (addJournalsSubView()));
    setupShortcut("document-character-journals", journals);
    characters->addAction (journals);

    QAction *topicInfos = new QAction (tr ("Topic Infos"), this);
    connect (topicInfos, SIGNAL (triggered()), this, SLOT (addTopicInfosSubView()));
    setupShortcut("document-character-topicinfos", topicInfos);
    characters->addAction (topicInfos);

    QAction *journalInfos = new QAction (tr ("Journal Infos"), this);
    connect (journalInfos, SIGNAL (triggered()), this, SLOT (addJournalInfosSubView()));
    setupShortcut("document-character-journalinfos", journalInfos);
    characters->addAction (journalInfos);

    QAction *bodyParts = new QAction (tr ("Body Parts"), this);
    connect (bodyParts, SIGNAL (triggered()), this, SLOT (addBodyPartsSubView()));
    setupShortcut("document-character-bodyparts", bodyParts);
    characters->addAction (bodyParts);
}

void CSVDoc::View::setupAssetsMenu()
{
    QMenu *assets = menuBar()->addMenu (tr ("Assets"));

    QAction *reload = new QAction (tr ("Reload"), this);
    connect (reload, SIGNAL (triggered()), &mDocument->getData(), SLOT (assetsChanged()));
    setupShortcut("document-assets-reload", reload);
    assets->addAction (reload);

    assets->addSeparator();

    QAction *sounds = new QAction (tr ("Sounds"), this);
    connect (sounds, SIGNAL (triggered()), this, SLOT (addSoundsSubView()));
    setupShortcut("document-assets-sounds", sounds);
    assets->addAction (sounds);

    QAction *soundGens = new QAction (tr ("Sound Generators"), this);
    connect (soundGens, SIGNAL (triggered()), this, SLOT (addSoundGensSubView()));
    setupShortcut("document-assets-soundgens", soundGens);
    assets->addAction (soundGens);

    assets->addSeparator(); // resources follow here

    QAction *meshes = new QAction (tr ("Meshes"), this);
    connect (meshes, SIGNAL (triggered()), this, SLOT (addMeshesSubView()));
    setupShortcut("document-assets-meshes", meshes);
    assets->addAction (meshes);

    QAction *icons = new QAction (tr ("Icons"), this);
    connect (icons, SIGNAL (triggered()), this, SLOT (addIconsSubView()));
    setupShortcut("document-assets-icons", icons);
    assets->addAction (icons);

    QAction *musics = new QAction (tr ("Music"), this);
    connect (musics, SIGNAL (triggered()), this, SLOT (addMusicsSubView()));
    setupShortcut("document-assets-music", musics);
    assets->addAction (musics);

    QAction *soundsRes = new QAction (tr ("Sound Files"), this);
    connect (soundsRes, SIGNAL (triggered()), this, SLOT (addSoundsResSubView()));
    setupShortcut("document-assets-soundres", soundsRes);
    assets->addAction (soundsRes);

    QAction *textures = new QAction (tr ("Textures"), this);
    connect (textures, SIGNAL (triggered()), this, SLOT (addTexturesSubView()));
    setupShortcut("document-assets-textures", textures);
    assets->addAction (textures);

    QAction *videos = new QAction (tr ("Videos"), this);
    connect (videos, SIGNAL (triggered()), this, SLOT (addVideosSubView()));
    setupShortcut("document-assets-videos", videos);
    assets->addAction (videos);
}

void CSVDoc::View::setupDebugMenu()
{
    QMenu *debug = menuBar()->addMenu (tr ("Debug"));

    QAction *profiles = new QAction (tr ("Debug Profiles"), this);
    connect (profiles, SIGNAL (triggered()), this, SLOT (addDebugProfilesSubView()));
    debug->addAction (profiles);

    debug->addSeparator();

    mGlobalDebugProfileMenu = new GlobalDebugProfileMenu (
        &dynamic_cast<CSMWorld::IdTable&> (*mDocument->getData().getTableModel (
        CSMWorld::UniversalId::Type_DebugProfiles)), this);

    connect (mGlobalDebugProfileMenu, SIGNAL (triggered (const std::string&)),
        this, SLOT (run (const std::string&)));

    QAction *runDebug = debug->addMenu (mGlobalDebugProfileMenu);
    runDebug->setText (tr ("Run OpenMW"));

    setupShortcut("document-debug-run", runDebug);

    mStopDebug = new QAction (tr ("Shutdown OpenMW"), this);
    connect (mStopDebug, SIGNAL (triggered()), this, SLOT (stop()));
    setupShortcut("document-debug-shutdown", mStopDebug);
    debug->addAction (mStopDebug);

    QAction *runLog = new QAction (tr ("Open Run Log"), this);
    connect (runLog, SIGNAL (triggered()), this, SLOT (addRunLogSubView()));
    setupShortcut("document-debug-runlog", runLog);
    debug->addAction (runLog);
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

    foreach (SubView *subView, mSubViews)
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
                subView->setTitleBarWidget (NULL);
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
      mViewTotal (totalViews), mScroll(NULL), mScrollbarOnly(false)
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
        foreach(SubView *sb, mSubViews)
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

    SubView *view = NULL;
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
        updateSubViewIndices (NULL);
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
            mScroll = NULL;
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
    foreach (QObject *view, mSubViewWindow.children())
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
        rect = dw->screenGeometry(dw->screen(dw->screenNumber(this)));

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

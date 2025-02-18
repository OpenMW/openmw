#include "view.hpp"

#include <QApplication>
#include <QCloseEvent>
#include <QMenuBar>
#include <QMessageBox>
#include <QScreen>
#include <QScrollArea>
#include <QScrollBar>

#include <algorithm>
#include <filesystem>
#include <sstream>

#include "../../model/doc/document.hpp"
#include "../../model/doc/state.hpp"

#include "../../model/prefs/shortcut.hpp"
#include "../../model/prefs/state.hpp"

#include "../../model/world/idtable.hpp"

#include "../world/dialoguesubview.hpp"
#include "../world/scenesubview.hpp"
#include "../world/scriptsubview.hpp"
#include "../world/subviews.hpp"
#include "../world/tablesubview.hpp"

#include "../tools/subviews.hpp"

#include <apps/opencs/model/prefs/category.hpp>
#include <apps/opencs/model/prefs/setting.hpp>
#include <apps/opencs/model/prefs/shortcutmanager.hpp>
#include <apps/opencs/model/world/data.hpp>
#include <apps/opencs/view/doc/subviewfactory.hpp>

#include <components/files/conversion.hpp>
#include <components/misc/helpviewer.hpp>
#include <components/misc/scalableicon.hpp>
#include <components/misc/strings/format.hpp>
#include <components/misc/timeconvert.hpp>
#include <components/version/version.hpp>

#include "globaldebugprofilemenu.hpp"
#include "operations.hpp"
#include "runlogsubview.hpp"
#include "subview.hpp"
#include "subviewfactoryimp.hpp"
#include "viewmanager.hpp"

QRect desktopRect()
{
    QRegion virtualGeometry;
    for (auto screen : QGuiApplication::screens())
    {
        virtualGeometry += screen->geometry();
    }
    return virtualGeometry.boundingRect();
}

void CSVDoc::View::closeEvent(QCloseEvent* event)
{
    if (!mViewManager.closeRequest(this))
        event->ignore();
    else
    {
        // closeRequest() returns true if last document
        mViewManager.removeDocAndView(mDocument);
    }
}

void CSVDoc::View::setupFileMenu()
{
    QMenu* file = menuBar()->addMenu(tr("File"));

    QAction* newGame = createMenuEntry("New Game", ":menu-new-game", file, "document-file-newgame");
    connect(newGame, &QAction::triggered, this, &View::newGameRequest);

    QAction* newAddon = createMenuEntry("New Addon", ":menu-new-addon", file, "document-file-newaddon");
    connect(newAddon, &QAction::triggered, this, &View::newAddonRequest);

    QAction* open = createMenuEntry("Open", ":menu-open", file, "document-file-open");
    connect(open, &QAction::triggered, this, &View::loadDocumentRequest);

    QAction* save = createMenuEntry("Save", ":menu-save", file, "document-file-save");
    connect(save, &QAction::triggered, this, &View::save);
    mSave = save;

    file->addSeparator();

    QAction* verify = createMenuEntry("Verify", ":menu-verify", file, "document-file-verify");
    connect(verify, &QAction::triggered, this, &View::verify);
    mVerify = verify;

    QAction* merge = createMenuEntry("Merge", ":menu-merge", file, "document-file-merge");
    connect(merge, &QAction::triggered, this, &View::merge);
    mMerge = merge;

    QAction* loadErrors = createMenuEntry("Error Log", ":error-log", file, "document-file-errorlog");
    connect(loadErrors, &QAction::triggered, this, &View::loadErrorLog);

    QAction* meta = createMenuEntry(CSMWorld::UniversalId::Type_MetaDatas, file, "document-file-metadata");
    connect(meta, &QAction::triggered, this, &View::addMetaDataSubView);

    file->addSeparator();

    QAction* close = createMenuEntry("Close", ":menu-close", file, "document-file-close");
    connect(close, &QAction::triggered, this, &View::close);

    QAction* exit = createMenuEntry("Exit", ":menu-exit", file, "document-file-exit");
    connect(exit, &QAction::triggered, this, &View::exit);

    connect(this, &View::exitApplicationRequest, &mViewManager, &ViewManager::exitApplication);
}

namespace
{

    void updateUndoRedoAction(QAction* action, const std::string& settingsKey)
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
    QMenu* edit = menuBar()->addMenu(tr("Edit"));

    mUndo = mDocument->getUndoStack().createUndoAction(this, tr("Undo"));
    setupShortcut("document-edit-undo", mUndo);
    connect(mUndo, &QAction::changed, this, &View::undoActionChanged);
    mUndo->setIcon(Misc::ScalableIcon::load(":menu-undo"));
    edit->addAction(mUndo);

    mRedo = mDocument->getUndoStack().createRedoAction(this, tr("Redo"));
    connect(mRedo, &QAction::changed, this, &View::redoActionChanged);
    setupShortcut("document-edit-redo", mRedo);
    mRedo->setIcon(Misc::ScalableIcon::load(":menu-redo"));
    edit->addAction(mRedo);

    QAction* userSettings = createMenuEntry("Preferences", ":menu-preferences", edit, "document-edit-preferences");
    connect(userSettings, &QAction::triggered, this, &View::editSettingsRequest);

    QAction* search = createMenuEntry(CSMWorld::UniversalId::Type_Search, edit, "document-edit-search");
    connect(search, &QAction::triggered, this, &View::addSearchSubView);
}

void CSVDoc::View::setupViewMenu()
{
    QMenu* view = menuBar()->addMenu(tr("View"));

    QAction* newWindow = createMenuEntry("New View", ":menu-new-window", view, "document-view-newview");
    connect(newWindow, &QAction::triggered, this, &View::newView);

    mShowStatusBar = createMenuEntry("Toggle Status Bar", ":menu-status-bar", view, "document-view-statusbar");
    connect(mShowStatusBar, &QAction::toggled, this, &View::toggleShowStatusBar);
    mShowStatusBar->setCheckable(true);
    mShowStatusBar->setChecked(CSMPrefs::get()["Windows"]["show-statusbar"].isTrue());

    view->addAction(mShowStatusBar);

    QAction* filters = createMenuEntry(CSMWorld::UniversalId::Type_Filters, view, "document-mechanics-filters");
    connect(filters, &QAction::triggered, this, &View::addFiltersSubView);
}

void CSVDoc::View::setupWorldMenu()
{
    QMenu* world = menuBar()->addMenu(tr("World"));

    QAction* referenceables
        = createMenuEntry(CSMWorld::UniversalId::Type_Referenceables, world, "document-world-referencables");
    connect(referenceables, &QAction::triggered, this, &View::addReferenceablesSubView);

    QAction* references = createMenuEntry(CSMWorld::UniversalId::Type_References, world, "document-world-references");
    connect(references, &QAction::triggered, this, &View::addReferencesSubView);

    world->addSeparator();

    QAction* cells = createMenuEntry(CSMWorld::UniversalId::Type_Cells, world, "document-world-cells");
    connect(cells, &QAction::triggered, this, &View::addCellsSubView);

    QAction* lands = createMenuEntry(CSMWorld::UniversalId::Type_Lands, world, "document-world-lands");
    connect(lands, &QAction::triggered, this, &View::addLandsSubView);

    QAction* landTextures
        = createMenuEntry(CSMWorld::UniversalId::Type_LandTextures, world, "document-world-landtextures");
    connect(landTextures, &QAction::triggered, this, &View::addLandTexturesSubView);

    QAction* grid = createMenuEntry(CSMWorld::UniversalId::Type_Pathgrids, world, "document-world-pathgrid");
    connect(grid, &QAction::triggered, this, &View::addPathgridSubView);

    world->addSeparator();

    QAction* regions = createMenuEntry(CSMWorld::UniversalId::Type_Regions, world, "document-world-regions");
    connect(regions, &QAction::triggered, this, &View::addRegionsSubView);

    QAction* regionMap = createMenuEntry(CSMWorld::UniversalId::Type_RegionMap, world, "document-world-regionmap");
    connect(regionMap, &QAction::triggered, this, &View::addRegionMapSubView);
}

void CSVDoc::View::setupMechanicsMenu()
{
    QMenu* mechanics = menuBar()->addMenu(tr("Mechanics"));

    QAction* scripts = createMenuEntry(CSMWorld::UniversalId::Type_Scripts, mechanics, "document-mechanics-scripts");
    connect(scripts, &QAction::triggered, this, &View::addScriptsSubView);

    QAction* startScripts
        = createMenuEntry(CSMWorld::UniversalId::Type_StartScripts, mechanics, "document-mechanics-startscripts");
    connect(startScripts, &QAction::triggered, this, &View::addStartScriptsSubView);

    QAction* globals = createMenuEntry(CSMWorld::UniversalId::Type_Globals, mechanics, "document-mechanics-globals");
    connect(globals, &QAction::triggered, this, &View::addGlobalsSubView);

    QAction* gmsts = createMenuEntry(CSMWorld::UniversalId::Type_Gmsts, mechanics, "document-mechanics-gamesettings");
    connect(gmsts, &QAction::triggered, this, &View::addGmstsSubView);

    mechanics->addSeparator();

    QAction* spells = createMenuEntry(CSMWorld::UniversalId::Type_Spells, mechanics, "document-mechanics-spells");
    connect(spells, &QAction::triggered, this, &View::addSpellsSubView);

    QAction* enchantments
        = createMenuEntry(CSMWorld::UniversalId::Type_Enchantments, mechanics, "document-mechanics-enchantments");
    connect(enchantments, &QAction::triggered, this, &View::addEnchantmentsSubView);

    QAction* magicEffects
        = createMenuEntry(CSMWorld::UniversalId::Type_MagicEffects, mechanics, "document-mechanics-magiceffects");
    connect(magicEffects, &QAction::triggered, this, &View::addMagicEffectsSubView);
}

void CSVDoc::View::setupCharacterMenu()
{
    QMenu* characters = menuBar()->addMenu(tr("Characters"));

    QAction* skills = createMenuEntry(CSMWorld::UniversalId::Type_Skills, characters, "document-character-skills");
    connect(skills, &QAction::triggered, this, &View::addSkillsSubView);

    QAction* classes = createMenuEntry(CSMWorld::UniversalId::Type_Classes, characters, "document-character-classes");
    connect(classes, &QAction::triggered, this, &View::addClassesSubView);

    QAction* factions = createMenuEntry(CSMWorld::UniversalId::Type_Faction, characters, "document-character-factions");
    connect(factions, &QAction::triggered, this, &View::addFactionsSubView);

    QAction* races = createMenuEntry(CSMWorld::UniversalId::Type_Races, characters, "document-character-races");
    connect(races, &QAction::triggered, this, &View::addRacesSubView);

    QAction* birthsigns
        = createMenuEntry(CSMWorld::UniversalId::Type_Birthsigns, characters, "document-character-birthsigns");
    connect(birthsigns, &QAction::triggered, this, &View::addBirthsignsSubView);

    QAction* bodyParts
        = createMenuEntry(CSMWorld::UniversalId::Type_BodyParts, characters, "document-character-bodyparts");
    connect(bodyParts, &QAction::triggered, this, &View::addBodyPartsSubView);

    characters->addSeparator();

    QAction* topics = createMenuEntry(CSMWorld::UniversalId::Type_Topics, characters, "document-character-topics");
    connect(topics, &QAction::triggered, this, &View::addTopicsSubView);

    QAction* topicInfos
        = createMenuEntry(CSMWorld::UniversalId::Type_TopicInfos, characters, "document-character-topicinfos");
    connect(topicInfos, &QAction::triggered, this, &View::addTopicInfosSubView);

    characters->addSeparator();

    QAction* journals
        = createMenuEntry(CSMWorld::UniversalId::Type_Journals, characters, "document-character-journals");
    connect(journals, &QAction::triggered, this, &View::addJournalsSubView);

    QAction* journalInfos
        = createMenuEntry(CSMWorld::UniversalId::Type_JournalInfos, characters, "document-character-journalinfos");
    connect(journalInfos, &QAction::triggered, this, &View::addJournalInfosSubView);
}

void CSVDoc::View::setupAssetsMenu()
{
    QMenu* assets = menuBar()->addMenu(tr("Assets"));

    QAction* reload = createMenuEntry("Reload", ":menu-reload", assets, "document-assets-reload");
    connect(reload, &QAction::triggered, &mDocument->getData(), &CSMWorld::Data::assetsChanged);

    assets->addSeparator();

    QAction* sounds = createMenuEntry(CSMWorld::UniversalId::Type_Sounds, assets, "document-assets-sounds");
    connect(sounds, &QAction::triggered, this, &View::addSoundsSubView);

    QAction* soundGens = createMenuEntry(CSMWorld::UniversalId::Type_SoundGens, assets, "document-assets-soundgens");
    connect(soundGens, &QAction::triggered, this, &View::addSoundGensSubView);

    assets->addSeparator(); // resources follow here

    QAction* meshes = createMenuEntry(CSMWorld::UniversalId::Type_Meshes, assets, "document-assets-meshes");
    connect(meshes, &QAction::triggered, this, &View::addMeshesSubView);

    QAction* icons = createMenuEntry(CSMWorld::UniversalId::Type_Icons, assets, "document-assets-icons");
    connect(icons, &QAction::triggered, this, &View::addIconsSubView);

    QAction* musics = createMenuEntry(CSMWorld::UniversalId::Type_Musics, assets, "document-assets-musics");
    connect(musics, &QAction::triggered, this, &View::addMusicsSubView);

    QAction* soundFiles = createMenuEntry(CSMWorld::UniversalId::Type_SoundsRes, assets, "document-assets-soundres");
    connect(soundFiles, &QAction::triggered, this, &View::addSoundsResSubView);

    QAction* textures = createMenuEntry(CSMWorld::UniversalId::Type_Textures, assets, "document-assets-textures");
    connect(textures, &QAction::triggered, this, &View::addTexturesSubView);

    QAction* videos = createMenuEntry(CSMWorld::UniversalId::Type_Videos, assets, "document-assets-videos");
    connect(videos, &QAction::triggered, this, &View::addVideosSubView);
}

void CSVDoc::View::setupDebugMenu()
{
    QMenu* debug = menuBar()->addMenu(tr("Debug"));

    QAction* profiles = createMenuEntry(CSMWorld::UniversalId::Type_DebugProfiles, debug, "document-debug-profiles");
    connect(profiles, &QAction::triggered, this, &View::addDebugProfilesSubView);

    debug->addSeparator();

    mGlobalDebugProfileMenu = new GlobalDebugProfileMenu(
        &dynamic_cast<CSMWorld::IdTable&>(
            *mDocument->getData().getTableModel(CSMWorld::UniversalId::Type_DebugProfiles)),
        this);

    connect(mGlobalDebugProfileMenu, &GlobalDebugProfileMenu::triggered, this,
        [this](const std::string& profile) { this->run(profile, ""); });

    QAction* runDebug = debug->addMenu(mGlobalDebugProfileMenu);
    runDebug->setText(tr("Run OpenMW"));
    setupShortcut("document-debug-run", runDebug);
    runDebug->setIcon(Misc::ScalableIcon::load(":run-openmw"));

    QAction* stopDebug = createMenuEntry("Stop OpenMW", ":stop-openmw", debug, "document-debug-shutdown");
    connect(stopDebug, &QAction::triggered, this, &View::stop);
    mStopDebug = stopDebug;

    QAction* runLog = createMenuEntry(CSMWorld::UniversalId::Type_RunLog, debug, "document-debug-runlog");
    connect(runLog, &QAction::triggered, this, &View::addRunLogSubView);
}

void CSVDoc::View::setupHelpMenu()
{
    QMenu* help = menuBar()->addMenu(tr("Help"));

    QAction* helpInfo = createMenuEntry("Help", ":info", help, "document-help-help");
    connect(helpInfo, &QAction::triggered, this, &View::openHelp);

    QAction* tutorial = createMenuEntry("Tutorial", ":info", help, "document-help-tutorial");
    connect(tutorial, &QAction::triggered, this, &View::tutorial);

    QAction* about = createMenuEntry("About OpenMW-CS", ":info", help, "document-help-about");
    connect(about, &QAction::triggered, this, &View::infoAbout);

    QAction* aboutQt = createMenuEntry("About Qt", ":qt", help, "document-help-qt");
    connect(aboutQt, &QAction::triggered, this, &View::infoAboutQt);
}

QAction* CSVDoc::View::createMenuEntry(CSMWorld::UniversalId::Type type, QMenu* menu, const char* shortcutName)
{
    const std::string title = CSMWorld::UniversalId(type).getTypeName();
    QAction* entry = new QAction(QString::fromStdString(title), this);
    setupShortcut(shortcutName, entry);
    const std::string iconName = CSMWorld::UniversalId(type).getIcon();
    if (!iconName.empty() && iconName != ":placeholder")
        entry->setIcon(Misc::ScalableIcon::load(QString::fromStdString(iconName)));

    menu->addAction(entry);

    return entry;
}

QAction* CSVDoc::View::createMenuEntry(
    const std::string& title, const std::string& iconName, QMenu* menu, const char* shortcutName)
{
    QAction* entry = new QAction(QString::fromStdString(title), this);
    setupShortcut(shortcutName, entry);
    if (!iconName.empty() && iconName != ":placeholder")
        entry->setIcon(Misc::ScalableIcon::load(QString::fromStdString(iconName)));

    menu->addAction(entry);

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

    stream << Files::pathToUnicodeString(mDocument->getSavePath().filename());

    if (mDocument->getState() & CSMDoc::State_Modified)
        stream << " *";

    if (mViewTotal > 1)
        stream << " [" << (mViewIndex + 1) << "/" << mViewTotal << "]";

    CSMPrefs::Category& windows = CSMPrefs::State::get()["Windows"];

    bool hideTitle = windows["hide-subview"].isTrue() && mSubViews.size() == 1 && !mSubViews.at(0)->isFloating();

    if (hideTitle)
        stream << " - " << mSubViews.at(0)->getTitle();

    setWindowTitle(QString::fromUtf8(stream.str().c_str()));
}

void CSVDoc::View::updateSubViewIndices(SubView* view)
{
    CSMPrefs::Category& windows = CSMPrefs::State::get()["Windows"];

    if (view && mSubViews.contains(view))
    {
        mSubViews.removeOne(view);

        // adjust (reduce) the scroll area (even floating), except when it is "Scrollbar Only"
        if (windows["mainwindow-scrollbar"].toString() == "Grow then Scroll")
            updateScrollbar();
    }

    bool hideTitle = windows["hide-subview"].isTrue() && mSubViews.size() == 1 && !mSubViews.at(0)->isFloating();

    updateTitle();

    for (SubView* subView : mSubViews)
    {
        if (!subView->isFloating())
        {
            if (hideTitle)
            {
                subView->setTitleBarWidget(new QWidget(this));
                subView->setWindowTitle(QString::fromUtf8(subView->getTitle().c_str()));
            }
            else
            {
                delete subView->titleBarWidget();
                subView->setTitleBarWidget(nullptr);
            }
        }
    }
}

void CSVDoc::View::updateActions()
{
    bool editing = !(mDocument->getState() & CSMDoc::State_Locked);
    bool running = mDocument->getState() & CSMDoc::State_Running;

    for (std::vector<QAction*>::iterator iter(mEditingActions.begin()); iter != mEditingActions.end(); ++iter)
        (*iter)->setEnabled(editing);

    mUndo->setEnabled(editing && mDocument->getUndoStack().canUndo());
    mRedo->setEnabled(editing && mDocument->getUndoStack().canRedo());

    mSave->setEnabled(!(mDocument->getState() & CSMDoc::State_Saving) && !running);
    mVerify->setEnabled(!(mDocument->getState() & CSMDoc::State_Verifying));

    mGlobalDebugProfileMenu->updateActions(running);
    mStopDebug->setEnabled(running);

    mMerge->setEnabled(mDocument->getContentFiles().size() > 1 && !(mDocument->getState() & CSMDoc::State_Merging));
}

CSVDoc::View::View(ViewManager& viewManager, CSMDoc::Document* document, int totalViews)
    : mViewManager(viewManager)
    , mDocument(document)
    , mViewIndex(totalViews - 1)
    , mViewTotal(totalViews)
    , mScroll(nullptr)
    , mScrollbarOnly(false)
{
    CSMPrefs::Category& windows = CSMPrefs::State::get()["Windows"];

    int width = std::max(windows["default-width"].toInt(), 300);
    int height = std::max(windows["default-height"].toInt(), 300);

    resize(width, height);

    mSubViewWindow.setDockOptions(QMainWindow::AllowNestedDocks);

    if (windows["mainwindow-scrollbar"].toString() == "Grow Only")
    {
        setCentralWidget(&mSubViewWindow);
    }
    else
    {
        createScrollArea();
    }

    mOperations = new Operations;
    addDockWidget(Qt::BottomDockWidgetArea, mOperations);

    setContextMenuPolicy(Qt::NoContextMenu);

    updateTitle();

    setupUi();

    updateActions();

    CSVWorld::addSubViewFactories(mSubViewFactory);
    CSVTools::addSubViewFactories(mSubViewFactory);

    mSubViewFactory.add(CSMWorld::UniversalId::Type_RunLog, new SubViewFactory<RunLogSubView>);

    connect(mOperations, &Operations::abortOperation, this, &View::abortOperation);

    connect(&CSMPrefs::State::get(), &CSMPrefs::State::settingChanged, this, &View::settingChanged);
}

const CSMDoc::Document* CSVDoc::View::getDocument() const
{
    return mDocument;
}

CSMDoc::Document* CSVDoc::View::getDocument()
{
    return mDocument;
}

void CSVDoc::View::setIndex(int viewIndex, int totalViews)
{
    mViewIndex = viewIndex;
    mViewTotal = totalViews;
    updateTitle();
}

void CSVDoc::View::updateDocumentState()
{
    updateTitle();
    updateActions();

    static const int operations[] = {
        CSMDoc::State_Saving,
        CSMDoc::State_Verifying,
        CSMDoc::State_Searching,
        CSMDoc::State_Merging,
        // end marker
        -1,
    };

    int state = mDocument->getState();

    for (int i = 0; operations[i] != -1; ++i)
        if (!(state & operations[i]))
            mOperations->quitOperation(operations[i]);

    QList<CSVDoc::SubView*> subViews = findChildren<CSVDoc::SubView*>();

    for (QList<CSVDoc::SubView*>::iterator iter(subViews.begin()); iter != subViews.end(); ++iter)
        (*iter)->setEditLock(state & CSMDoc::State_Locked);
}

void CSVDoc::View::updateProgress(int current, int max, int type, int threads)
{
    mOperations->setProgress(current, max, type, threads);
}

void CSVDoc::View::addSubView(const CSMWorld::UniversalId& id, const std::string& hint)
{
    CSMPrefs::Category& windows = CSMPrefs::State::get()["Windows"];

    bool isReferenceable = id.getClass() == CSMWorld::UniversalId::Class_RefRecord;

    // User setting to reuse sub views (on a per top level view basis)
    if (windows["reuse"].isTrue())
    {
        for (SubView* sb : mSubViews)
        {
            bool isSubViewReferenceable = sb->getUniversalId().getType() == CSMWorld::UniversalId::Type_Referenceable;

            if ((isReferenceable && isSubViewReferenceable && id.getId() == sb->getUniversalId().getId())
                || (!isReferenceable && id == sb->getUniversalId()))
            {
                sb->setFocus();
                if (!hint.empty())
                    sb->useHint(hint);
                return;
            }
        }
    }

    if (mScroll)
        QObject::connect(mScroll->horizontalScrollBar(), &QScrollBar::rangeChanged, this, &View::moveScrollBarToEnd);

    // User setting for limiting the number of sub views per top level view.
    // Automatically open a new top level view if this number is exceeded
    //
    // If the sub view limit setting is one, the sub view title bar is hidden and the
    // text in the main title bar is adjusted accordingly
    if (mSubViews.size() >= windows["max-subviews"].toInt()) // create a new top level view
    {
        mViewManager.addView(mDocument, id, hint);

        return;
    }

    SubView* view = nullptr;
    if (isReferenceable)
    {
        view = mSubViewFactory.makeSubView(
            CSMWorld::UniversalId(CSMWorld::UniversalId::Type_Referenceable, id), *mDocument);
    }
    else
    {
        view = mSubViewFactory.makeSubView(id, *mDocument);
    }
    assert(view);
    view->setParent(this);
    view->setEditLock(mDocument->getState() & CSMDoc::State_Locked);
    mSubViews.append(view); // only after assert

    int minWidth = windows["minimum-width"].toInt();
    view->setMinimumWidth(minWidth);

    view->setStatusBar(mShowStatusBar->isChecked());

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

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    updateWidth(windows["grow-limit"].isTrue(), minWidth);
#else
    updateWidth(true, minWidth);
#endif

    mSubViewWindow.addDockWidget(Qt::TopDockWidgetArea, view);

    updateSubViewIndices();

    connect(view, &SubView::focusId, this, &View::addSubView);

    connect(view, qOverload<SubView*>(&SubView::closeRequest), this, &View::closeRequest);

    connect(view, &SubView::updateTitle, this, &View::updateTitle);

    connect(view, &SubView::updateSubViewIndices, this, &View::updateSubViewIndices);

    CSVWorld::TableSubView* tableView = dynamic_cast<CSVWorld::TableSubView*>(view);
    if (tableView)
    {
        connect(this, &View::requestFocus, tableView, &CSVWorld::TableSubView::requestFocus);
    }

    CSVWorld::SceneSubView* sceneView = dynamic_cast<CSVWorld::SceneSubView*>(view);
    if (sceneView)
    {
        connect(sceneView, &CSVWorld::SceneSubView::requestFocus, this, &View::onRequestFocus);
    }

    if (CSMPrefs::State::get()["ID Tables"]["subview-new-window"].isTrue())
    {
        CSVWorld::DialogueSubView* dialogueView = dynamic_cast<CSVWorld::DialogueSubView*>(view);
        if (dialogueView)
            dialogueView->setFloating(true);

        CSVWorld::ScriptSubView* scriptView = dynamic_cast<CSVWorld::ScriptSubView*>(view);
        if (scriptView)
            scriptView->setFloating(true);
    }

    view->show();

    if (!hint.empty())
        view->useHint(hint);
}

void CSVDoc::View::moveScrollBarToEnd(int min, int max)
{
    if (mScroll)
    {
        mScroll->horizontalScrollBar()->setValue(max);

        QObject::disconnect(mScroll->horizontalScrollBar(), &QScrollBar::rangeChanged, this, &View::moveScrollBarToEnd);
    }
}

void CSVDoc::View::settingChanged(const CSMPrefs::Setting* setting)
{
    if (*setting == "Windows/hide-subview")
        updateSubViewIndices(nullptr);
    else if (*setting == "Windows/mainwindow-scrollbar")
    {
        if (setting->toString() != "Grow Only")
        {
            if (mScroll)
            {
                if (setting->toString() == "Scrollbar Only")
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
        else if (mScroll)
        {
            mScroll->takeWidget();
            setCentralWidget(&mSubViewWindow);
            mScroll->deleteLater();
            mScroll = nullptr;
        }
    }
}

void CSVDoc::View::newView()
{
    mViewManager.addView(mDocument);
}

void CSVDoc::View::save()
{
    mDocument->save();
}

void CSVDoc::View::openHelp()
{
    Misc::HelpViewer::openHelp("manuals/openmw-cs/index.html");
}

void CSVDoc::View::tutorial()
{
    Misc::HelpViewer::openHelp("manuals/openmw-cs/tour.html");
}

void CSVDoc::View::infoAbout()
{
    // Get current OpenMW version
    QString versionInfo = (Version::getOpenmwVersionDescription() +
#if defined(__x86_64__) || defined(_M_X64)
        " (64-bit)")
                              .c_str();
#else
        " (32-bit)")
                              .c_str();
#endif

    // Get current year
    const auto copyrightInfo = Misc::timeToString(std::chrono::system_clock::now(), "Copyright © 2008-%Y OpenMW Team");

    QString aboutText = QString(
        "<p style=\"white-space: pre-wrap;\">"
        "<b><h2>OpenMW Construction Set</h2></b>"
        "%1\n\n"
        "%2\n\n"
        "%3\n\n"
        "<table>"
        "<tr><td>%4</td><td><a href=\"https://openmw.org\">https://openmw.org</a></td></tr>"
        "<tr><td>%5</td><td><a href=\"https://forum.openmw.org\">https://forum.openmw.org</a></td></tr>"
        "<tr><td>%6</td><td><a "
        "href=\"https://gitlab.com/OpenMW/openmw/issues\">https://gitlab.com/OpenMW/openmw/issues</a></td></tr>"
        "<tr><td>%7</td><td><a href=\"https://web.libera.chat/#openmw\">ircs://irc.libera.chat/#openmw</a></td></tr>"
        "</table>"
        "</p>")
                            .arg(versionInfo,
                                tr("OpenMW-CS is a content file editor for OpenMW, a modern, free and open source game "
                                   "engine."),
                                tr(copyrightInfo.c_str()), tr("Home Page:"), tr("Forum:"), tr("Bug Tracker:"),
                                tr("IRC:"));

    QMessageBox::about(this, "About OpenMW-CS", aboutText);
}

void CSVDoc::View::infoAboutQt()
{
    QMessageBox::aboutQt(this);
}

void CSVDoc::View::verify()
{
    addSubView(mDocument->verify());
}

void CSVDoc::View::addGlobalsSubView()
{
    addSubView(CSMWorld::UniversalId::Type_Globals);
}

void CSVDoc::View::addGmstsSubView()
{
    addSubView(CSMWorld::UniversalId::Type_Gmsts);
}

void CSVDoc::View::addSkillsSubView()
{
    addSubView(CSMWorld::UniversalId::Type_Skills);
}

void CSVDoc::View::addClassesSubView()
{
    addSubView(CSMWorld::UniversalId::Type_Classes);
}

void CSVDoc::View::addFactionsSubView()
{
    addSubView(CSMWorld::UniversalId::Type_Factions);
}

void CSVDoc::View::addRacesSubView()
{
    addSubView(CSMWorld::UniversalId::Type_Races);
}

void CSVDoc::View::addSoundsSubView()
{
    addSubView(CSMWorld::UniversalId::Type_Sounds);
}

void CSVDoc::View::addScriptsSubView()
{
    addSubView(CSMWorld::UniversalId::Type_Scripts);
}

void CSVDoc::View::addRegionsSubView()
{
    addSubView(CSMWorld::UniversalId::Type_Regions);
}

void CSVDoc::View::addBirthsignsSubView()
{
    addSubView(CSMWorld::UniversalId::Type_Birthsigns);
}

void CSVDoc::View::addSpellsSubView()
{
    addSubView(CSMWorld::UniversalId::Type_Spells);
}

void CSVDoc::View::addCellsSubView()
{
    addSubView(CSMWorld::UniversalId::Type_Cells);
}

void CSVDoc::View::addReferenceablesSubView()
{
    addSubView(CSMWorld::UniversalId::Type_Referenceables);
}

void CSVDoc::View::addReferencesSubView()
{
    addSubView(CSMWorld::UniversalId::Type_References);
}

void CSVDoc::View::addRegionMapSubView()
{
    addSubView(CSMWorld::UniversalId::Type_RegionMap);
}

void CSVDoc::View::addFiltersSubView()
{
    addSubView(CSMWorld::UniversalId::Type_Filters);
}

void CSVDoc::View::addTopicsSubView()
{
    addSubView(CSMWorld::UniversalId::Type_Topics);
}

void CSVDoc::View::addJournalsSubView()
{
    addSubView(CSMWorld::UniversalId::Type_Journals);
}

void CSVDoc::View::addTopicInfosSubView()
{
    addSubView(CSMWorld::UniversalId::Type_TopicInfos);
}

void CSVDoc::View::addJournalInfosSubView()
{
    addSubView(CSMWorld::UniversalId::Type_JournalInfos);
}

void CSVDoc::View::addEnchantmentsSubView()
{
    addSubView(CSMWorld::UniversalId::Type_Enchantments);
}

void CSVDoc::View::addBodyPartsSubView()
{
    addSubView(CSMWorld::UniversalId::Type_BodyParts);
}

void CSVDoc::View::addSoundGensSubView()
{
    addSubView(CSMWorld::UniversalId::Type_SoundGens);
}

void CSVDoc::View::addMeshesSubView()
{
    addSubView(CSMWorld::UniversalId::Type_Meshes);
}

void CSVDoc::View::addIconsSubView()
{
    addSubView(CSMWorld::UniversalId::Type_Icons);
}

void CSVDoc::View::addMusicsSubView()
{
    addSubView(CSMWorld::UniversalId::Type_Musics);
}

void CSVDoc::View::addSoundsResSubView()
{
    addSubView(CSMWorld::UniversalId::Type_SoundsRes);
}

void CSVDoc::View::addMagicEffectsSubView()
{
    addSubView(CSMWorld::UniversalId::Type_MagicEffects);
}

void CSVDoc::View::addTexturesSubView()
{
    addSubView(CSMWorld::UniversalId::Type_Textures);
}

void CSVDoc::View::addVideosSubView()
{
    addSubView(CSMWorld::UniversalId::Type_Videos);
}

void CSVDoc::View::addDebugProfilesSubView()
{
    addSubView(CSMWorld::UniversalId::Type_DebugProfiles);
}

void CSVDoc::View::addRunLogSubView()
{
    addSubView(CSMWorld::UniversalId::Type_RunLog);
}

void CSVDoc::View::addLandsSubView()
{
    addSubView(CSMWorld::UniversalId::Type_Lands);
}

void CSVDoc::View::addLandTexturesSubView()
{
    addSubView(CSMWorld::UniversalId::Type_LandTextures);
}

void CSVDoc::View::addPathgridSubView()
{
    addSubView(CSMWorld::UniversalId::Type_Pathgrids);
}

void CSVDoc::View::addStartScriptsSubView()
{
    addSubView(CSMWorld::UniversalId::Type_StartScripts);
}

void CSVDoc::View::addSearchSubView()
{
    addSubView(mDocument->newSearch());
}

void CSVDoc::View::addMetaDataSubView()
{
    addSubView(CSMWorld::UniversalId(CSMWorld::UniversalId::Type_MetaData, "sys::meta"));
}

void CSVDoc::View::abortOperation(int type)
{
    mDocument->abortOperation(type);
    updateActions();
}

CSVDoc::Operations* CSVDoc::View::getOperations() const
{
    return mOperations;
}

void CSVDoc::View::exit()
{
    emit exitApplicationRequest(this);
}

void CSVDoc::View::resizeViewWidth(int width)
{
    if (width >= 0)
        resize(width, geometry().height());
}

void CSVDoc::View::resizeViewHeight(int height)
{
    if (height >= 0)
        resize(geometry().width(), height);
}

void CSVDoc::View::toggleShowStatusBar(bool show)
{
    for (QObject* view : mSubViewWindow.children())
    {
        if (CSVDoc::SubView* subView = dynamic_cast<CSVDoc::SubView*>(view))
            subView->setStatusBar(show);
    }
}

void CSVDoc::View::toggleStatusBar(bool checked)
{
    mShowStatusBar->setChecked(checked);
}

void CSVDoc::View::loadErrorLog()
{
    addSubView(CSMWorld::UniversalId(CSMWorld::UniversalId::Type_LoadErrorLog, 0));
}

void CSVDoc::View::run(const std::string& profile, const std::string& startupInstruction)
{
    mDocument->startRunning(profile, startupInstruction);
}

void CSVDoc::View::stop()
{
    mDocument->stopRunning();
}

void CSVDoc::View::closeRequest(SubView* subView)
{
    CSMPrefs::Category& windows = CSMPrefs::State::get()["Windows"];

    if (mSubViews.size() > 1 || mViewTotal <= 1 || !windows["hide-subview"].isTrue())
    {
        subView->deleteLater();
        mSubViews.removeOne(subView);
    }
    else if (mViewManager.closeRequest(this))
        mViewManager.removeDocAndView(mDocument);
}

void CSVDoc::View::updateScrollbar()
{
    QRect rect;
    QWidget* topLevel = QApplication::topLevelAt(pos());
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

    if ((newWidth + frameWidth) >= rect.width())
        mSubViewWindow.setMinimumWidth(newWidth);
    else
        mSubViewWindow.setMinimumWidth(0);
}

void CSVDoc::View::merge()
{
    emit mergeDocument(mDocument);
}

void CSVDoc::View::updateWidth(bool isGrowLimit, int minSubViewWidth)
{
    QRect rect;
    if (isGrowLimit)
    {
        QScreen* screen = getWidgetScreen(pos());
        rect = screen->geometry();
    }
    else
        rect = desktopRect();

    if (!mScrollbarOnly && mScroll && mSubViews.size() > 1)
    {
        int newWidth = width() + minSubViewWidth;
        int frameWidth = frameGeometry().width() - width();
        if (newWidth + frameWidth <= rect.width())
        {
            resize(newWidth, height());
            // WARNING: below code assumes that new subviews are added to the right
            if (x() > rect.width() - (newWidth + frameWidth))
                move(rect.width() - (newWidth + frameWidth), y()); // shift left to stay within the screen
        }
        else
        {
            // full width
            resize(rect.width() - frameWidth, height());
            mSubViewWindow.setMinimumWidth(mSubViewWindow.width() + minSubViewWidth);
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

void CSVDoc::View::onRequestFocus(const std::string& id)
{
    if (CSMPrefs::get()["3D Scene Editing"]["open-list-view"].isTrue())
    {
        addReferencesSubView();
        emit requestFocus(id);
    }
    else
    {
        addSubView(CSMWorld::UniversalId(CSMWorld::UniversalId::Type_Reference, id));
    }
}

QScreen* CSVDoc::View::getWidgetScreen(const QPoint& position)
{
    QScreen* screen = QApplication::screenAt(position);
    if (screen)
        return screen;

    const QList<QScreen*> screens = QApplication::screens();
    if (screens.isEmpty())
        throw std::runtime_error("No screens available");

    int closestDistance = std::numeric_limits<int>::max();
    for (QScreen* candidate : screens)
    {
        const QRect geometry = candidate->geometry();
        const int dx = position.x() - std::clamp(position.x(), geometry.left(), geometry.right());
        const int dy = position.y() - std::clamp(position.y(), geometry.top(), geometry.bottom());
        const int distance = dx * dx + dy * dy;

        if (distance < closestDistance)
        {
            closestDistance = distance;
            screen = candidate;
        }
    }

    if (screen == nullptr)
        throw std::runtime_error(
            Misc::StringUtils::format("Cannot detect the screen for position [%d, %d]", position.x(), position.y()));

    return screen;
}

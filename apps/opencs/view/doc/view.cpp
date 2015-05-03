#include "view.hpp"

#include <sstream>
#include <stdexcept>

#include <QCloseEvent>
#include <QMenuBar>
#include <QMdiArea>
#include <QDockWidget>
#include <QtGui/QApplication>
#include <QDesktopWidget>

#include "../../model/doc/document.hpp"
#include "../../model/settings/usersettings.hpp"

#include "../../model/world/idtable.hpp"

#include "../world/subviews.hpp"

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
    QMenu *file = menuBar()->addMenu (tr ("&File"));

    QAction *newGame = new QAction (tr ("New Game"), this);
    connect (newGame, SIGNAL (triggered()), this, SIGNAL (newGameRequest()));
    file->addAction (newGame);

    QAction *newAddon = new QAction (tr ("New Addon"), this);
    connect (newAddon, SIGNAL (triggered()), this, SIGNAL (newAddonRequest()));
    file->addAction (newAddon);

    QAction *open = new QAction (tr ("&Open"), this);
    connect (open, SIGNAL (triggered()), this, SIGNAL (loadDocumentRequest()));
    file->addAction (open);

    mSave = new QAction (tr ("&Save"), this);
    connect (mSave, SIGNAL (triggered()), this, SLOT (save()));
    file->addAction (mSave);

    mVerify = new QAction (tr ("&Verify"), this);
    connect (mVerify, SIGNAL (triggered()), this, SLOT (verify()));
    file->addAction (mVerify);

    QAction *loadErrors = new QAction (tr ("Load Error Log"), this);
    connect (loadErrors, SIGNAL (triggered()), this, SLOT (loadErrorLog()));
    file->addAction (loadErrors);

    QAction *close = new QAction (tr ("&Close"), this);
    connect (close, SIGNAL (triggered()), this, SLOT (close()));
    file->addAction(close);

    QAction *exit = new QAction (tr ("&Exit"), this);
    connect (exit, SIGNAL (triggered()), this, SLOT (exit()));
    connect (this, SIGNAL(exitApplicationRequest(CSVDoc::View *)), &mViewManager, SLOT(exitApplication(CSVDoc::View *)));

    file->addAction(exit);
}

void CSVDoc::View::setupEditMenu()
{
    QMenu *edit = menuBar()->addMenu (tr ("&Edit"));

    mUndo = mDocument->getUndoStack().createUndoAction (this, tr("&Undo"));
    mUndo->setShortcuts (QKeySequence::Undo);
    edit->addAction (mUndo);

    mRedo= mDocument->getUndoStack().createRedoAction (this, tr("&Redo"));
    mRedo->setShortcuts (QKeySequence::Redo);
    edit->addAction (mRedo);

    QAction *userSettings = new QAction (tr ("&Preferences"), this);
    connect (userSettings, SIGNAL (triggered()), this, SIGNAL (editSettingsRequest()));
    edit->addAction (userSettings);

    QAction *search = new QAction (tr ("Search"), this);
    connect (search, SIGNAL (triggered()), this, SLOT (addSearchSubView()));
    edit->addAction (search);
}

void CSVDoc::View::setupViewMenu()
{
    QMenu *view = menuBar()->addMenu (tr ("&View"));

    QAction *newWindow = new QAction (tr ("&New View"), this);
    connect (newWindow, SIGNAL (triggered()), this, SLOT (newView()));
    view->addAction (newWindow);

    mShowStatusBar = new QAction (tr ("Show Status Bar"), this);
    mShowStatusBar->setCheckable (true);
    connect (mShowStatusBar, SIGNAL (toggled (bool)), this, SLOT (toggleShowStatusBar (bool)));
    std::string showStatusBar =
        CSMSettings::UserSettings::instance().settingValue("window/show-statusbar").toStdString();
    if(showStatusBar == "true")
        mShowStatusBar->setChecked(true);
    view->addAction (mShowStatusBar);

    QAction *filters = new QAction (tr ("Filters"), this);
    connect (filters, SIGNAL (triggered()), this, SLOT (addFiltersSubView()));
    view->addAction (filters);
}

void CSVDoc::View::setupWorldMenu()
{
    QMenu *world = menuBar()->addMenu (tr ("&World"));

    QAction *regions = new QAction (tr ("Regions"), this);
    connect (regions, SIGNAL (triggered()), this, SLOT (addRegionsSubView()));
    world->addAction (regions);

    QAction *cells = new QAction (tr ("Cells"), this);
    connect (cells, SIGNAL (triggered()), this, SLOT (addCellsSubView()));
    world->addAction (cells);

    QAction *referenceables = new QAction (tr ("Objects"), this);
    connect (referenceables, SIGNAL (triggered()), this, SLOT (addReferenceablesSubView()));
    world->addAction (referenceables);

    QAction *references = new QAction (tr ("Instances"), this);
    connect (references, SIGNAL (triggered()), this, SLOT (addReferencesSubView()));
    world->addAction (references);

    QAction *grid = new QAction (tr ("Pathgrid"), this);
    connect (grid, SIGNAL (triggered()), this, SLOT (addPathgridSubView()));
    world->addAction (grid);

    world->addSeparator(); // items that don't represent single record lists follow here

    QAction *regionMap = new QAction (tr ("Region Map"), this);
    connect (regionMap, SIGNAL (triggered()), this, SLOT (addRegionMapSubView()));
    world->addAction (regionMap);
}

void CSVDoc::View::setupMechanicsMenu()
{
    QMenu *mechanics = menuBar()->addMenu (tr ("&Mechanics"));

    QAction *globals = new QAction (tr ("Globals"), this);
    connect (globals, SIGNAL (triggered()), this, SLOT (addGlobalsSubView()));
    mechanics->addAction (globals);

    QAction *gmsts = new QAction (tr ("Game settings"), this);
    connect (gmsts, SIGNAL (triggered()), this, SLOT (addGmstsSubView()));
    mechanics->addAction (gmsts);

    QAction *scripts = new QAction (tr ("Scripts"), this);
    connect (scripts, SIGNAL (triggered()), this, SLOT (addScriptsSubView()));
    mechanics->addAction (scripts);

    QAction *spells = new QAction (tr ("Spells"), this);
    connect (spells, SIGNAL (triggered()), this, SLOT (addSpellsSubView()));
    mechanics->addAction (spells);

    QAction *enchantments = new QAction (tr ("Enchantments"), this);
    connect (enchantments, SIGNAL (triggered()), this, SLOT (addEnchantmentsSubView()));
    mechanics->addAction (enchantments);

    QAction *effects = new QAction (tr ("Magic Effects"), this);
    connect (effects, SIGNAL (triggered()), this, SLOT (addMagicEffectsSubView()));
    mechanics->addAction (effects);

    QAction *startScripts = new QAction (tr ("Start Scripts"), this);
    connect (startScripts, SIGNAL (triggered()), this, SLOT (addStartScriptsSubView()));
    mechanics->addAction (startScripts);
}

void CSVDoc::View::setupCharacterMenu()
{
    QMenu *characters = menuBar()->addMenu (tr ("Characters"));

    QAction *skills = new QAction (tr ("Skills"), this);
    connect (skills, SIGNAL (triggered()), this, SLOT (addSkillsSubView()));
    characters->addAction (skills);

    QAction *classes = new QAction (tr ("Classes"), this);
    connect (classes, SIGNAL (triggered()), this, SLOT (addClassesSubView()));
    characters->addAction (classes);

    QAction *factions = new QAction (tr ("Factions"), this);
    connect (factions, SIGNAL (triggered()), this, SLOT (addFactionsSubView()));
    characters->addAction (factions);

    QAction *races = new QAction (tr ("Races"), this);
    connect (races, SIGNAL (triggered()), this, SLOT (addRacesSubView()));
    characters->addAction (races);

    QAction *birthsigns = new QAction (tr ("Birthsigns"), this);
    connect (birthsigns, SIGNAL (triggered()), this, SLOT (addBirthsignsSubView()));
    characters->addAction (birthsigns);

    QAction *topics = new QAction (tr ("Topics"), this);
    connect (topics, SIGNAL (triggered()), this, SLOT (addTopicsSubView()));
    characters->addAction (topics);

    QAction *journals = new QAction (tr ("Journals"), this);
    connect (journals, SIGNAL (triggered()), this, SLOT (addJournalsSubView()));
    characters->addAction (journals);

    QAction *topicInfos = new QAction (tr ("Topic Infos"), this);
    connect (topicInfos, SIGNAL (triggered()), this, SLOT (addTopicInfosSubView()));
    characters->addAction (topicInfos);

    QAction *journalInfos = new QAction (tr ("Journal Infos"), this);
    connect (journalInfos, SIGNAL (triggered()), this, SLOT (addJournalInfosSubView()));
    characters->addAction (journalInfos);

    QAction *bodyParts = new QAction (tr ("Body Parts"), this);
    connect (bodyParts, SIGNAL (triggered()), this, SLOT (addBodyPartsSubView()));
    characters->addAction (bodyParts);
}

void CSVDoc::View::setupAssetsMenu()
{
    QMenu *assets = menuBar()->addMenu (tr ("&Assets"));

    QAction *sounds = new QAction (tr ("Sounds"), this);
    connect (sounds, SIGNAL (triggered()), this, SLOT (addSoundsSubView()));
    assets->addAction (sounds);

    QAction *soundGens = new QAction (tr ("Sound Generators"), this);
    connect (soundGens, SIGNAL (triggered()), this, SLOT (addSoundGensSubView()));
    assets->addAction (soundGens);

    assets->addSeparator(); // resources follow here

    QAction *meshes = new QAction (tr ("Meshes"), this);
    connect (meshes, SIGNAL (triggered()), this, SLOT (addMeshesSubView()));
    assets->addAction (meshes);

    QAction *icons = new QAction (tr ("Icons"), this);
    connect (icons, SIGNAL (triggered()), this, SLOT (addIconsSubView()));
    assets->addAction (icons);

    QAction *musics = new QAction (tr ("Music"), this);
    connect (musics, SIGNAL (triggered()), this, SLOT (addMusicsSubView()));
    assets->addAction (musics);

    QAction *soundsRes = new QAction (tr ("Sound Files"), this);
    connect (soundsRes, SIGNAL (triggered()), this, SLOT (addSoundsResSubView()));
    assets->addAction (soundsRes);

    QAction *textures = new QAction (tr ("Textures"), this);
    connect (textures, SIGNAL (triggered()), this, SLOT (addTexturesSubView()));
    assets->addAction (textures);

    QAction *videos = new QAction (tr ("Videos"), this);
    connect (videos, SIGNAL (triggered()), this, SLOT (addVideosSubView()));
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

    mStopDebug = new QAction (tr ("Shutdown OpenMW"), this);
    connect (mStopDebug, SIGNAL (triggered()), this, SLOT (stop()));
    debug->addAction (mStopDebug);

    QAction *runLog = new QAction (tr ("Run Log"), this);
    connect (runLog, SIGNAL (triggered()), this, SLOT (addRunLogSubView()));
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

void CSVDoc::View::updateTitle()
{
    std::ostringstream stream;

    stream << mDocument->getSavePath().filename().string();

    if (mDocument->getState() & CSMDoc::State_Modified)
            stream << " *";

    if (mViewTotal>1)
        stream << " [" << (mViewIndex+1) << "/" << mViewTotal << "]";

    CSMSettings::UserSettings &userSettings = CSMSettings::UserSettings::instance();

    bool hideTitle = userSettings.setting ("window/hide-subview", QString ("false"))=="true" &&
        mSubViews.size()==1 && !mSubViews.at (0)->isFloating();

    if (hideTitle)
        stream << " - " << mSubViews.at (0)->getTitle();

    setWindowTitle (QString::fromUtf8(stream.str().c_str()));
}

void CSVDoc::View::updateSubViewIndicies(SubView *view)
{
    if(view && mSubViews.contains(view))
        mSubViews.removeOne(view);

    CSMSettings::UserSettings &userSettings = CSMSettings::UserSettings::instance();

    bool hideTitle = userSettings.setting ("window/hide-subview", QString ("false"))=="true" &&
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
                subView->setTitleBarWidget (0);
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
}

CSVDoc::View::View (ViewManager& viewManager, CSMDoc::Document *document, int totalViews)
    : mViewManager (viewManager), mDocument (document), mViewIndex (totalViews-1),
      mViewTotal (totalViews)
{
    int width = CSMSettings::UserSettings::instance().settingValue
                                    ("window/default-width").toInt();

    int height = CSMSettings::UserSettings::instance().settingValue
                                    ("window/default-height").toInt();

    width = std::max(width, 300);
    height = std::max(height, 300);

    // trick to get the window decorations and their sizes
    show();
    hide();
    resize (width - (frameGeometry().width() - geometry().width()),
            height - (frameGeometry().height() - geometry().height()));

    mSubViewWindow.setDockOptions (QMainWindow::AllowNestedDocks);

    setCentralWidget (&mSubViewWindow);

    mOperations = new Operations;
    addDockWidget (Qt::BottomDockWidgetArea, mOperations);

    updateTitle();

    setupUi();

    updateActions();

    CSVWorld::addSubViewFactories (mSubViewFactory);
    CSVTools::addSubViewFactories (mSubViewFactory);

    mSubViewFactory.add (CSMWorld::UniversalId::Type_RunLog, new SubViewFactory<RunLogSubView>);

    connect (mOperations, SIGNAL (abortOperation (int)), this, SLOT (abortOperation (int)));
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
    CSMSettings::UserSettings &userSettings = CSMSettings::UserSettings::instance();

    bool isReferenceable = id.getClass() == CSMWorld::UniversalId::Class_RefRecord;

    // User setting to reuse sub views (on a per top level view basis)
    bool reuse =
        userSettings.setting ("window/reuse", QString("true")) == "true" ? true : false;
    if(reuse)
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

    // User setting for limiting the number of sub views per top level view.
    // Automatically open a new top level view if this number is exceeded
    //
    // If the sub view limit setting is one, the sub view title bar is hidden and the
    // text in the main title bar is adjusted accordingly
    int maxSubView = userSettings.setting("window/max-subviews", QString("256")).toInt();
    if(mSubViews.size() >= maxSubView) // create a new top level view
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
    mSubViews.append(view); // only after assert

    int minWidth = userSettings.setting ("window/minimum-width", QString("325")).toInt();
    view->setMinimumWidth(minWidth);

    view->setStatusBar (mShowStatusBar->isChecked());

    mSubViewWindow.addDockWidget (Qt::TopDockWidgetArea, view);

    updateSubViewIndicies();

    connect (view, SIGNAL (focusId (const CSMWorld::UniversalId&, const std::string&)), this,
        SLOT (addSubView (const CSMWorld::UniversalId&, const std::string&)));

    connect (view, SIGNAL (closeRequest (SubView *)), this, SLOT (closeRequest (SubView *)));

    connect (view, SIGNAL (updateTitle()), this, SLOT (updateTitle()));

    connect (view, SIGNAL (updateSubViewIndicies (SubView *)),
        this, SLOT (updateSubViewIndicies (SubView *)));

    view->show();

    if (!hint.empty())
        view->useHint (hint);
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

void CSVDoc::View::updateUserSetting (const QString &name, const QStringList &list)
{
    if (name=="window/hide-subview")
        updateSubViewIndicies (0);

    foreach (SubView *subView, mSubViews)
    {
        subView->updateUserSetting (name, list);
    }
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
    CSMSettings::UserSettings &userSettings = CSMSettings::UserSettings::instance();

    if (mSubViews.size()>1 || mViewTotal<=1 ||
        userSettings.setting ("window/hide-subview", QString ("false"))!="true")
        subView->deleteLater();
    else if (mViewManager.closeRequest (this))
        mViewManager.removeDocAndView (mDocument);
}

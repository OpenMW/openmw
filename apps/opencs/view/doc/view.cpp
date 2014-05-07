#include "view.hpp"

#include <sstream>
#include <stdexcept>

#include <QCloseEvent>
#include <QMenuBar>
#include <QMdiArea>
#include <QDockWidget>
#include <QtGui/QApplication>

#include "../../model/doc/document.hpp"
#include "../world/subviews.hpp"
#include "../tools/subviews.hpp"
#include "../../model/settings/usersettings.hpp"
#include "viewmanager.hpp"
#include "operations.hpp"
#include "subview.hpp"

void CSVDoc::View::closeEvent (QCloseEvent *event)
{
    if (!mViewManager.closeRequest (this))
        event->ignore();
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

    QAction *referenceables = new QAction (tr ("Referenceables"), this);
    connect (referenceables, SIGNAL (triggered()), this, SLOT (addReferenceablesSubView()));
    world->addAction (referenceables);

    QAction *references = new QAction (tr ("References"), this);
    connect (references, SIGNAL (triggered()), this, SLOT (addReferencesSubView()));
    world->addAction (references);

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
}

void CSVDoc::View::setupAssetsMenu()
{
    QMenu *assets = menuBar()->addMenu (tr ("&Assets"));

    QAction *sounds = new QAction (tr ("Sounds"), this);
    connect (sounds, SIGNAL (triggered()), this, SLOT (addSoundsSubView()));
    assets->addAction (sounds);
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
}

void CSVDoc::View::updateTitle()
{
    std::ostringstream stream;

    stream << mDocument->getSavePath().filename().string();

    if (mDocument->getState() & CSMDoc::State_Modified)
            stream << " *";

    if (mViewTotal>1)
        stream << " [" << (mViewIndex+1) << "/" << mViewTotal << "]";

    setWindowTitle (stream.str().c_str());
}

void CSVDoc::View::updateActions()
{
    bool editing = !(mDocument->getState() & CSMDoc::State_Locked);

    for (std::vector<QAction *>::iterator iter (mEditingActions.begin()); iter!=mEditingActions.end(); ++iter)
        (*iter)->setEnabled (editing);

    mUndo->setEnabled (editing & mDocument->getUndoStack().canUndo());
    mRedo->setEnabled (editing & mDocument->getUndoStack().canRedo());

    mSave->setEnabled (!(mDocument->getState() & CSMDoc::State_Saving));
    mVerify->setEnabled (!(mDocument->getState() & CSMDoc::State_Verifying));
}

CSVDoc::View::View (ViewManager& viewManager, CSMDoc::Document *document, int totalViews)
    : mViewManager (viewManager), mDocument (document), mViewIndex (totalViews-1),
      mViewTotal (totalViews)
{
    QString width = CSMSettings::UserSettings::instance().settingValue
                                    ("Window Size/Width");

    QString height = CSMSettings::UserSettings::instance().settingValue
                                    ("Window Size/Height");

    resize (width.toInt(), height.toInt());

    mSubViewWindow.setDockOptions (QMainWindow::AllowNestedDocks);

    setCentralWidget (&mSubViewWindow);

    mOperations = new Operations;
    addDockWidget (Qt::BottomDockWidgetArea, mOperations);

    updateTitle();

    setupUi();

    CSVWorld::addSubViewFactories (mSubViewFactory);
    CSVTools::addSubViewFactories (mSubViewFactory);

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
        CSMDoc::State_Saving, CSMDoc::State_Verifying,
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
    /// \todo add an user setting for limiting the number of sub views per top level view. Automatically open a new top level view if this
    /// number is exceeded

    /// \todo if the sub view limit setting is one, the sub view title bar should be hidden and the text in the main title bar adjusted
    /// accordingly

    /// \todo add an user setting to reuse sub views (on a per document basis or on a per top level view basis)

    const std::vector<CSMWorld::UniversalId::Type> referenceables(CSMWorld::UniversalId::listReferenceableTypes());
    SubView *view = NULL;
    if(std::find(referenceables.begin(), referenceables.end(), id.getType()) != referenceables.end())
    {
        view = mSubViewFactory.makeSubView (CSMWorld::UniversalId(CSMWorld::UniversalId::Type_Referenceable, id.getId()), *mDocument);
    } else
    {
        view = mSubViewFactory.makeSubView (id, *mDocument);
    }
    assert(view);
    if (!hint.empty())
        view->useHint (hint);

    view->setStatusBar (mShowStatusBar->isChecked());

    mSubViewWindow.addDockWidget (Qt::TopDockWidgetArea, view);

    connect (view, SIGNAL (focusId (const CSMWorld::UniversalId&, const std::string&)), this,
        SLOT (addSubView (const CSMWorld::UniversalId&, const std::string&)));

    connect (&CSMSettings::UserSettings::instance(),
             SIGNAL (userSettingUpdated (const QString &, const QStringList &)),
             view,
             SLOT (updateUserSetting (const QString &, const QStringList &)));

    view->show();
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

void CSVDoc::View::updateUserSetting
                                (const QString &name, const QStringList &list)
{}

void CSVDoc::View::toggleShowStatusBar (bool show)
{
    foreach (QObject *view, mSubViewWindow.children())
    {
        if (CSVDoc::SubView *subView = dynamic_cast<CSVDoc::SubView *> (view))
            subView->setStatusBar (show);
    }
}

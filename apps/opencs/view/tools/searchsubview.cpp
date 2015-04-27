
#include "searchsubview.hpp"

#include <QVBoxLayout>

#include "../../model/doc/document.hpp"
#include "../../model/tools/search.hpp"
#include "../../model/tools/reportmodel.hpp"
#include "../../model/world/idtablebase.hpp"
#include "../../model/settings/usersettings.hpp"

#include "reporttable.hpp"
#include "searchbox.hpp"

void CSVTools::SearchSubView::replace (bool selection)
{
    if (mLocked)
        return;
        
    std::vector<int> indices = mTable->getReplaceIndices (selection);

    std::string replace = mSearchBox.getReplaceText();

    const CSMTools::ReportModel& model =
        dynamic_cast<const CSMTools::ReportModel&> (*mTable->model());

    bool autoDelete = CSMSettings::UserSettings::instance().setting (
        "search/auto-delete", QString ("true"))=="true";

    CSMTools::Search search (mSearch);
    CSMWorld::IdTableBase *currentTable = 0;
        
    // We are running through the indices in reverse order to avoid messing up multiple results
    // in a single string.
    for (std::vector<int>::const_reverse_iterator iter (indices.rbegin()); iter!=indices.rend(); ++iter)
    {
        CSMWorld::UniversalId id = model.getUniversalId (*iter);

        CSMWorld::UniversalId::Type type = CSMWorld::UniversalId::getParentType (id.getType());

        CSMWorld::IdTableBase *table = &dynamic_cast<CSMWorld::IdTableBase&> (
            *mDocument.getData().getTableModel (type));

        if (table!=currentTable)
        {
            search.configure (table);
            currentTable = table;
        }
            
        std::string hint = model.getHint (*iter);

        if (search.verify (mDocument, table, id, hint))
        {
            search.replace (mDocument, table, id, hint, replace);
            mTable->flagAsReplaced (*iter);

            if (autoDelete)
                mTable->model()->removeRows (*iter, 1);
        }
    }
}

CSVTools::SearchSubView::SearchSubView (const CSMWorld::UniversalId& id, CSMDoc::Document& document)
: CSVDoc::SubView (id), mDocument (document), mLocked (false)
{
    QVBoxLayout *layout = new QVBoxLayout;

    layout->setContentsMargins (QMargins (0, 0, 0, 0));

    layout->addWidget (&mSearchBox);
    
    layout->addWidget (mTable = new ReportTable (document, id, true), 2);

    QWidget *widget = new QWidget;
    
    widget->setLayout (layout);

    setWidget (widget);

    stateChanged (document.getState(), &document);
    
    connect (mTable, SIGNAL (editRequest (const CSMWorld::UniversalId&, const std::string&)),
        SIGNAL (focusId (const CSMWorld::UniversalId&, const std::string&)));

    connect (mTable, SIGNAL (replaceRequest()), this, SLOT (replaceRequest()));
        
    connect (&document, SIGNAL (stateChanged (int, CSMDoc::Document *)),
        this, SLOT (stateChanged (int, CSMDoc::Document *)));

    connect (&mSearchBox, SIGNAL (startSearch (const CSMTools::Search&)),
        this, SLOT (startSearch (const CSMTools::Search&)));

    connect (&mSearchBox, SIGNAL (replaceAll()), this, SLOT (replaceAllRequest()));
}

void CSVTools::SearchSubView::setEditLock (bool locked)
{
    mLocked = locked;
    mSearchBox.setEditLock (locked);
}

void CSVTools::SearchSubView::updateUserSetting (const QString &name, const QStringList &list)
{
    mTable->updateUserSetting (name, list);
}

void CSVTools::SearchSubView::stateChanged (int state, CSMDoc::Document *document)
{
    mSearchBox.setSearchMode (!(state & CSMDoc::State_Searching));
}

void CSVTools::SearchSubView::startSearch (const CSMTools::Search& search)
{
    CSMSettings::UserSettings &userSettings = CSMSettings::UserSettings::instance();

    int paddingBefore = userSettings.setting ("search/char-before", QString ("5")).toInt();
    int paddingAfter = userSettings.setting ("search/char-after", QString ("5")).toInt();

    mSearch = search;
    mSearch.setPadding (paddingBefore, paddingAfter);
    
    mTable->clear();
    mDocument.runSearch (getUniversalId(), mSearch);
}

void CSVTools::SearchSubView::replaceRequest()
{
    replace (true);
}

void CSVTools::SearchSubView::replaceAllRequest()
{
    replace (false);
}


#include "searchsubview.hpp"

#include "reporttable.hpp"

CSVTools::SearchSubView::SearchSubView (const CSMWorld::UniversalId& id, CSMDoc::Document& document)
: CSVDoc::SubView (id)
{
    setWidget (mTable = new ReportTable (document, id, this));

    connect (mTable, SIGNAL (editRequest (const CSMWorld::UniversalId&, const std::string&)),
        SIGNAL (focusId (const CSMWorld::UniversalId&, const std::string&)));
}

void CSVTools::SearchSubView::setEditLock (bool locked)
{
    // ignored. We don't change document state anyway.
}

void CSVTools::SearchSubView::updateUserSetting (const QString &name, const QStringList &list)
{
    mTable->updateUserSetting (name, list);
}

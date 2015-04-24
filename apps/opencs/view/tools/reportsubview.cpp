
#include "reportsubview.hpp"

#include "reporttable.hpp"

CSVTools::ReportSubView::ReportSubView (const CSMWorld::UniversalId& id, CSMDoc::Document& document)
: CSVDoc::SubView (id)
{
    setWidget (mTable = new ReportTable (document, id, false, this));

    connect (mTable, SIGNAL (editRequest (const CSMWorld::UniversalId&, const std::string&)),
        SIGNAL (focusId (const CSMWorld::UniversalId&, const std::string&)));
}

void CSVTools::ReportSubView::setEditLock (bool locked)
{
    // ignored. We don't change document state anyway.
}

void CSVTools::ReportSubView::updateUserSetting (const QString &name, const QStringList &list)
{
    mTable->updateUserSetting (name, list);
}

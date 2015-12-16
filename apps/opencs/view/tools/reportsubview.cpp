#include "reportsubview.hpp"

#include "reporttable.hpp"

CSVTools::ReportSubView::ReportSubView (const CSMWorld::UniversalId& id, CSMDoc::Document& document)
: CSVDoc::SubView (id), mDocument (document), mRefreshState (0)
{
    if (id.getType()==CSMWorld::UniversalId::Type_VerificationResults)
        mRefreshState = CSMDoc::State_Verifying;

    setWidget (mTable = new ReportTable (document, id, false, mRefreshState, this));

    connect (mTable, SIGNAL (editRequest (const CSMWorld::UniversalId&, const std::string&)),
        SIGNAL (focusId (const CSMWorld::UniversalId&, const std::string&)));

    if (mRefreshState==CSMDoc::State_Verifying)
    {
        connect (mTable, SIGNAL (refreshRequest()), this, SLOT (refreshRequest()));

        connect (&document, SIGNAL (stateChanged (int, CSMDoc::Document *)),
            mTable, SLOT (stateChanged (int, CSMDoc::Document *)));
    }
}

void CSVTools::ReportSubView::setEditLock (bool locked)
{
    // ignored. We don't change document state anyway.
}

void CSVTools::ReportSubView::refreshRequest()
{
    if (!(mDocument.getState() & mRefreshState))
    {
        if (mRefreshState==CSMDoc::State_Verifying)
        {
            mTable->clear();
            mDocument.verify (getUniversalId());
        }
    }
}

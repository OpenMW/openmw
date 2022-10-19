#include "reportsubview.hpp"

#include "reporttable.hpp"

#include <apps/opencs/view/doc/subview.hpp>

#include "../../model/doc/document.hpp"
#include "../../model/doc/state.hpp"

CSVTools::ReportSubView::ReportSubView(const CSMWorld::UniversalId& id, CSMDoc::Document& document)
    : CSVDoc::SubView(id)
    , mDocument(document)
    , mRefreshState(0)
{
    if (id.getType() == CSMWorld::UniversalId::Type_VerificationResults)
        mRefreshState = CSMDoc::State_Verifying;

    setWidget(mTable = new ReportTable(document, id, false, mRefreshState, this));

    connect(mTable, &ReportTable::editRequest, this, &ReportSubView::focusId);

    if (mRefreshState == CSMDoc::State_Verifying)
    {
        connect(mTable, &ReportTable::refreshRequest, this, &ReportSubView::refreshRequest);

        connect(&document, &CSMDoc::Document::stateChanged, mTable, &ReportTable::stateChanged);
    }
}

void CSVTools::ReportSubView::setEditLock(bool locked)
{
    // ignored. We don't change document state anyway.
}

void CSVTools::ReportSubView::refreshRequest()
{
    if (!(mDocument.getState() & mRefreshState))
    {
        if (mRefreshState == CSMDoc::State_Verifying)
        {
            mTable->clear();
            mDocument.verify(getUniversalId());
        }
    }
}

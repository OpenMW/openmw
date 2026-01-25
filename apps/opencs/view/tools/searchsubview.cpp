#include "searchsubview.hpp"

#include <QVBoxLayout>

#include "../../model/doc/document.hpp"
#include "../../model/doc/state.hpp"
#include "../../model/prefs/state.hpp"
#include "../../model/tools/reportmodel.hpp"
#include "../../model/tools/search.hpp"
#include "../../model/world/idtablebase.hpp"

#include "../world/creator.hpp"
#include "../world/tablebottombox.hpp"

#include <apps/opencs/model/prefs/category.hpp>
#include <apps/opencs/model/prefs/setting.hpp>
#include <apps/opencs/model/world/data.hpp>
#include <apps/opencs/view/doc/subview.hpp>

#include "reporttable.hpp"
#include "searchbox.hpp"

void CSVTools::SearchSubView::replace(bool selection)
{
    if (mLocked)
        return;

    std::vector<int> indices = mTable->getReplaceIndices(selection);

    std::string replace = mSearchBox.getReplaceText();

    const CSMTools::ReportModel* model = mTable->getReportModel();

    bool autoDelete = CSMPrefs::get()["Search & Replace"]["auto-delete"].isTrue();

    CSMTools::Search search(mSearch);
    CSMWorld::IdTableBase* currentTable = nullptr;

    // We are running through the indices in reverse order to avoid messing up multiple results
    // in a single string.
    for (std::vector<int>::const_reverse_iterator iter(indices.rbegin()); iter != indices.rend(); ++iter)
    {
        const CSMWorld::UniversalId& id = model->getUniversalId(*iter);

        CSMWorld::UniversalId::Type type = CSMWorld::UniversalId::getParentType(id.getType());

        CSMWorld::IdTableBase* table = &dynamic_cast<CSMWorld::IdTableBase&>(*mDocument.getData().getTableModel(type));

        if (table != currentTable)
        {
            search.configure(table);
            currentTable = table;
        }

        std::string hint = model->getHint(*iter);

        if (search.verify(mDocument, table, id, hint))
        {
            search.replace(mDocument, table, id, hint, replace);
            mTable->flagAsReplaced(*iter);

            if (autoDelete)
                mTable->model()->removeRows(*iter, 1);
        }
    }
}

void CSVTools::SearchSubView::showEvent(QShowEvent* event)
{
    CSVDoc::SubView::showEvent(event);
    mSearchBox.focus();
}

CSVTools::SearchSubView::SearchSubView(const CSMWorld::UniversalId& id, CSMDoc::Document& document)
    : CSVDoc::SubView(id)
    , mDocument(document)
    , mLocked(false)
{
    QVBoxLayout* layout = new QVBoxLayout;

    layout->addWidget(&mSearchBox);

    layout->addWidget(mTable = new ReportTable(document, id, true), 2);

    layout->addWidget(mBottom = new CSVWorld::TableBottomBox(CSVWorld::NullCreatorFactory(), document, id, this), 0);

    QWidget* widget = new QWidget;

    widget->setLayout(layout);

    setWidget(widget);

    stateChanged(document.getState(), &document);

    connect(mTable, &ReportTable::editRequest, this, &SearchSubView::focusId);

    connect(mTable, &ReportTable::replaceRequest, this, &SearchSubView::replaceRequest);

    connect(&document, &CSMDoc::Document::stateChanged, this, &SearchSubView::stateChanged);

    connect(
        &mSearchBox, qOverload<const CSMTools::Search&>(&SearchBox::startSearch), this, &SearchSubView::startSearch);

    connect(&mSearchBox, qOverload<>(&SearchBox::replaceAll), this, &SearchSubView::replaceAllRequest);

    connect(document.getReport(id), &CSMTools::ReportModel::rowsRemoved, this, &SearchSubView::tableSizeUpdate);

    connect(document.getReport(id), &CSMTools::ReportModel::rowsInserted, this, &SearchSubView::tableSizeUpdate);

    connect(&document, &CSMDoc::Document::operationDone, this, &SearchSubView::operationDone);
}

void CSVTools::SearchSubView::setEditLock(bool locked)
{
    mLocked = locked;
    mSearchBox.setEditLock(locked);
}

void CSVTools::SearchSubView::setStatusBar(bool show)
{
    mBottom->setStatusBar(show);
}

void CSVTools::SearchSubView::stateChanged(int state, CSMDoc::Document* document)
{
    mSearchBox.setSearchMode(!(state & CSMDoc::State_Searching));
}

void CSVTools::SearchSubView::startSearch(const CSMTools::Search& search)
{
    CSMPrefs::Category& settings = CSMPrefs::get()["Search & Replace"];

    mSearch = search;
    mSearch.setPadding(settings["char-before"].toInt(), settings["char-after"].toInt());

    mTable->clear();
    mDocument.runSearch(getUniversalId(), mSearch);
}

void CSVTools::SearchSubView::replaceRequest()
{
    replace(true);
}

void CSVTools::SearchSubView::replaceAllRequest()
{
    replace(false);
}

void CSVTools::SearchSubView::tableSizeUpdate()
{
    mBottom->tableSizeChanged(mDocument.getReport(getUniversalId())->rowCount(), 0, 0);
}

void CSVTools::SearchSubView::operationDone(int type, bool failed)
{
    if (type == CSMDoc::State_Searching && !failed && !mDocument.getReport(getUniversalId())->rowCount())
    {
        mBottom->setStatusMessage("No Results");
    }
}

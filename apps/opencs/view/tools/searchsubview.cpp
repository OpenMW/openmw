
#include "searchsubview.hpp"

#include <QVBoxLayout>

#include "../../model/doc/document.hpp"
#include "../../model/tools/search.hpp"

#include "reporttable.hpp"
#include "searchbox.hpp"

CSVTools::SearchSubView::SearchSubView (const CSMWorld::UniversalId& id, CSMDoc::Document& document)
: CSVDoc::SubView (id), mDocument (document), mPaddingBefore (10), mPaddingAfter (10)
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

    connect (&document, SIGNAL (stateChanged (int, CSMDoc::Document *)),
        this, SLOT (stateChanged (int, CSMDoc::Document *)));

    connect (&mSearchBox, SIGNAL (startSearch (const CSMTools::Search&)),
        this, SLOT (startSearch (const CSMTools::Search&)));
}

void CSVTools::SearchSubView::setEditLock (bool locked)
{
    // ignored. We don't change document state anyway.
}

void CSVTools::SearchSubView::updateUserSetting (const QString &name, const QStringList &list)
{
    mTable->updateUserSetting (name, list);

    if (!list.empty())
    {
        if (name=="search/char-before")
            mPaddingBefore = list.at (0).toInt();
        else if (name=="search/char-after")
            mPaddingAfter = list.at (0).toInt();
    }
}

void CSVTools::SearchSubView::stateChanged (int state, CSMDoc::Document *document)
{
    mSearchBox.setSearchMode (!(state & CSMDoc::State_Searching));
}

void CSVTools::SearchSubView::startSearch (const CSMTools::Search& search)
{
    CSMTools::Search search2 (search);
    search2.setPadding (mPaddingBefore, mPaddingAfter);
    
    mTable->clear();
    mDocument.runSearch (getUniversalId(), search2);
}

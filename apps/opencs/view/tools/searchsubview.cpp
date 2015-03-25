
#include "searchsubview.hpp"

#include <QVBoxLayout>

#include "../../model/doc/document.hpp"

#include "reporttable.hpp"
#include "searchbox.hpp"

CSVTools::SearchSubView::SearchSubView (const CSMWorld::UniversalId& id, CSMDoc::Document& document)
: CSVDoc::SubView (id)
{
    QVBoxLayout *layout = new QVBoxLayout;

    layout->setContentsMargins (QMargins (0, 0, 0, 0));

    layout->addWidget (&mSearchBox);
    
    layout->addWidget (mTable = new ReportTable (document, id), 2);

    QWidget *widget = new QWidget;
    
    widget->setLayout (layout);

    setWidget (widget);

    stateChanged (document.getState(), &document);
    
    connect (mTable, SIGNAL (editRequest (const CSMWorld::UniversalId&, const std::string&)),
        SIGNAL (focusId (const CSMWorld::UniversalId&, const std::string&)));

    connect (&document, SIGNAL (stateChanged (int, CSMDoc::Document *)),
        this, SLOT (stateChanged (int, CSMDoc::Document *)));
}

void CSVTools::SearchSubView::setEditLock (bool locked)
{
    // ignored. We don't change document state anyway.
}

void CSVTools::SearchSubView::updateUserSetting (const QString &name, const QStringList &list)
{
    mTable->updateUserSetting (name, list);
}

void CSVTools::SearchSubView::stateChanged (int state, CSMDoc::Document *document)
{
    mSearchBox.setSearchMode (!(state & CSMDoc::State_Searching));
}

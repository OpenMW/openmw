#include "runlogsubview.hpp"

#include <QTextEdit>

CSVDoc::RunLogSubView::RunLogSubView (const CSMWorld::UniversalId& id,
    CSMDoc::Document& document)
: SubView (id)
{
    QTextEdit *edit = new QTextEdit (this);
    edit->setDocument (document.getRunLog());
    edit->setReadOnly (true);

    setWidget (edit);
}

void CSVDoc::RunLogSubView::setEditLock (bool locked)
{
    // ignored since this SubView does not have editing
}

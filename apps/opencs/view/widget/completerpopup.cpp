#include "completerpopup.hpp"

CSVWidget::CompleterPopup::CompleterPopup(QWidget* parent)
    : QListView(parent)
{
    setEditTriggers(QAbstractItemView::NoEditTriggers);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setSelectionMode(QAbstractItemView::SingleSelection);
}

int CSVWidget::CompleterPopup::sizeHintForRow(int row) const
{
    if (model() == nullptr)
    {
        return -1;
    }
    if (row < 0 || row >= model()->rowCount())
    {
        return -1;
    }

    ensurePolished();
    QModelIndex index = model()->index(row, modelColumn());
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    QStyleOptionViewItem option;
    initViewItemOption(&option);
#else
    QStyleOptionViewItem option = viewOptions();
#endif
    QAbstractItemDelegate* delegate = itemDelegate(index);
    return delegate->sizeHint(option, index).height();
}

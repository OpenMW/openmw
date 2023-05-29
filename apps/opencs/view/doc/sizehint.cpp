#include "sizehint.hpp"

CSVDoc::SizeHintWidget::SizeHintWidget(QWidget* parent)
    : QWidget(parent)
{
}

QSize CSVDoc::SizeHintWidget::sizeHint() const
{
    return mSize;
}

void CSVDoc::SizeHintWidget::setSizeHint(const QSize& size)
{
    mSize = size;
}

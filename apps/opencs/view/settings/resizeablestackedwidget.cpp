#include "resizeablestackedwidget.hpp"
#include "page.hpp"

#include <QListWidgetItem>

CSVSettings::ResizeableStackedWidget::ResizeableStackedWidget(QWidget *parent) :
    QStackedWidget(parent)
{}

void CSVSettings::ResizeableStackedWidget::addWidget(QWidget* pWidget)
{
   QStackedWidget::addWidget(pWidget);
}

void CSVSettings::ResizeableStackedWidget::changePage
                                                    (int current, int previous)
{
    if (current == previous)
        return;

    Page *prevPage = 0;
    Page *curPage = 0;

    if (previous > -1)
        prevPage = static_cast <Page *> (widget (previous));

    if (current > -1)
        curPage = static_cast <Page *> (widget (current));

    if (prevPage)
        prevPage->hideWidgets();

    if (curPage)
        curPage->showWidgets();

    layout()->activate();

    setCurrentIndex (current);
}

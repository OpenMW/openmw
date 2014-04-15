#include "resizeablestackedwidget.hpp"
#include "page.hpp"

#include <QListWidgetItem>

#include <QDebug>

CSVSettings::ResizeableStackedWidget::ResizeableStackedWidget(QWidget *parent) :
    QStackedWidget(parent)
{}

void CSVSettings::ResizeableStackedWidget::addWidget(QWidget* pWidget)
{
    qDebug() << "ResizeableStackedWidget::addWidget " << pWidget->objectName();
   QStackedWidget::addWidget(pWidget);
}

void CSVSettings::ResizeableStackedWidget::changePage
                                                    (int current, int previous)
{
    qDebug () << "previous = " << previous << "; current = " << current;
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
    setFixedSize(minimumSizeHint());

    setCurrentIndex (current);
}

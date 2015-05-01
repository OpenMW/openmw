#include "frame.hpp"

#include <QWidget>

const QString CSVSettings::Frame::sInvisibleBoxStyle =
    QString::fromUtf8("Frame { border:2px; padding: 2px; margin: 2px;}");

CSVSettings::Frame::Frame (bool isVisible, const QString &title,
                                                               QWidget *parent)
    : QGroupBox (title, parent), mIsHorizontal (true),
    mLayout (new SettingLayout())
{
    setFlat (true);
    mVisibleBoxStyle = styleSheet();

    if (!isVisible)
    {
        // must be Page, not a View
        setStyleSheet (sInvisibleBoxStyle);
    }

    setLayout (mLayout);
}

void CSVSettings::Frame::hideWidgets()
{
    for (int i = 0; i < children().size(); i++)
    {
        QObject *obj = children().at(i);

        Frame *widgFrame = dynamic_cast <Frame *> (obj);

        if (widgFrame)
        {
            widgFrame->hideWidgets();
            continue;
        }

        QWidget *widg = static_cast <QWidget *> (obj);
        if (widg->property("sizePolicy").isValid())
            widg->setSizePolicy (QSizePolicy::Expanding, QSizePolicy::Expanding);
    }

    layout()->activate();
    setFixedSize(minimumSizeHint());

}

void CSVSettings::Frame::showWidgets()
{
    for (int i = 0; i < children().size(); i++)
    {
        QObject *obj = children().at(i);

        Frame *widgFrame = dynamic_cast <Frame *> (obj);

        if (widgFrame)
        {
            widgFrame->showWidgets();
            continue;
        }

        QWidget *widg = static_cast <QWidget *> (obj);

        if (widg->property("sizePolicy").isValid())
        {
            widg->setSizePolicy
                (QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
        }
    }
    layout()->activate();
    setFixedSize(minimumSizeHint());
}

void CSVSettings::Frame::addWidget (QWidget *widget, int row, int column,
                                    int rowSpan, int columnSpan)
{
    if (row == -1)
        row = getNextRow();

    if (column == -1)
        column = getNextColumn();

    mLayout->addWidget (widget, row, column, rowSpan, columnSpan);
    //, Qt::AlignLeft | Qt::AlignTop);

    widget->setSizePolicy (QSizePolicy::Ignored, QSizePolicy::Ignored);
}

int CSVSettings::Frame::getNextRow () const
{
    int row = mLayout->rowCount();

    if (mIsHorizontal && row > 0)
        row--;

    return row;
}

int CSVSettings::Frame::getNextColumn () const
{
    int column = 0;

    if (mIsHorizontal)
        column = mLayout->columnCount();

    return column;
}

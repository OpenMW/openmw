#include "settingbox.hpp"

#include <QDebug>

const QString CSVSettings::SettingBox::INVISIBLE_BOX_STYLE =
    QString::fromUtf8("QSettingBox { border: 0px; padding 0px; margin: 0px;}");

CSVSettings::SettingBox::SettingBox (bool isVisible, const QString &title,
                                                               QWidget *parent)
    : mIsHorizontal (true), QGroupBox (title, parent)
{
    setFlat (true);
    mVisibleBoxStyle = styleSheet();

    if (!isVisible)
        setStyleSheet (INVISIBLE_BOX_STYLE);

    mLayout = new SettingLayout();
    setLayout (mLayout);
}

void CSVSettings::SettingBox::addWidget (QWidget *widget)
{
    if (mIsHorizontal)
    {
        mLayout->addWidget (widget, mLayout->rowCount() - 1,
                            mLayout->columnCount());
    }
    else
        mLayout->addWidget (widget, mLayout->rowCount(), 0);
}

void CSVSettings::SettingBox::addWidget (QWidget *widget, int row, int column)
{
    if (row == -1)
        row = mLayout->rowCount();

    if (column == -1)
        column = 0;

    mLayout->addWidget (widget, row, column, Qt::AlignLeft | Qt::AlignTop);
}

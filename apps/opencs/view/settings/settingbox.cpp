#include <QVBoxLayout>
#include <QHBoxLayout>

#include "settingbox.hpp"

const QString CSVSettings::SettingBox::INVISIBLE_BOX_STYLE =
        QString::fromUtf8("QSettingBox { border: 0px; padding 0px; margin: 0px;}");

CSVSettings::SettingBox::SettingBox (Orientation layoutOrientation, bool isVisible, QWidget *parent) :
    QGroupBox (parent)
{
    setFlat (true);
    mVisibleBoxStyle = styleSheet();

    if (!isVisible)
        setStyleSheet (INVISIBLE_BOX_STYLE);

    if (layoutOrientation == Orient_Horizontal)
        setLayout (new QHBoxLayout);
    else
        setLayout (new QVBoxLayout);
}

bool CSVSettings::SettingBox::borderVisibile() const
{
    return (styleSheet() != INVISIBLE_BOX_STYLE);
}

void CSVSettings::SettingBox::setTitle (const QString &title)
{
    if (borderVisibile() )
    {
        QGroupBox::setTitle (title);
        setMinimumWidth();
    }
}

void CSVSettings::SettingBox::setBorderVisibility (bool value)
{
    if (value)
        setStyleSheet(mVisibleBoxStyle);
    else
        setStyleSheet(INVISIBLE_BOX_STYLE);
}

void CSVSettings::SettingBox::setMinimumWidth()
{
    //set minimum width to accommodate title, if needed
    //1.5 multiplier to account for bold title.
    QFontMetrics fm (font());
    int minWidth = fm.width(title());
    QGroupBox::setMinimumWidth (minWidth * 1.5);
}

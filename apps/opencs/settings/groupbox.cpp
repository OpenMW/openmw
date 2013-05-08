#include "groupbox.hpp"

const QString CsSettings::GroupBox::INVISIBLE_BOX_STYLE =
        QString::fromUtf8("QGroupBox { border: 0px; padding 0px; margin: 0px;}");

CsSettings::GroupBox::GroupBox(QWidget *parent) :
    QGroupBox (parent)
{
    initBox();
}

CsSettings::GroupBox::GroupBox (bool isVisible, QWidget *parent) :
    QGroupBox (parent)
{
    initBox(isVisible);
}

void CsSettings::GroupBox::initBox(bool isVisible)
{
    setFlat (true);
    VISIBLE_BOX_STYLE = styleSheet();

    if (!isVisible)
        setStyleSheet (INVISIBLE_BOX_STYLE);
}

bool CsSettings::GroupBox::borderVisibile() const
{
    return (styleSheet() != INVISIBLE_BOX_STYLE);
}

void CsSettings::GroupBox::setTitle (const QString &title)
{
    if (borderVisibile() )
    {
        QGroupBox::setTitle (title);
        setMinimumWidth();
    }
}

void CsSettings::GroupBox::setBorderVisibility (bool value)
{
    if (value)
        setStyleSheet(VISIBLE_BOX_STYLE);
    else
        setStyleSheet(INVISIBLE_BOX_STYLE);
}

void CsSettings::GroupBox::setMinimumWidth()
{
    //set minimum width to accommodate title, if needed
    //1.5 multiplier to account for bold title.
    QFontMetrics fm (font());
    int minWidth = fm.width(title());
    QGroupBox::setMinimumWidth (minWidth * 1.5);
}

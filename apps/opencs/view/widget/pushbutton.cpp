#include "pushbutton.hpp"

#include <QMouseEvent>
#include <QKeyEvent>

#include "../../model/prefs/state.hpp"
#include "../../model/prefs/shortcutmanager.hpp"

void CSVWidget::PushButton::processShortcuts()
{
    mProcessedToolTip = CSMPrefs::State::get().getShortcutManager().processToolTip(mToolTip);
}

void CSVWidget::PushButton::setExtendedToolTip()
{
    QString tooltip = mProcessedToolTip;

    if (tooltip.isEmpty())
        tooltip = "(Tool tip not implemented yet)";

    switch (mType)
    {
        case Type_TopMode:

            tooltip +=
                "<p>(left click to change mode)";

            break;

        case Type_TopAction:

            break;

        case Type_Mode:

            tooltip +=
                "<p>(left click to activate,"
                "<br>shift-left click to activate and keep panel open)";

            break;

        case Type_Toggle:

            tooltip += "<p>(left click to ";
            tooltip += isChecked() ? "disable" : "enable";
            tooltip += "<p>shift-left click to ";
            tooltip += isChecked() ? "disable" : "enable";
            tooltip += " and keep panel open)";

            break;
    }

    setToolTip (tooltip);
}

void CSVWidget::PushButton::keyPressEvent (QKeyEvent *event)
{
    if (event->key()!=Qt::Key_Shift)
        mKeepOpen = false;

    QPushButton::keyPressEvent (event);
}

void CSVWidget::PushButton::keyReleaseEvent (QKeyEvent *event)
{
    if (event->key()==Qt::Key_Space)
        mKeepOpen = event->modifiers() & Qt::ShiftModifier;

    QPushButton::keyReleaseEvent (event);
}

void CSVWidget::PushButton::mouseReleaseEvent (QMouseEvent *event)
{
    mKeepOpen = event->button()==Qt::LeftButton && (event->modifiers() & Qt::ShiftModifier);
    QPushButton::mouseReleaseEvent (event);
}

CSVWidget::PushButton::PushButton (const QIcon& icon, Type type, const QString& tooltip,
    QWidget *parent)
: QPushButton (icon, "", parent), mKeepOpen (false), mType (type), mToolTip (tooltip)
{
    if (type==Type_Mode || type==Type_Toggle)
    {
        setCheckable (true);
        connect (this, SIGNAL (toggled (bool)), this, SLOT (checkedStateChanged (bool)));
    }
    setCheckable (type==Type_Mode || type==Type_Toggle);
    processShortcuts();
    setExtendedToolTip();

    connect (&CSMPrefs::State::get(), SIGNAL (settingChanged (const CSMPrefs::Setting *)),
        this, SLOT (settingChanged (const CSMPrefs::Setting *)));
}

CSVWidget::PushButton::PushButton (Type type, const QString& tooltip, QWidget *parent)
: QPushButton (parent), mKeepOpen (false), mType (type), mToolTip (tooltip)
{
    setCheckable (type==Type_Mode || type==Type_Toggle);
    processShortcuts();
    setExtendedToolTip();
}

bool CSVWidget::PushButton::hasKeepOpen() const
{
    return mKeepOpen;
}

QString CSVWidget::PushButton::getBaseToolTip() const
{
    return mProcessedToolTip;
}

CSVWidget::PushButton::Type CSVWidget::PushButton::getType() const
{
    return mType;
}

void CSVWidget::PushButton::checkedStateChanged (bool checked)
{
    setExtendedToolTip();
}

void CSVWidget::PushButton::settingChanged (const CSMPrefs::Setting *setting)
{
    if (setting->getParent()->getKey() == "Key Bindings")
    {
        processShortcuts();
        setExtendedToolTip();
    }
}

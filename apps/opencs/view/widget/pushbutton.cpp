#include "pushbutton.hpp"

#include <QMouseEvent>
#include <QKeyEvent>

#include "../../model/prefs/state.hpp"

void CSVWidget::PushButton::processShortcuts()
{
    const QChar SequenceStart = '{';
    const QChar SequenceEnd = '}';
    const QString ModifierSequence = QString::fromUtf8(":mod");

    const QChar SettingSeparator = ';';

    QStringList substrings;

    int prevIndex = 0;
    int startIndex = mToolTip.indexOf(SequenceStart);
    int endIndex = (startIndex != -1) ? mToolTip.indexOf(SequenceEnd, startIndex) : -1;

    // Process every valid shortcut escape sequence
    while (startIndex != -1 && endIndex != -1)
    {
        int count = startIndex - prevIndex;
        if (count > 0)
        {
            substrings.push_back(mToolTip.mid(prevIndex, count));
        }

        // Find sequence name
        count = endIndex - startIndex - 1;
        if (count > 0)
        {
            // Check if looking for modifier
            int separatorIndex = mToolTip.indexOf(ModifierSequence, startIndex);
            if (separatorIndex != -1 && separatorIndex < endIndex)
            {
                count = separatorIndex - startIndex - 1;

                QString settingName = mToolTip.mid(startIndex+1, count);
                QString value = QString::fromUtf8(
                    CSMPrefs::State::get()["Key Bindings"][settingName.toUtf8().data()].toString().c_str());

                substrings.push_back(value.right(value.size() - value.indexOf(SettingSeparator) - 1));
            }
            else
            {
                QString settingName = mToolTip.mid(startIndex+1, count);
                QString value = QString::fromUtf8(
                    CSMPrefs::State::get()["Key Bindings"][settingName.toUtf8().data()].toString().c_str());

                // Don't want modifier
                substrings.push_back(value.left(value.indexOf(SettingSeparator)));
            }

            prevIndex = endIndex + 1;
        }

        startIndex = mToolTip.indexOf(SequenceStart, endIndex);
        endIndex = (startIndex != -1) ? mToolTip.indexOf(SequenceEnd, startIndex) : -1;
    }

    if (prevIndex < mToolTip.size())
    {
        substrings.push_back(mToolTip.mid(prevIndex));
    }

    mProcessedToolTip = substrings.join("");
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

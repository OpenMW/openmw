#include "shortcutsetting.hpp"

#include <QEvent>
#include <QKeyEvent>
#include <QLabel>
#include <QMouseEvent>
#include <QPushButton>
#include <QString>
#include <QWidget>

#include <components/settings/settings.hpp>

#include <apps/opencs/model/prefs/category.hpp>
#include <apps/opencs/model/prefs/setting.hpp>

#include "shortcutmanager.hpp"
#include "state.hpp"

namespace CSMPrefs
{
    ShortcutSetting::ShortcutSetting(
        Category* parent, QMutex* mutex, std::string_view key, const QString& label, Settings::Index& index)
        : TypedSetting(parent, mutex, key, label, index)
        , mButton(nullptr)
        , mEditorActive(false)
        , mEditorPos(0)
    {
        for (int i = 0; i < MaxKeys; ++i)
        {
            mEditorKeys[i] = 0;
        }
    }

    SettingWidgets ShortcutSetting::makeWidgets(QWidget* parent)
    {
        QKeySequence sequence;
        State::get().getShortcutManager().getSequence(getKey(), sequence);

        QString text = QString::fromUtf8(State::get().getShortcutManager().convertToString(sequence).c_str());

        QLabel* label = new QLabel(getLabel(), parent);
        QPushButton* widget = new QPushButton(text, parent);

        widget->setCheckable(true);
        widget->installEventFilter(this);

        // right clicking on button sets shortcut to RMB, so context menu should not appear
        widget->setContextMenuPolicy(Qt::PreventContextMenu);

        mButton = widget;

        connect(widget, &QPushButton::toggled, this, &ShortcutSetting::buttonToggled);

        return SettingWidgets{ .mLabel = label, .mInput = widget };
    }

    void ShortcutSetting::updateWidget()
    {
        if (mButton)
        {
            const std::string shortcut = getValue();

            QKeySequence sequence;
            State::get().getShortcutManager().convertFromString(shortcut, sequence);
            State::get().getShortcutManager().setSequence(getKey(), sequence);
            resetState();
        }
    }

    bool ShortcutSetting::eventFilter(QObject* target, QEvent* event)
    {
        if (event->type() == QEvent::KeyPress)
        {
            QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
            if (keyEvent->isAutoRepeat())
                return true;

            int mod = keyEvent->modifiers();
            int key = keyEvent->key();

            return handleEvent(target, mod, key, true);
        }
        else if (event->type() == QEvent::KeyRelease)
        {
            QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
            if (keyEvent->isAutoRepeat())
                return true;

            int mod = keyEvent->modifiers();
            int key = keyEvent->key();

            return handleEvent(target, mod, key, false);
        }
        else if (event->type() == QEvent::MouseButtonPress)
        {
            QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
            int mod = mouseEvent->modifiers();
            int key = mouseEvent->button();

            return handleEvent(target, mod, key, true);
        }
        else if (event->type() == QEvent::MouseButtonRelease)
        {
            QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
            int mod = mouseEvent->modifiers();
            int key = mouseEvent->button();

            return handleEvent(target, mod, key, false);
        }
        else if (event->type() == QEvent::FocusOut)
        {
            resetState();
        }

        return false;
    }

    bool ShortcutSetting::handleEvent(QObject* target, int mod, int value, bool active)
    {
        // Modifiers are handled differently
        const int blacklist[] = { Qt::Key_Shift, Qt::Key_Control, Qt::Key_Meta, Qt::Key_Alt, Qt::Key_AltGr };

        const size_t blacklistSize = std::size(blacklist);

        if (!mEditorActive)
        {
            if (value == Qt::RightButton && !active)
            {
                // Clear sequence
                QKeySequence sequence = QKeySequence(0, 0, 0, 0);
                storeValue(sequence);

                resetState();
            }

            return false;
        }

        // Handle blacklist
        for (size_t i = 0; i < blacklistSize; ++i)
        {
            if (value == blacklist[i])
                return true;
        }

        if (!active || mEditorPos >= MaxKeys)
        {
            // Update key
            QKeySequence sequence = QKeySequence(mEditorKeys[0], mEditorKeys[1], mEditorKeys[2], mEditorKeys[3]);
            storeValue(sequence);

            resetState();
        }
        else
        {
            if (mEditorPos == 0)
            {
                mEditorKeys[0] = mod | value;
            }
            else
            {
                mEditorKeys[mEditorPos] = value;
            }

            mEditorPos += 1;
        }

        return true;
    }

    void ShortcutSetting::storeValue(const QKeySequence& sequence)
    {
        State::get().getShortcutManager().setSequence(getKey(), sequence);
        setValue(State::get().getShortcutManager().convertToString(sequence));
        getParent()->getState()->update(*this);
    }

    void ShortcutSetting::resetState()
    {
        mButton->setChecked(false);
        mEditorActive = false;
        mEditorPos = 0;
        for (int i = 0; i < MaxKeys; ++i)
        {
            mEditorKeys[i] = 0;
        }

        // Button text
        QKeySequence sequence;
        State::get().getShortcutManager().getSequence(getKey(), sequence);

        QString text = QString::fromUtf8(State::get().getShortcutManager().convertToString(sequence).c_str());
        mButton->setText(text);
    }

    void ShortcutSetting::buttonToggled(bool checked)
    {
        if (checked)
            mButton->setText("Press keys or click here...");

        mEditorActive = checked;
    }
}

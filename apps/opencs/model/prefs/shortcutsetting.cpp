#include "shortcutsetting.hpp"

#include <QEvent>
#include <QKeyEvent>
#include <QLabel>
#include <QMouseEvent>
#include <QPushButton>
#include <QWidget>
#include <QString>

#include "state.hpp"
#include "shortcutmanager.hpp"

namespace CSMPrefs
{
    ShortcutSetting::ShortcutSetting(Category* parent, Settings::Manager* values, QMutex* mutex, const std::string& key,
        const std::string& label)
        : Setting(parent, values, mutex, key, label)
        , mButton(nullptr)
        , mEditorActive(false)
        , mEditorPos(0)
    {
        for (int i = 0; i < MaxKeys; ++i)
        {
            mEditorKeys[i] = 0;
        }
    }

    std::pair<QWidget*, QWidget*> ShortcutSetting::makeWidgets(QWidget* parent)
    {
        QKeySequence sequence;
        State::get().getShortcutManager().getSequence(getKey(), sequence);

        QString text = QString::fromUtf8(State::get().getShortcutManager().convertToString(sequence).c_str());

        QLabel* label = new QLabel(QString::fromUtf8(getLabel().c_str()), parent);
        QPushButton* widget = new QPushButton(text, parent);

        widget->setCheckable(true);
        widget->installEventFilter(this);

        // right clicking on button sets shortcut to RMB, so context menu should not appear
        widget->setContextMenuPolicy(Qt::PreventContextMenu);

        mButton = widget;

        connect(widget, SIGNAL(toggled(bool)), this, SLOT(buttonToggled(bool)));

        return std::make_pair(label, widget);
    }

    void ShortcutSetting::updateWidget()
    {
        if (mButton)
        {
            std::string shortcut = getValues().getString(getKey(), getParent()->getKey());

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
        const int Blacklist[] =
        {
            Qt::Key_Shift,
            Qt::Key_Control,
            Qt::Key_Meta,
            Qt::Key_Alt,
            Qt::Key_AltGr
        };

        const size_t BlacklistSize = sizeof(Blacklist) / sizeof(int);

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
        for (size_t i = 0; i < BlacklistSize; ++i)
        {
            if (value == Blacklist[i])
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

        // Convert to string and assign
        std::string value = State::get().getShortcutManager().convertToString(sequence);

        {
            QMutexLocker lock(getMutex());
            getValues().setString(getKey(), getParent()->getKey(), value);
        }

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

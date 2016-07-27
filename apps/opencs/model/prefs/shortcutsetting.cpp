#include "shortcutsetting.hpp"

#include <QEvent>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QMouseEvent>
#include <QPushButton>
#include <QWidget>

#include "state.hpp"
#include "shortcutmanager.hpp"

namespace CSMPrefs
{
    ShortcutSetting::ShortcutSetting(Category* parent, Settings::Manager* values, QMutex* mutex, const std::string& key,
        const std::string& label, const QKeySequence& default_)
        : Setting(parent, values, mutex, key, label)
        , mDefault(default_)
        , mEditorActive(false)
        , mEditorPos(0)
    {
        const int MaxKeys = 4; // A limitation of QKeySequence

        for (int i = 0; i < MaxKeys; ++i)
        {
            mEditorKeys[i] = 0;
        }
    }

    std::pair<QWidget*, QWidget*> ShortcutSetting::makeWidgets(QWidget* parent)
    {
        QString text = QString::fromUtf8(State::get().getShortcutManager().convertToString(mDefault).c_str());

        QLabel* label = new QLabel(QString::fromUtf8(getLabel().c_str()), parent);
        QPushButton* widget = new QPushButton(text, parent);

        widget->setCheckable(true);
        widget->installEventFilter(this);
        mButton = widget;

        connect(widget, SIGNAL(toggled(bool)), this, SLOT(buttonToggled(bool)));

        return std::make_pair(label, widget);
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

        return false;
    }

    bool ShortcutSetting::handleEvent(QObject* target, int mod, int value, bool active)
    {
        const int MaxKeys = 4; // A limitation of QKeySequence

        // Modifiers are handled differently
        const int Blacklist[]
        {
            Qt::Key_Shift,
            Qt::Key_Control,
            Qt::Key_Meta,
            Qt::Key_Alt,
            Qt::Key_AltGr
        };

        const size_t BlacklistSize = sizeof(Blacklist) / sizeof(int);

        if (!mEditorActive)
            return false;

        // Handle blacklist
        for (size_t i = 0; i < BlacklistSize; ++i)
        {
            if (value == Blacklist[i])
                return true;
        }

        if (!active || mEditorPos >= MaxKeys)
        {
            // Update key
            QKeySequence sequence;
            int modifier = 0;

            State::get().getShortcutManager().getSequence(getKey(), sequence, modifier);

            sequence = QKeySequence(mEditorKeys[0], mEditorKeys[1], mEditorKeys[2], mEditorKeys[3]);
            State::get().getShortcutManager().setSequence(getKey(), sequence, modifier);

            // Store
            {
                std::string value = State::get().getShortcutManager().convertToString(sequence, modifier);

                QMutexLocker lock(getMutex());
                getValues().setString(getKey(), getParent()->getKey(), value);
            }

            getParent()->getState()->update(*this);

            // Update button
            QString text = QString::fromUtf8(State::get().getShortcutManager().convertToString(sequence).c_str());

            mButton->setText(text);
            mButton->setChecked(false);
            mEditorActive = false;

            // Reset
            mEditorPos = 0;
            for (int i = 0; i < MaxKeys; ++i)
            {
                mEditorKeys[i] = 0;
            }
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

    void ShortcutSetting::buttonToggled(bool checked)
    {
        if (checked)
            mButton->setText("Press keys or click here...");

        mEditorActive = checked;
    }
}

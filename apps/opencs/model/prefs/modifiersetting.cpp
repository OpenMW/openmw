#include "modifiersetting.hpp"

#include <QEvent>
#include <QKeyEvent>
#include <QLabel>
#include <QMouseEvent>
#include <QPushButton>
#include <QWidget>

#include "state.hpp"
#include "shortcutmanager.hpp"

namespace CSMPrefs
{
    ModifierSetting::ModifierSetting(Category* parent, Settings::Manager* values, QMutex* mutex, const std::string& key,
        const std::string& label)
        : Setting(parent, values, mutex, key, label)
        , mButton(nullptr)
        , mEditorActive(false)
    {
    }

    std::pair<QWidget*, QWidget*> ModifierSetting::makeWidgets(QWidget* parent)
    {
        int modifier = 0;
        State::get().getShortcutManager().getModifier(getKey(), modifier);

        QString text = QString::fromUtf8(State::get().getShortcutManager().convertToString(modifier).c_str());

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

    void ModifierSetting::updateWidget()
    {
        if (mButton)
        {
            std::string shortcut = getValues().getString(getKey(), getParent()->getKey());

            int modifier;
            State::get().getShortcutManager().convertFromString(shortcut, modifier);
            State::get().getShortcutManager().setModifier(getKey(), modifier);
            resetState();
        }
    }

    bool ModifierSetting::eventFilter(QObject* target, QEvent* event)
    {
        if (event->type() == QEvent::KeyPress)
        {
            QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
            if (keyEvent->isAutoRepeat())
                return true;

            int mod = keyEvent->modifiers();
            int key = keyEvent->key();

            return handleEvent(target, mod, key);
        }
        else if (event->type() == QEvent::MouseButtonPress)
        {
            QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
            int mod = mouseEvent->modifiers();
            int button = mouseEvent->button();

            return handleEvent(target, mod, button);
        }
        else if (event->type() == QEvent::FocusOut)
        {
            resetState();
        }

        return false;
    }

    bool ModifierSetting::handleEvent(QObject* target, int mod, int value)
    {
        // For potential future exceptions
        const int Blacklist[] =
        {
            0
        };

        const size_t BlacklistSize = sizeof(Blacklist) / sizeof(int);

        if (!mEditorActive)
        {
            if (value == Qt::RightButton)
            {
                // Clear modifier
                int modifier = 0;
                storeValue(modifier);

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


        // Update modifier
        int modifier = value;
        storeValue(modifier);

        resetState();

        return true;
    }

    void ModifierSetting::storeValue(int modifier)
    {
        State::get().getShortcutManager().setModifier(getKey(), modifier);

        // Convert to string and assign
        std::string value = State::get().getShortcutManager().convertToString(modifier);

        {
            QMutexLocker lock(getMutex());
            getValues().setString(getKey(), getParent()->getKey(), value);
        }

        getParent()->getState()->update(*this);
    }

    void ModifierSetting::resetState()
    {
        mButton->setChecked(false);
        mEditorActive = false;

        // Button text
        int modifier = 0;
        State::get().getShortcutManager().getModifier(getKey(), modifier);

        QString text = QString::fromUtf8(State::get().getShortcutManager().convertToString(modifier).c_str());
        mButton->setText(text);
    }

    void ModifierSetting::buttonToggled(bool checked)
    {
        if (checked)
            mButton->setText("Press keys or click here...");

        mEditorActive = checked;
    }
}

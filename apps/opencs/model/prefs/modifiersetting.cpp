#include "modifiersetting.hpp"

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
    ModifierSetting::ModifierSetting(Category* parent, Settings::Manager* values, QMutex* mutex, const std::string& key,
        const std::string& label)
        : Setting(parent, values, mutex, key, label)
        , mEditorActive(false)
    {
    }

    std::pair<QWidget*, QWidget*> ModifierSetting::makeWidgets(QWidget* parent)
    {
        QKeySequence sequence;
        int modifier = 0;
        State::get().getShortcutManager().getSequence(getKey(), sequence, modifier);

        QString text = QString::fromUtf8(State::get().getShortcutManager().convertToString(modifier).c_str());

        QLabel* label = new QLabel(QString::fromUtf8(getLabel().c_str()), parent);
        QPushButton* widget = new QPushButton(text, parent);

        widget->setCheckable(true);
        widget->installEventFilter(this);
        mButton = widget;

        connect(widget, SIGNAL(toggled(bool)), this, SLOT(buttonToggled(bool)));

        return std::make_pair(label, widget);
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
                QKeySequence sequence;
                int modifier = 0;

                State::get().getShortcutManager().getSequence(getKey(), sequence, modifier);

                modifier = 0;
                State::get().getShortcutManager().setSequence(getKey(), sequence, modifier);

                // Store
                {
                    std::string value = State::get().getShortcutManager().convertToString(sequence, modifier);

                    QMutexLocker lock(getMutex());
                    getValues().setString(getKey(), getParent()->getKey(), value);
                }

                // Update button
                mButton->setText("");
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
        QKeySequence sequence;
        int modifier = 0;

        State::get().getShortcutManager().getSequence(getKey(), sequence, modifier);

        modifier = value;
        State::get().getShortcutManager().setSequence(getKey(), sequence, modifier);

        // Store
        {
            std::string value = State::get().getShortcutManager().convertToString(sequence, modifier);

            QMutexLocker lock(getMutex());
            getValues().setString(getKey(), getParent()->getKey(), value);
        }

        getParent()->getState()->update(*this);

        // Update button
        QString text = QString::fromUtf8(State::get().getShortcutManager().convertToString(modifier).c_str());

        mButton->setText(text);
        mButton->setChecked(false);
        mEditorActive = false;

        return true;
    }

    void ModifierSetting::buttonToggled(bool checked)
    {
        if (checked)
            mButton->setText("Press keys or click here...");

        mEditorActive = checked;
    }
}

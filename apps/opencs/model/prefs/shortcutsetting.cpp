#include "shortcutsetting.hpp"

#include <QLabel>
#include <QLineEdit>
#include <QWidget>

#include "state.hpp"
#include "shortcutmanager.hpp"

namespace CSMPrefs
{
    ShortcutSetting::ShortcutSetting(Category* parent, Settings::Manager* values, QMutex* mutex, const std::string& key,
        const std::string& label, const SequenceData& default_)
        : Setting(parent, values, mutex, key, label)
        , mDefault(default_)
    {
        State::get().getShortcutManager().setSequence(key, mDefault);
    }

    std::pair<QWidget*, QWidget*> ShortcutSetting::makeWidgets(QWidget* parent)
    {
        QLabel* label = new QLabel(QString::fromUtf8(getLabel().c_str()), parent);
        QLineEdit* widget = new QLineEdit(State::get().getShortcutManager().sequenceToString(mDefault).c_str(), parent);

        connect(widget, SIGNAL(textChanged(const QString&)), this, SLOT(valueChanged(const QString&)));

        return std::make_pair(label, widget);
    }

    void ShortcutSetting::valueChanged(const QString& text)
    {
        {
            QMutexLocker lock(getMutex());
            getValues().setString(getKey(), getParent()->getKey(), text.toUtf8().data());

            SequenceData data = State::get().getShortcutManager().stringToSequence(text.toUtf8().data());

            State::get().getShortcutManager().setSequence(getKey(), data);
        }

        getParent()->getState()->update(*this);
    }
}

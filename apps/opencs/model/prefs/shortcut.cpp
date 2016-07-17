#include "shortcut.hpp"

#include <QKeyEvent>
#include <QMouseEvent>
#include <QShortcut>

#include "state.hpp"
#include "shortcutmanager.hpp"

namespace CSMPrefs
{
    Shortcut::Shortcut(const std::string& name, QObject* parent)
        : QObject(parent)
        , mName(name)
        , mCurrentPos(0)
        , mLastPos(0)
        , mActive(false)
        , mEnabled(true)
    {
        State::get().getShortcutManager().addShortcut(this);
        setSequence(State::get().getShortcutManager().getSequence(name));
    }

    Shortcut::~Shortcut()
    {
        State::get().getShortcutManager().removeShortcut(this);
    }

    bool Shortcut::isActive() const
    {
        return mActive;
    }

    bool Shortcut::isEnabled() const
    {
        return mEnabled;
    }

    const std::string& Shortcut::getName() const
    {
        return mName;
    }

    const QKeySequence& Shortcut::getSequence() const
    {
        return mSequence;
    }

    int Shortcut::getPosition() const
    {
        return mCurrentPos;
    }

    int Shortcut::getLastPosition() const
    {
        return mLastPos;
    }

    void Shortcut::setPosition(int pos)
    {
        mCurrentPos = pos;
    }

    void Shortcut::setSequence(const QKeySequence& sequence)
    {
        mSequence = sequence;
        mCurrentPos = 0;
        mLastPos = sequence.count() - 1;
    }

    void Shortcut::activate(bool state)
    {
        mActive = state;
        emit activated(state);

        if (state)
            emit activated();
    }

    void Shortcut::enable(bool state)
    {
        mEnabled = state;
    }

    QString Shortcut::toString() const
    {
        return QString(State::get().getShortcutManager().sequenceToString(mSequence).data());
    }
}

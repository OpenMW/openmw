#include "shortcut.hpp"

#include <cassert>

#include <QAction>
#include <QWidget>

#include <components/debug/debuglog.hpp>

#include "state.hpp"
#include "shortcutmanager.hpp"

namespace CSMPrefs
{
    Shortcut::Shortcut(const std::string& name, QWidget* parent)
        : QObject(parent)
        , mEnabled(true)
        , mName(name)
        , mModName("")
        , mSecondaryMode(SM_Ignore)
        , mModifier(0)
        , mCurrentPos(0)
        , mLastPos(0)
        , mActivationStatus(AS_Inactive)
        , mModifierStatus(false)
        , mAction(0)
    {
        assert (parent);

        State::get().getShortcutManager().addShortcut(this);
        State::get().getShortcutManager().getSequence(name, mSequence);
    }

    Shortcut::Shortcut(const std::string& name, const std::string& modName, QWidget* parent)
        : QObject(parent)
        , mEnabled(true)
        , mName(name)
        , mModName(modName)
        , mSecondaryMode(SM_Ignore)
        , mModifier(0)
        , mCurrentPos(0)
        , mLastPos(0)
        , mActivationStatus(AS_Inactive)
        , mModifierStatus(false)
        , mAction(0)
    {
        assert (parent);

        State::get().getShortcutManager().addShortcut(this);
        State::get().getShortcutManager().getSequence(name, mSequence);
        State::get().getShortcutManager().getModifier(modName, mModifier);
    }

    Shortcut::Shortcut(const std::string& name, const std::string& modName, SecondaryMode secMode, QWidget* parent)
        : QObject(parent)
        , mEnabled(true)
        , mName(name)
        , mModName(modName)
        , mSecondaryMode(secMode)
        , mModifier(0)
        , mCurrentPos(0)
        , mLastPos(0)
        , mActivationStatus(AS_Inactive)
        , mModifierStatus(false)
        , mAction(0)
    {
        assert (parent);

        State::get().getShortcutManager().addShortcut(this);
        State::get().getShortcutManager().getSequence(name, mSequence);
        State::get().getShortcutManager().getModifier(modName, mModifier);
    }

    Shortcut::~Shortcut()
    {
        try
        {
            State::get().getShortcutManager().removeShortcut(this);
        }
        catch(const std::exception& e)
        {
            Log(Debug::Error) << "Error in the destructor: " << e.what();
        }
    }

    bool Shortcut::isEnabled() const
    {
        return mEnabled;
    }

    const std::string& Shortcut::getName() const
    {
        return mName;
    }

    const std::string& Shortcut::getModifierName() const
    {
        return mModName;
    }

    Shortcut::SecondaryMode Shortcut::getSecondaryMode() const
    {
        return mSecondaryMode;
    }

    const QKeySequence& Shortcut::getSequence() const
    {
        return mSequence;
    }

    int Shortcut::getModifier() const
    {
        return mModifier;
    }

    int Shortcut::getPosition() const
    {
        return mCurrentPos;
    }

    int Shortcut::getLastPosition() const
    {
        return mLastPos;
    }

    Shortcut::ActivationStatus Shortcut::getActivationStatus() const
    {
        return mActivationStatus;
    }

    bool Shortcut::getModifierStatus() const
    {
        return mModifierStatus;
    }

    void Shortcut::enable(bool state)
    {
        mEnabled = state;
    }

    void Shortcut::setSequence(const QKeySequence& sequence)
    {
        mSequence = sequence;
        mCurrentPos = 0;
        mLastPos = sequence.count() - 1;

        if (mAction)
        {
            mAction->setText(mActionText + "\t" + State::get().getShortcutManager().convertToString(mSequence).data());
        }
    }

    void Shortcut::setModifier(int modifier)
    {
        mModifier = modifier;
    }

    void Shortcut::setPosition(int pos)
    {
        mCurrentPos = pos;
    }

    void Shortcut::setActivationStatus(ActivationStatus status)
    {
        mActivationStatus = status;
    }

    void Shortcut::setModifierStatus(bool status)
    {
        mModifierStatus = status;
    }

    void Shortcut::associateAction(QAction* action)
    {
        if (mAction)
        {
            mAction->setText(mActionText);

            disconnect(this, SIGNAL(activated()), mAction, SLOT(trigger()));
            disconnect(mAction, SIGNAL(destroyed()), this, SLOT(actionDeleted()));
        }

        mAction = action;

        if (mAction)
        {
            mActionText = mAction->text();
            mAction->setText(mActionText + "\t" + State::get().getShortcutManager().convertToString(mSequence).data());

            connect(this, SIGNAL(activated()), mAction, SLOT(trigger()));
            connect(mAction, SIGNAL(destroyed()), this, SLOT(actionDeleted()));
        }
    }

    void Shortcut::signalActivated(bool state)
    {
        emit activated(state);
    }

    void Shortcut::signalActivated()
    {
        emit activated();
    }

    void Shortcut::signalSecondary(bool state)
    {
        emit secondary(state);
    }
    void Shortcut::signalSecondary()
    {
        emit secondary();
    }

    QString Shortcut::toString() const
    {
        return QString(State::get().getShortcutManager().convertToString(mSequence, mModifier).data());
    }

    void Shortcut::actionDeleted()
    {
        mAction = 0;
    }
}

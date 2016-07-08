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
    {
        State::get().getShortcutManager().addShortcut(this);
        setSequence(State::get().getShortcutManager().getSequence(name));
    }

    Shortcut::~Shortcut()
    {
        State::get().getShortcutManager().removeShortcut(this);
    }

    const std::string& Shortcut::getName() const
    {
        return mName;
    }

    const QKeySequence& Shortcut::getSequence() const
    {
        return mSequence;
    }

    void Shortcut::setSequence(const QKeySequence& sequence)
    {
        mSequence = sequence;
        mCurrentPos = 0;
        mLastPos = sequence.count() - 1;
    }

    void Shortcut::keyPressEvent(QKeyEvent* event)
    {
        int withMod = event->key() | event->modifiers();
        int noMod = event->key();

        if (withMod == mSequence[mCurrentPos] || (mCurrentPos > 0 && noMod == mSequence[mCurrentPos]))
        {
            if (mCurrentPos == mLastPos)
            {
                activated(true);
            }
            else
                ++mCurrentPos;
        }
    }

    void Shortcut::keyReleaseEvent(QKeyEvent* event)
    {
        const int KeyMask = 0x01FFFFFF;

        if ((mSequence[mCurrentPos] & KeyMask) == event->key())
        {
            if (mCurrentPos == mLastPos)
            {
                activated(false);
                mCurrentPos = 0; // Resets to start, maybe shouldn't?
            }
            else if (mCurrentPos > 0)
            {
                --mCurrentPos;
            }
        }
    }

    void Shortcut::mousePressEvent(QMouseEvent* event)
    {
        int withMod = event->button() | (int)event->modifiers();
        int noMod = event->button();

        if (withMod == mSequence[mCurrentPos] || (mCurrentPos > 0 && noMod == mSequence[mCurrentPos]))
        {
            if (mCurrentPos == mLastPos)
                activated(true);
            else
                ++mCurrentPos;
        }
    }

    void Shortcut::mouseReleaseEvent(QMouseEvent* event)
    {
        const int MouseMask = 0x0000001F;

        if ((mSequence[mCurrentPos] & MouseMask) == event->button())
        {
            if (mCurrentPos == mLastPos)
            {
                activated(false);
                mCurrentPos = 0;
            }
            else if (mCurrentPos > 0)
            {
                --mCurrentPos;
            }
        }
    }

    QShortcutWrapper::QShortcutWrapper(const std::string& name, QShortcut* shortcut)
        : QObject(shortcut)
        , mName(name)
        , mShortcut(shortcut)
    {
        State::get().getShortcutManager().addShortcut(this);
        setSequence(State::get().getShortcutManager().getSequence(name));
    }

    QShortcutWrapper::~QShortcutWrapper()
    {
        State::get().getShortcutManager().removeShortcut(this);
    }

    const std::string& QShortcutWrapper::getName() const
    {
        return mName;
    }

    const QKeySequence& QShortcutWrapper::getSequence() const
    {
        return mSequence;
    }

    void QShortcutWrapper::setSequence(const QKeySequence& sequence)
    {
        mSequence = sequence;
        mShortcut->setKey(sequence);
    }
}

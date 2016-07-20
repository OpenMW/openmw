#include "shortcuteventhandler.hpp"

#include <algorithm>
#include <iostream>

#include <QEvent>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QWidget>

#include "shortcut.hpp"

namespace CSMPrefs
{
    ShortcutEventHandler::ShortcutEventHandler(QObject* parent)
        : QObject(parent)
    {
    }

    void ShortcutEventHandler::addShortcut(Shortcut* shortcut)
    {
        mShortcuts.push_back(shortcut);
    }

    void ShortcutEventHandler::removeShortcut(Shortcut* shortcut)
    {
        std::remove(mShortcuts.begin(), mShortcuts.end(), shortcut);
    }

    bool ShortcutEventHandler::eventFilter(QObject* watched, QEvent* event)
    {
        if (event->type() == QEvent::KeyPress)
        {
            QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
            unsigned int mod = (unsigned int) keyEvent->modifiers();
            unsigned int key = (unsigned int) keyEvent->key();

            if (!keyEvent->isAutoRepeat())
                return activate(mod, key);
        }
        else if (event->type() == QEvent::KeyRelease)
        {
            QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
            unsigned int key = (unsigned int) keyEvent->key();

            if (!keyEvent->isAutoRepeat())
                return deactivate(key);
        }
        else if (event->type() == QEvent::MouseButtonPress)
        {
            QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
            unsigned int mod = (unsigned int) mouseEvent->modifiers();
            unsigned int button = (unsigned int) mouseEvent->button();

            return activate(mod, button);
        }
        else if (event->type() == QEvent::MouseButtonRelease)
        {
            QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
            unsigned int button = (unsigned int) mouseEvent->button();

            return deactivate(button);
        }
        else if (event->type() == QEvent::FocusOut)
        {
            // Deactivate in case events are missed
            for (std::vector<Shortcut*>::iterator it = mShortcuts.begin(); it != mShortcuts.end(); ++it)
            {
                Shortcut* shortcut = *it;

                if (shortcut->isActive())
                {
                    shortcut->activate(false);
                    shortcut->setPosition(0);
                }
            }
        }

        return false;
    }

    bool ShortcutEventHandler::activate(unsigned int mod, unsigned int button)
    {
        std::vector<std::pair<MatchResult, Shortcut*> > potentials;
        bool used = false;

        // Find potential activations
        for (std::vector<Shortcut*>::iterator it = mShortcuts.begin(); it != mShortcuts.end(); ++it)
        {
            Shortcut* shortcut = *it;
            int pos = shortcut->getPosition();
            int lastPos = shortcut->getLastPosition();
            MatchResult result = match(mod, button, shortcut->getSequence()[pos]);

            if (!shortcut->isEnabled())
                continue;

            if (result == Matches_WithMod || result == Matches_NoMod)
            {
                if (pos < lastPos && (result == Matches_WithMod || pos > 0))
                {
                    shortcut->setPosition(pos+1);
                    used = true;
                }
                else if (pos == lastPos)
                {
                    potentials.push_back(std::make_pair(result, shortcut));
                }
            }
        }

        // Only activate the best match; in exact conflicts, this will favor the first shortcut added.
        if (!potentials.empty())
        {
            std::sort(potentials.begin(), potentials.end(), ShortcutEventHandler::sort);
            potentials.front().second->activate(true);
            used = true;
        }

        return used;
    }

    bool ShortcutEventHandler::deactivate(unsigned int button)
    {
        const int KeyMask = 0x01FFFFFF;

        bool used = false;

        for (std::vector<Shortcut*>::iterator it = mShortcuts.begin(); it != mShortcuts.end(); ++it)
        {
            Shortcut* shortcut = *it;
            int pos = shortcut->getPosition();
            MatchResult result = match(0, button, shortcut->getSequence()[pos] & KeyMask);

            if (result != Matches_Not)
            {
                if (shortcut->isActive())
                    shortcut->activate(false);

                shortcut->setPosition(0);

                used = true;
            }
        }

        return used;
    }

    ShortcutEventHandler::MatchResult ShortcutEventHandler::match(unsigned int mod, unsigned int button,
        unsigned int value)
    {
        if ((mod | button) == value)
        {
            return Matches_WithMod;
        }
        else if (button == value)
        {
            return Matches_NoMod;
        }
        else
        {
            return Matches_Not;
        }
    }

    bool ShortcutEventHandler::sort(const std::pair<MatchResult, Shortcut*>& left,
        const std::pair<MatchResult, Shortcut*>& right)
    {
        if (left.first == Matches_WithMod && left.first != right.first)
            return true;
        else
            return left.second->getPosition() >= right.second->getPosition();
    }
}

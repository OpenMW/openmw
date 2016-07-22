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
            unsigned int mod = (unsigned int) keyEvent->modifiers();
            unsigned int key = (unsigned int) keyEvent->key();

            if (!keyEvent->isAutoRepeat())
                return deactivate(mod, key);
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
            unsigned int mod = (unsigned int) mouseEvent->modifiers();
            unsigned int button = (unsigned int) mouseEvent->button();

            return deactivate(mod, button);
        }
        else if (event->type() == QEvent::FocusOut)
        {
            // Deactivate in case events are missed
            for (std::vector<Shortcut*>::iterator it = mShortcuts.begin(); it != mShortcuts.end(); ++it)
            {
                Shortcut* shortcut = *it;

                shortcut->setPosition(0);
                shortcut->setModifierStatus(false);

                if (shortcut->getActivationStatus() == Shortcut::AS_Regular)
                {
                    shortcut->setActivationStatus(Shortcut::AS_Inactive);
                    shortcut->signalActivated(false);
                }
                else if (shortcut->getActivationStatus() == Shortcut::AS_Secondary)
                {
                    shortcut->setActivationStatus(Shortcut::AS_Inactive);
                    shortcut->signalSecondary(false);
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

            if (!shortcut->isEnabled())
                continue;

            int pos = shortcut->getPosition();
            int lastPos = shortcut->getLastPosition();
            MatchResult result = match(mod, button, shortcut->getSequence()[pos]);

            if (result == Matches_WithMod || result == Matches_NoMod)
            {
                if (pos < lastPos && (result == Matches_WithMod || pos > 0))
                {
                    shortcut->setPosition(pos+1);
                }
                else if (pos == lastPos)
                {
                    potentials.push_back(std::make_pair(result, shortcut));
                }
            }

            if (checkModifier(mod, button, shortcut, true))
                used = true;
        }

        // Only activate the best match; in exact conflicts, this will favor the first shortcut added.
        if (!potentials.empty())
        {
            std::sort(potentials.begin(), potentials.end(), ShortcutEventHandler::sort);
            Shortcut* shortcut = potentials.front().second;

            if (shortcut->getModifierStatus() && shortcut->getSecondaryMode() == Shortcut::SM_Replace)
            {
                shortcut->setActivationStatus(Shortcut::AS_Secondary);
                shortcut->signalSecondary(true);
                shortcut->signalSecondary();
            }
            else
            {
                shortcut->setActivationStatus(Shortcut::AS_Regular);
                shortcut->signalActivated(true);
                shortcut->signalActivated();
            }

            used = true;
        }

        return used;
    }

    bool ShortcutEventHandler::deactivate(unsigned int mod, unsigned int button)
    {
        const int KeyMask = 0x01FFFFFF;

        bool used = false;

        for (std::vector<Shortcut*>::iterator it = mShortcuts.begin(); it != mShortcuts.end(); ++it)
        {
            Shortcut* shortcut = *it;

            if (checkModifier(mod, button, shortcut, false))
                used = true;

            int pos = shortcut->getPosition();
            MatchResult result = match(0, button, shortcut->getSequence()[pos] & KeyMask);

            if (result != Matches_Not)
            {
                shortcut->setPosition(0);

                if (shortcut->getActivationStatus() == Shortcut::AS_Regular)
                {
                    shortcut->setActivationStatus(Shortcut::AS_Inactive);
                    shortcut->signalActivated(false);
                    used = true;
                }
                else if (shortcut->getActivationStatus() == Shortcut::AS_Secondary)
                {
                    shortcut->setActivationStatus(Shortcut::AS_Inactive);
                    shortcut->signalSecondary(false);
                    used = true;
                }
            }
        }

        return used;
    }

    bool ShortcutEventHandler::checkModifier(unsigned int mod, unsigned int button, Shortcut* shortcut, bool activate)
    {
        if (!shortcut->isEnabled() || !shortcut->getModifier() || shortcut->getSecondaryMode() == Shortcut::SM_Ignore)
            return false;

        MatchResult result = match(mod, button, shortcut->getModifier());
        bool used = false;

        if (result != Matches_Not)
        {
            shortcut->setModifierStatus(activate);

            if (shortcut->getSecondaryMode() == Shortcut::SM_Detach)
            {
                if (activate)
                {
                    shortcut->signalSecondary(true);
                    shortcut->signalSecondary();
                }
                else
                {
                    shortcut->signalSecondary(false);
                }

                used = true;
            }
            else if (!activate && shortcut->getActivationStatus() == Shortcut::AS_Secondary)
            {
                shortcut->setActivationStatus(Shortcut::AS_Inactive);
                shortcut->setPosition(0);
                shortcut->signalSecondary(false);
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

#include "shortcuteventhandler.hpp"

#include <algorithm>
#include <cassert>

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
        // Enforced by shortcut class
        QWidget* widget = static_cast<QWidget*>(shortcut->parent());

        // Check if widget setup is needed
        ShortcutMap::iterator shortcutListIt = mWidgetShortcuts.find(widget);
        if (shortcutListIt == mWidgetShortcuts.end())
        {
            // Create list
            shortcutListIt = mWidgetShortcuts.insert(std::make_pair(widget, ShortcutList())).first;

            // Check if widget has a parent with shortcuts, unfortunately it is not typically set yet
            updateParent(widget);

            // Intercept widget events
            widget->installEventFilter(this);
            connect(widget, SIGNAL(destroyed()), this, SLOT(widgetDestroyed()));
        }

        // Add to list
        shortcutListIt->second.push_back(shortcut);
    }

    void ShortcutEventHandler::removeShortcut(Shortcut* shortcut)
    {
        // Enforced by shortcut class
        QWidget* widget = static_cast<QWidget*>(shortcut->parent());

        ShortcutMap::iterator shortcutListIt = mWidgetShortcuts.find(widget);
        if (shortcutListIt != mWidgetShortcuts.end())
        {
            shortcutListIt->second.erase(std::remove(shortcutListIt->second.begin(), shortcutListIt->second.end(), shortcut), shortcutListIt->second.end());
        }
    }

    bool ShortcutEventHandler::eventFilter(QObject* watched, QEvent* event)
    {
        // Process event
        if (event->type() == QEvent::KeyPress)
        {
            QWidget* widget = static_cast<QWidget*>(watched);
            QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
            unsigned int mod = (unsigned int) keyEvent->modifiers();
            unsigned int key = (unsigned int) keyEvent->key();

            if (!keyEvent->isAutoRepeat())
                return activate(widget, mod, key);
        }
        else if (event->type() == QEvent::KeyRelease)
        {
            QWidget* widget = static_cast<QWidget*>(watched);
            QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
            unsigned int mod = (unsigned int) keyEvent->modifiers();
            unsigned int key = (unsigned int) keyEvent->key();

            if (!keyEvent->isAutoRepeat())
                return deactivate(widget, mod, key);
        }
        else if (event->type() == QEvent::MouseButtonPress)
        {
            QWidget* widget = static_cast<QWidget*>(watched);
            QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
            unsigned int mod = (unsigned int) mouseEvent->modifiers();
            unsigned int button = (unsigned int) mouseEvent->button();

            return activate(widget, mod, button);
        }
        else if (event->type() == QEvent::MouseButtonRelease)
        {
            QWidget* widget = static_cast<QWidget*>(watched);
            QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
            unsigned int mod = (unsigned int) mouseEvent->modifiers();
            unsigned int button = (unsigned int) mouseEvent->button();

            return deactivate(widget, mod, button);
        }
        else if (event->type() == QEvent::FocusOut)
        {
            QWidget* widget = static_cast<QWidget*>(watched);
            ShortcutMap::iterator shortcutListIt = mWidgetShortcuts.find(widget);

            // Deactivate in case events are missed
            for (ShortcutList::iterator it = shortcutListIt->second.begin(); it != shortcutListIt->second.end(); ++it)
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
        else if (event->type() == QEvent::FocusIn)
        {
            QWidget* widget = static_cast<QWidget*>(watched);
            updateParent(widget);
        }

        return false;
    }

    void ShortcutEventHandler::updateParent(QWidget* widget)
    {
        QWidget* parent = widget->parentWidget();
        while (parent)
        {
            ShortcutMap::iterator parentIt = mWidgetShortcuts.find(parent);
            if (parentIt != mWidgetShortcuts.end())
            {
                mChildParentRelations.insert(std::make_pair(widget, parent));
                updateParent(parent);
                break;
            }

            // Check next
            parent = parent->parentWidget();
        }
    }

    bool ShortcutEventHandler::activate(QWidget* widget, unsigned int mod, unsigned int button)
    {
        std::vector<std::pair<MatchResult, Shortcut*> > potentials;
        bool used = false;

        while (widget)
        {
            ShortcutMap::iterator shortcutListIt = mWidgetShortcuts.find(widget);
            assert(shortcutListIt != mWidgetShortcuts.end());

            // Find potential activations
            for (ShortcutList::iterator it = shortcutListIt->second.begin(); it != shortcutListIt->second.end(); ++it)
            {
                Shortcut* shortcut = *it;

                if (!shortcut->isEnabled())
                    continue;

                if (checkModifier(mod, button, shortcut, true))
                    used = true;

                if (shortcut->getActivationStatus() != Shortcut::AS_Inactive)
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
            }

            // Move on to parent
            WidgetMap::iterator widgetIt = mChildParentRelations.find(widget);
            widget = (widgetIt != mChildParentRelations.end()) ? widgetIt->second : 0;
        }

        // Only activate the best match; in exact conflicts, this will favor the first shortcut added.
        if (!potentials.empty())
        {
            std::stable_sort(potentials.begin(), potentials.end(), ShortcutEventHandler::sort);
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

    bool ShortcutEventHandler::deactivate(QWidget* widget, unsigned int mod, unsigned int button)
    {
        const int KeyMask = 0x01FFFFFF;

        bool used = false;

        while (widget)
        {
            ShortcutMap::iterator shortcutListIt = mWidgetShortcuts.find(widget);
            assert(shortcutListIt != mWidgetShortcuts.end());

            for (ShortcutList::iterator it = shortcutListIt->second.begin(); it != shortcutListIt->second.end(); ++it)
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

            // Move on to parent
            WidgetMap::iterator widgetIt = mChildParentRelations.find(widget);
            widget = (widgetIt != mChildParentRelations.end()) ? widgetIt->second : 0;
        }

        return used;
    }

    bool ShortcutEventHandler::checkModifier(unsigned int mod, unsigned int button, Shortcut* shortcut, bool activate)
    {
        if (!shortcut->isEnabled() || !shortcut->getModifier() || shortcut->getSecondaryMode() == Shortcut::SM_Ignore ||
            shortcut->getModifierStatus() == activate)
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
        if (left.first == Matches_WithMod && right.first == Matches_NoMod)
            return true;
        else
            return left.second->getPosition() > right.second->getPosition();
    }

    void ShortcutEventHandler::widgetDestroyed()
    {
        QWidget* widget = static_cast<QWidget*>(sender());

        mWidgetShortcuts.erase(widget);
        mChildParentRelations.erase(widget);
    }
}

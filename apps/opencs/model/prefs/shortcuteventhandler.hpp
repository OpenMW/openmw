#ifndef CSM_PREFS_SHORTCUT_EVENT_HANDLER_H
#define CSM_PREFS_SHORTCUT_EVENT_HANDLER_H

#include <map>
#include <vector>

#include <QObject>

class QEvent;
class QWidget;

namespace CSMPrefs
{
    class Shortcut;

    /// Users of this class should install it as an event handler
    class ShortcutEventHandler : public QObject
    {
            Q_OBJECT

        public:

            ShortcutEventHandler(QObject* parent);

            void addShortcut(Shortcut* shortcut);
            void removeShortcut(Shortcut* shortcut);

        protected:

            bool eventFilter(QObject* watched, QEvent* event) override;

        private:

            typedef std::vector<Shortcut*> ShortcutList;
            // Child, Parent
            typedef std::map<QWidget*, QWidget*> WidgetMap;
            typedef std::map<QWidget*, ShortcutList> ShortcutMap;

            enum MatchResult
            {
                Matches_WithMod,
                Matches_NoMod,
                Matches_Not
            };

            void updateParent(QWidget* widget);

            bool activate(QWidget* widget, unsigned int mod, unsigned int button);

            bool deactivate(QWidget* widget, unsigned int mod, unsigned int button);

            bool checkModifier(unsigned int mod, unsigned int button, Shortcut* shortcut, bool activate);

            MatchResult match(unsigned int mod, unsigned int button, unsigned int value);

            // Prefers Matches_WithMod and a larger number of buttons
            static bool sort(const std::pair<MatchResult, Shortcut*>& left,
                const std::pair<MatchResult, Shortcut*>& right);

            WidgetMap mChildParentRelations;
            ShortcutMap mWidgetShortcuts;

        private slots:

            void widgetDestroyed();
    };
}

#endif

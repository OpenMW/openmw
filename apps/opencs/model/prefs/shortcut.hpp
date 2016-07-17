#ifndef CSM_PREFS_SHORTCUT_H
#define CSM_PREFS_SHORTCUT_H

#include <string>

#include <QKeySequence>
#include <QObject>
#include <QString>

class QKeyEvent;
class QMouseEvent;
class QShortcut;

namespace CSMPrefs
{
    /// A class similar in purpose to QShortcut, but with the ability to use mouse buttons
    class Shortcut : public QObject
    {
            Q_OBJECT

        public:

            Shortcut(const std::string& name, QObject* parent);
            ~Shortcut();

            bool isActive() const;
            bool isEnabled() const;

            const std::string& getName() const;
            const QKeySequence& getSequence() const;
            /// The position in the sequence
            int getPosition() const;
            /// The position in the sequence
            int getLastPosition() const;

            void setSequence(const QKeySequence& sequence);
            /// The position in the sequence
            void setPosition(int pos);

            void activate(bool state);
            void enable(bool state);

            QString toString() const;

        private:

            std::string mName;
            QKeySequence mSequence;
            int mCurrentPos;
            int mLastPos;

            bool mActive;
            bool mEnabled;

        signals:

            /// Triggered when the shortcut is activated or deactived; can be determined from \p state
            void activated(bool state);

            /// Trigger when activated; convenience signal.
            void activated();
    };
}

#endif

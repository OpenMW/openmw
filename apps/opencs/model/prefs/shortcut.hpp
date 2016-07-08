#ifndef CSM_PREFS_SHORTCUT_H
#define CSM_PREFS_SHORTCUT_H

#include <QKeySequence>
#include <QObject>

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

            const std::string& getName() const;
            const QKeySequence& getSequence() const;

            void setSequence(const QKeySequence& sequence);

        private:

            std::string mName;
            QKeySequence mSequence;
            int mCurrentPos;
            int mLastPos;

        public slots:

            void keyPressEvent(QKeyEvent* event);
            void keyReleaseEvent(QKeyEvent* event);
            void mousePressEvent(QMouseEvent* event);
            void mouseReleaseEvent(QMouseEvent* event);

        signals:

            /// Triggered when the shortcut is activated or deactived; can be determined from \p active
            void activated(bool active);
    };

    /// Wraps a QShortcut object so that the sequence can be modified by the settings
    class QShortcutWrapper : public QObject
    {
            Q_OBJECT

        public:

            QShortcutWrapper(const std::string& name, QShortcut* shortcut);
            ~QShortcutWrapper();

            const std::string& getName() const;
            const QKeySequence& getSequence() const;

            void setSequence(const QKeySequence& sequence);

        private:

            std::string mName;
            QKeySequence mSequence;
            QShortcut* mShortcut;
    };
}

#endif

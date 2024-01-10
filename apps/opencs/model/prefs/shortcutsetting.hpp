#ifndef CSM_PREFS_SHORTCUTSETTING_H
#define CSM_PREFS_SHORTCUTSETTING_H

#include <string>
#include <string_view>
#include <utility>

#include <QKeySequence>

#include "setting.hpp"

class QEvent;
class QMutex;
class QObject;
class QPushButton;
class QWidget;

namespace CSMPrefs
{
    class Category;

    class ShortcutSetting final : public TypedSetting<std::string>
    {
        Q_OBJECT

    public:
        explicit ShortcutSetting(
            Category* parent, QMutex* mutex, std::string_view key, const QString& label, Settings::Index& index);

        SettingWidgets makeWidgets(QWidget* parent) override;

        void updateWidget() override;

    protected:
        bool eventFilter(QObject* target, QEvent* event) override;

    private:
        bool handleEvent(QObject* target, int mod, int value, bool active);

        void storeValue(const QKeySequence& sequence);
        void resetState();

        static constexpr int MaxKeys = 4;

        QPushButton* mButton;

        bool mEditorActive;
        int mEditorPos;
        int mEditorKeys[MaxKeys];

    private slots:

        void buttonToggled(bool checked);
    };
}

#endif

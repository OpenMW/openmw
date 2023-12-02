#ifndef CSM_PREFS_SETTING_H
#define CSM_PREFS_SETTING_H

#include <string>
#include <utility>

#include <QObject>

class QWidget;
class QColor;
class QMutex;
class QGridLayout;
class QLabel;

namespace CSMPrefs
{
    class Category;

    struct SettingWidgets
    {
        QLabel* mLabel;
        QWidget* mInput;
        QGridLayout* mLayout;
    };

    class Setting : public QObject
    {
        Q_OBJECT

        Category* mParent;
        QMutex* mMutex;
        std::string mKey;
        QString mLabel;

    protected:
        QMutex* getMutex();

    public:
        Setting(Category* parent, QMutex* mutex, const std::string& key, const QString& label);

        ~Setting() override = default;

        virtual SettingWidgets makeWidgets(QWidget* parent) = 0;

        /// Updates the widget returned by makeWidgets() to the current setting.
        ///
        /// \note If make_widgets() has not been called yet then nothing happens.
        virtual void updateWidget() = 0;

        const Category* getParent() const;

        const std::string& getKey() const;

        const QString& getLabel() const { return mLabel; }

        int toInt() const;

        double toDouble() const;

        std::string toString() const;

        bool isTrue() const;

        QColor toColor() const;
    };

    // note: fullKeys have the format categoryKey/settingKey
    bool operator==(const Setting& setting, const std::string& fullKey);
    bool operator==(const std::string& fullKey, const Setting& setting);
    bool operator!=(const Setting& setting, const std::string& fullKey);
    bool operator!=(const std::string& fullKey, const Setting& setting);
}

#endif

#ifndef CSM_PREFS_SETTING_H
#define CSM_PREFS_SETTING_H

#include <string>
#include <utility>

#include <QObject>

class QWidget;
class QColor;
class QMutex;

namespace Settings
{
    class Manager;
}

namespace CSMPrefs
{
    class Category;

    class Setting : public QObject
    {
            Q_OBJECT

            Category *mParent;
            Settings::Manager *mValues;
            QMutex *mMutex;
            std::string mKey;
            std::string mLabel;

        protected:

            Settings::Manager& getValues();

            QMutex *getMutex();

        public:

            Setting (Category *parent, Settings::Manager *values, QMutex *mutex, const std::string& key, const std::string& label);

            virtual ~Setting();

            /// Return label, input widget.
            ///
            /// \note first can be a 0-pointer, which means that the label is part of the input
            /// widget.
            virtual std::pair<QWidget *, QWidget *> makeWidgets (QWidget *parent);

            /// Updates the widget returned by makeWidgets() to the current setting.
            ///
            /// \note If make_widgets() has not been called yet then nothing happens.
            virtual void updateWidget();

            const Category *getParent() const;

            const std::string& getKey() const;

            const std::string& getLabel() const;

            int toInt() const;

            double toDouble() const;

            std::string toString() const;

            bool isTrue() const;

            QColor toColor() const;
    };

    // note: fullKeys have the format categoryKey/settingKey
    bool operator== (const Setting& setting, const std::string& fullKey);
    bool operator== (const std::string& fullKey, const Setting& setting);
    bool operator!= (const Setting& setting, const std::string& fullKey);
    bool operator!= (const std::string& fullKey, const Setting& setting);
}

#endif

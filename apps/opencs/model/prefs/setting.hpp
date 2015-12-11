#ifndef CSM_PREFS_SETTING_H
#define CSM_PREFS_SETTING_H

#include <string>
#include <utility>

#include <QObject>

class QWidget;

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
            std::string mKey;
            std::string mLabel;

        protected:

            Settings::Manager& getValues();

        public:

            Setting (Category *parent, Settings::Manager *values, const std::string& key, const std::string& label);

            virtual ~Setting();

            /// Return label, input widget.
            ///
            /// \note first can be a 0-pointer, which means that the label is part of the input
            /// widget.
            virtual std::pair<QWidget *, QWidget *> makeWidgets (QWidget *parent);

            const Category *getParent() const;

            const std::string& getKey() const;

            const std::string& getLabel() const;
    };
}

#endif

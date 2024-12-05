#ifndef CSM_PREFS_SETTING_H
#define CSM_PREFS_SETTING_H

#include <string>
#include <utility>

#include <QMutexLocker>
#include <QObject>

#include <components/settings/settingvalue.hpp>

#include "category.hpp"

class QWidget;
class QColor;
class QMutex;
class QGridLayout;
class QLabel;

namespace CSMPrefs
{
    struct SettingWidgets
    {
        QLabel* mLabel;
        QWidget* mInput;
    };

    class Setting : public QObject
    {
        Q_OBJECT

        Category* mParent;
        QMutex* mMutex;
        std::string mKey;
        QString mLabel;
        Settings::Index& mIndex;

    protected:
        QMutex* getMutex();

        template <class T>
        void resetValueImpl()
        {
            QMutexLocker lock(mMutex);
            return mIndex.get<T>(mParent->getKey(), mKey).reset();
        }

        template <class T>
        T getValueImpl() const
        {
            QMutexLocker lock(mMutex);
            return mIndex.get<T>(mParent->getKey(), mKey).get();
        }

        template <class T>
        void setValueImpl(const T& value)
        {
            QMutexLocker lock(mMutex);
            return mIndex.get<T>(mParent->getKey(), mKey).set(value);
        }

    public:
        explicit Setting(
            Category* parent, QMutex* mutex, std::string_view key, const QString& label, Settings::Index& index);

        ~Setting() override = default;

        virtual SettingWidgets makeWidgets(QWidget* parent) = 0;

        /// Updates the widget returned by makeWidgets() to the current setting.
        ///
        /// \note If make_widgets() has not been called yet then nothing happens.
        virtual void updateWidget() = 0;

        virtual void reset() = 0;

        const Category* getParent() const;

        const std::string& getKey() const;

        const QString& getLabel() const { return mLabel; }

        int toInt() const { return getValueImpl<int>(); }

        double toDouble() const { return getValueImpl<double>(); }

        std::string toString() const { return getValueImpl<std::string>(); }

        bool isTrue() const { return getValueImpl<bool>(); }

        QColor toColor() const;
    };

    template <class T>
    class TypedSetting : public Setting
    {
    public:
        using Setting::Setting;

        void reset() final
        {
            resetValueImpl<T>();
            updateWidget();
        }

        T getValue() const { return getValueImpl<T>(); }

        void setValue(const T& value) { return setValueImpl(value); }
    };

    // note: fullKeys have the format categoryKey/settingKey
    bool operator==(const Setting& setting, const std::string& fullKey);
    bool operator==(const std::string& fullKey, const Setting& setting);
    bool operator!=(const Setting& setting, const std::string& fullKey);
    bool operator!=(const std::string& fullKey, const Setting& setting);
}

#endif

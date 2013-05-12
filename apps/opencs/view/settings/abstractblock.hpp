#ifndef ABSTRACTBLOCK_HPP
#define ABSTRACTBLOCK_HPP

#include <QObject>
#include <QList>

#include "settingwidget.hpp"
#include "../../model/settings/settingsitem.hpp"
#include "groupbox.hpp"

namespace CSVSettings
{

    class AbstractBlock : public QObject
    {
        Q_OBJECT

    protected:

        typedef QMap<QString, CSMSettings::SettingsItem*> SettingsItemMap;
        GroupBox *mBox;
        QWidget *mWidgetParent;

    public:

        explicit AbstractBlock (QWidget *parent = 0);
        explicit AbstractBlock (bool isVisible, QWidget *parent = 0);

        QGroupBox *getGroupBox();
        void setVisible (bool isVisible);
        bool isVisible() const;

        virtual CSMSettings::SettingList *getSettings() = 0;
        virtual bool updateSettings (const CSMSettings::SettingMap &settings) = 0;
        virtual bool updateBySignal (const QString &name, const QString &value, bool &doEmit)
        { return false; }

    protected:

        QLayout *createLayout (Orientation direction, bool isZeroMargin, QWidget* parent = 0);
        AbstractWidget *buildWidget (const QString &widgetName, WidgetDef &wDef,
                                     QLayout *layout = 0, bool isConnected = true) const;

        template <typename T>
        AbstractWidget *createSettingWidget (WidgetDef &wDef, QLayout *layout) const
        {
            return new SettingWidget<T> (wDef, layout, mBox);
        }

        QWidget *getParent() const;

    public slots:

        void slotSetEnabled (bool value);
        void slotUpdateSetting (const QString &settingName, const QString &settingValue);

    private slots:

        void slotUpdate (const QString &value);

    signals:

        //signal to functions outside the settings tab widget
        void signalUpdateSetting (const QString &propertyName, const QString &propertyValue);
        void signalUpdateWidget (const QString & value);

        //propertyName and propertyValue are for properties for which the updated setting acts as a proxy
        void signalUpdateProxySetting (const QString &propertyName, const QString &propertyValue);
    };
}
#endif // ABSTRACTBLOCK_HPP

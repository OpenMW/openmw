#ifndef ABSTRACTBLOCK_HPP
#define ABSTRACTBLOCK_HPP

#include <QObject>
#include <QList>

#include "settingwidget.hpp"
#include "../../model/settings/settingsitem.hpp"
#include "groupbox.hpp"

namespace CSVSettings
{

    /// Abstract base class for all blocks
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

        /// update settings found in the passed map and are encapsulated by the block
        virtual bool updateSettings (const CSMSettings::SettingMap &settings) = 0;

        /// update callback function called from update slot
        /// used for updating application-level settings in the editor
        virtual bool updateBySignal (const QString &name, const QString &value, bool &doEmit)
        { return false; }

    protected:

        /// Creates the layout for the block's QGroupBox
        QLayout *createLayout (Orientation direction, bool isZeroMargin, QWidget* parent = 0);

        /// Creates widgets that exist as direct children of the block
        AbstractWidget *buildWidget (const QString &widgetName, WidgetDef &wDef,
                                     QLayout *layout = 0, bool isConnected = true) const;

        QWidget *getParent() const;

    public slots:

        /// enables / disables block-level widgets based on signals from other widgets
        /// used in ToggleBlock
        void slotSetEnabled (bool value);

        /// receives updates to applicaion-level settings in the Editor
        void slotUpdateSetting (const QString &settingName, const QString &settingValue);

    private slots:

        /// receives updates to a setting in the block pushed from the application level
        void slotUpdate (const QString &value);

    signals:

        /// signal to UserSettings instance
        void signalUpdateSetting (const QString &propertyName, const QString &propertyValue);

        /// signal to widget for updating widget value
        void signalUpdateWidget (const QString & value);

        /// ProxyBlock use only.
        /// Name and value correspond to settings for which the block is a proxy.
        void signalUpdateProxySetting (const QString &propertyName, const QString &propertyValue);
    };
}
#endif // ABSTRACTBLOCK_HPP

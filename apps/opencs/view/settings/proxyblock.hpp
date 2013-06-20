#ifndef PROXYBLOCK_HPP
#define PROXYBLOCK_HPP

#include "groupblock.hpp"

namespace CSVSettings
{
    class ProxyBlock : public GroupBlock
    {
        Q_OBJECT

        /// TODO:  Combine mProxyItemBlockList and mProxyList.
        ItemBlockList mProxiedItemBlockList;
        ProxyList mProxyList;
        QStringList *mValueList;

    public:

        explicit ProxyBlock (QWidget *parent = 0);
        explicit ProxyBlock (ItemBlock *proxyItemBlock, QWidget *parent = 0);

        /// Add a block that contains a proxied setting to the proxy block.
        void addSetting (ItemBlock* settingBlock, QStringList *proxyList);

        int build (GroupBlockDef *def);

        CSMSettings::SettingList *getSettings()  { return 0; }

        /// Update settings local to the proxy block pushed from application level
        bool updateSettings (const CSMSettings::SettingMap &settings);

        /// callback function triggered when update to the application level is signaled.
        bool updateBySignal (const QString &name, const QString &value, bool &doEmit);

    private:

        /// return the item block of a proxied setting
        ItemBlock *getProxiedItemBlock (const QString &name);

        /// update the proxy setting with data from the proxied settings
        bool updateByProxiedSettings(const CSMSettings::SettingMap *settings = 0);

        /// update proxied settings with data from the proxy setting
        bool updateProxiedSettings();

    private slots:

        void slotUpdateProxySetting (const QString &name, const QString &value);

    };
}
#endif // PROXYBLOCK_HPP

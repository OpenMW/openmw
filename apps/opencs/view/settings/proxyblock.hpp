#ifndef PROXYBLOCK_HPP
#define PROXYBLOCK_HPP

#include "groupblock.hpp"

namespace CSVSettings
{
    class ProxyBlock : public GroupBlock
    {
        Q_OBJECT

        //NOTE:  mProxyItemBlockList and mProxyList
        //should be combined into a value pair and stored in one list.
        ItemBlockList mProxiedItemBlockList;
        ProxyList mProxyList;
        QStringList *mValueList;

    public:

        explicit ProxyBlock (QWidget *parent = 0);
        explicit ProxyBlock (ItemBlock *proxyItemBlock, QWidget *parent = 0);

        void addSetting (ItemBlock* settingBlock, QStringList *proxyList);
        int build (GroupBlockDef &def);

        CSMSettings::SettingList *getSettings()  { return 0; }
        bool updateSettings (const CSMSettings::SettingMap &settings);
        bool updateBySignal (const QString &name, const QString &value, bool &doEmit);

    private:

        ItemBlock *getProxiedItemBlock (const QString &name);
        bool updateByProxiedSettings(const CSMSettings::SettingMap *settings = 0);
        bool updateProxiedSettings();

    private slots:

        void slotUpdateProxySetting (const QString &name, const QString &value);

    };
}
#endif // PROXYBLOCK_HPP

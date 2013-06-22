#include "proxyblock.hpp"
#include "itemblock.hpp"

CSVSettings::ProxyBlock::ProxyBlock (QWidget *parent)
    : GroupBlock (parent)
{
}
int CSVSettings::ProxyBlock::build (GroupBlockDef *proxyDef)
{
    //get the list of pre-defined values for the proxy
    mValueList = proxyDef->settingItems.at(0)->valueList;

    bool success = GroupBlock::build(proxyDef);

    //connect the item block of the proxy setting to the proxy-update slot
    connect (getItemBlock(0), SIGNAL (signalUpdateSetting(const QString &, const QString &)),
             this, SLOT (slotUpdateProxySetting (const QString &, const QString &)));

    return success;
}

void CSVSettings::ProxyBlock::addSetting (ItemBlock *settingBlock, QStringList *proxyList)
{
    //connect the item block of the proxied seting to the generic update slot
    connect (settingBlock, SIGNAL (signalUpdateSetting(const QString &, const QString &)),
             this, SLOT (slotUpdateProxySetting(const QString &, const QString &)));

    mProxiedItemBlockList << settingBlock;
    mProxyList << proxyList;
}

bool CSVSettings::ProxyBlock::updateSettings (const CSMSettings::SettingMap &settings)
{
    return updateByProxiedSettings(&settings);
}

bool CSVSettings::ProxyBlock::updateBySignal(const QString &name, const QString &value, bool &doEmit)
{
    doEmit = false;
    return updateProxiedSettings();
}

void CSVSettings::ProxyBlock::slotUpdateProxySetting (const QString &name, const QString &value)
{
    updateByProxiedSettings();
}

bool CSVSettings::ProxyBlock::updateProxiedSettings()
{
    foreach (ItemBlock *block, mProxiedItemBlockList)
    {
        QString value = getItemBlock(0)->getValue();

        bool success = false;
        int i = 0;

        //find the value index of the selected value in the proxy setting
        for (; i < mValueList->size(); ++i)
        {
            success =  (value == mValueList->at(i));

            if (success)
                break;
        }

        if (!success)
            return false;

        // update the containing the proxied item's name
        foreach (QStringList *list, mProxyList)
        {
            if ( list->at(0) == block->objectName())
                block->update (list->at(++i));
        }
    }

    return true;
}

bool CSVSettings::ProxyBlock::updateByProxiedSettings(const CSMSettings::SettingMap *settings)
{
    bool success = false;
    int commonIndex = -1;

    //update all proxy settings based on values from non-proxies
    foreach (QStringList *list, mProxyList)
    {
        //Iterate each proxy item's proxied setting list, getting the current values
        //Compare those value indices.
        //If indices match, they correlate to one of the proxy's values in it's value list

        //first value is always the name of the setting the proxy setting manages
        QStringList::Iterator itProxyValue = list->begin();
        QString proxiedSettingName = (*itProxyValue);
        QString proxiedSettingValue = "";
        itProxyValue++;

        if (!settings)
        {
            //get the actual setting value
            ItemBlock *block = getProxiedItemBlock (proxiedSettingName);

            if (block)
                proxiedSettingValue = block->getValue();
        }
        else
            proxiedSettingValue = (*settings)[proxiedSettingName]->getValue();

        int j = 0;

        //iterate each value in the proxy string list
        for (; itProxyValue != (list)->end(); ++itProxyValue)
        {
            success = ((*itProxyValue) == proxiedSettingValue);

            if (success)
                break;

            j++;
        }

        //break if no match was found
        if ( !success )
            break;

        if (commonIndex != -1)
            success = (commonIndex == j);
        else
            commonIndex = j;

        //break if indices were found, but mismatch
        if (!success)
            break;
    }

    //if successful, the proxied setting values match a pre-defined value in the
    //proxy's value list.  Set the proxy to that value index
    if (success)
    {
        ItemBlock *block = getItemBlock(0);

        if (block)
            block->update (mValueList->at(commonIndex));
    }

    return success;
}

CSVSettings::ItemBlock *CSVSettings::ProxyBlock::getProxiedItemBlock (const QString &name)
{
    return getItemBlock (name, &mProxiedItemBlockList);
}

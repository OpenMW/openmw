#include "setting.hpp"
#include "support.hpp"

CSMSettings::Setting::Setting()
{
    buildDefaultSetting();
}

CSMSettings::Setting::Setting(SettingType typ, const QString &settingName,
                             const QString &pageName, const QStringList &values)
    : mIsEditorSetting (false)
{
    buildDefaultSetting();

    int vType = static_cast <int> (typ);

    if ((vType % 2) == 0)
        setProperty (Property_IsMultiValue,
                                QVariant(true).toString());
    else
        vType--;

    setProperty (Property_ViewType, QVariant (vType / 2).toString());
    setProperty (Property_Page, pageName);
    setProperty (Property_Name, settingName);
    setProperty (Property_DeclaredValues, values);
}

void CSMSettings::Setting::buildDefaultSetting()
{
    int arrLen = sizeof(sPropertyDefaults) / sizeof (*sPropertyDefaults);

    for (int i = 0; i < arrLen; i++)
    {
        QStringList propertyList;

        if (i <Property_DefaultValues)
            propertyList.append (sPropertyDefaults[i]);

        mProperties.append (propertyList);
    }
}

void CSMSettings::Setting::addProxy (const Setting *setting,
                                     const QStringList &vals)
{
    if (serializable())
        setSerializable (false);

    QList <QStringList> list;

    foreach  (const QString &val, vals)
        list << (QStringList() << val);

    mProxies [setting->page() + '.' + setting->name()] = list;
}

void CSMSettings::Setting::addProxy (const Setting *setting,
                                     const QList <QStringList> &list)
{
    if (serializable())
        setProperty (Property_Serializable, false);

    mProxies [setting->page() + '.' + setting->name()] = list;
}

void CSMSettings::Setting::setColumnSpan (int value)
{
    setProperty (Property_ColumnSpan, value);
}

int CSMSettings::Setting::columnSpan() const
{
    return property (Property_ColumnSpan).at(0).toInt();
}

QStringList CSMSettings::Setting::declaredValues() const
{
    return property (Property_DeclaredValues);
}

void CSMSettings::Setting::setDefinedValues (QStringList list)
{
    setProperty (Property_DefinedValues, list);
}

QStringList CSMSettings::Setting::definedValues() const
{
    return property (Property_DefinedValues);
}

QStringList CSMSettings::Setting::property (SettingProperty prop) const
{
    if (prop >= mProperties.size())
        return QStringList();

    return mProperties.at(prop);
}

void CSMSettings::Setting::setDefaultValue (const QString &value)
{
    setDefaultValues (QStringList() << value);
}

void CSMSettings::Setting::setDefaultValues (const QStringList &values)
{
    setProperty (Property_DefaultValues, values);
}

QStringList CSMSettings::Setting::defaultValues() const
{
    return property (Property_DefaultValues);
}

void CSMSettings::Setting::setDelimiter (const QString &value)
{
    setProperty (Property_Delimiter, value);
}

QString CSMSettings::Setting::delimiter() const
{
    return property (Property_Delimiter).at(0);
}

void CSMSettings::Setting::setEditorSetting(bool state)
{
    mIsEditorSetting = true;
}

bool CSMSettings::Setting::isEditorSetting() const
{
    return mIsEditorSetting;
}
void CSMSettings::Setting::setIsMultiLine (bool state)
{
    setProperty (Property_IsMultiLine, state);
}

bool CSMSettings::Setting::isMultiLine() const
{
    return (property (Property_IsMultiLine).at(0) == "true");
}

void CSMSettings::Setting::setIsMultiValue (bool state)
{
    setProperty (Property_IsMultiValue, state);
}

bool CSMSettings::Setting::isMultiValue() const
{
    return (property (Property_IsMultiValue).at(0) == "true");
}

const CSMSettings::ProxyValueMap &CSMSettings::Setting::proxyLists() const
{
    return mProxies;
}

void CSMSettings::Setting::setSerializable (bool state)
{
    setProperty (Property_Serializable, state);
}

bool CSMSettings::Setting::serializable() const
{
    return (property (Property_Serializable).at(0) == "true");
}

void CSMSettings::Setting::setName (const QString &value)
{
    setProperty (Property_Name, value);
}

QString CSMSettings::Setting::name() const
{
    return property (Property_Name).at(0);
}

void CSMSettings::Setting::setPage (const QString &value)
{
    setProperty (Property_Page, value);
}

QString CSMSettings::Setting::page() const
{
    return property (Property_Page).at(0);
}

void CSMSettings::Setting::setRowSpan (const int value)
{
    setProperty (Property_RowSpan, value);
}

int CSMSettings::Setting::rowSpan () const
{
    return property (Property_RowSpan).at(0).toInt();
}

void CSMSettings::Setting::setViewType (int vType)
{
    setProperty (Property_ViewType, vType);
}

CSVSettings::ViewType CSMSettings::Setting::viewType() const
{
    return static_cast <CSVSettings::ViewType>
                                    (property(Property_ViewType).at(0).toInt());
}

void CSMSettings::Setting::setViewColumn (int value)
{
    setProperty (Property_ViewColumn, value);
}

int CSMSettings::Setting::viewColumn() const
{
    return property (Property_ViewColumn).at(0).toInt();
}

void CSMSettings::Setting::setViewLocation (int row, int column)
{
    setViewRow (row);
    setViewColumn (column);
}

void CSMSettings::Setting::setViewRow (int value)
{
    setProperty (Property_ViewRow, value);
}

int CSMSettings::Setting::viewRow() const
{
    return property (Property_ViewRow).at(0).toInt();
}

void CSMSettings::Setting::setWidgetWidth (int value)
{
    setProperty (Property_WidgetWidth, value);
}

int CSMSettings::Setting::widgetWidth() const
{
    return property (Property_WidgetWidth).at(0).toInt();
}
void CSMSettings::Setting::setProperty (SettingProperty prop, bool value)
{
    setProperty (prop, QStringList() << QVariant (value).toString());
}

void CSMSettings::Setting::setProperty (SettingProperty prop, int value)
{
    setProperty (prop, QStringList() << QVariant (value).toString());
}

void CSMSettings::Setting::setProperty (SettingProperty prop,
                                                        const QString &value)
{
    setProperty (prop, QStringList() << value);
}

void CSMSettings::Setting::setProperty (SettingProperty prop,
                                                    const QStringList &value)
{
    if (prop < mProperties.size())
        mProperties.replace (prop, value);
}

QDataStream &operator <<(QDataStream &stream, const CSMSettings::Setting& setting)
{
    stream << setting.properties();

    stream << setting.proxies();
    return stream;
}

QDataStream &operator >>(QDataStream& stream, CSMSettings::Setting& setting)
{
  //  stream >> setting.properties();
  //  stream >> setting.proxies();
    return stream;
}

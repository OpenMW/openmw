#include "setting.hpp"
#include "support.hpp"

CSMSettings::Setting::Setting(SettingType typ, const QString &settingName,
    const QString &pageName, const QString& label)
: mIsEditorSetting (true)
{
    buildDefaultSetting();

    int settingType = static_cast <int> (typ);

    //even-numbered setting types are multi-valued
    if ((settingType % 2) == 0)
        setProperty (Property_IsMultiValue, QVariant(true).toString());

    //view type is related to setting type by an order of magnitude
    setProperty (Property_SettingType, QVariant (settingType).toString());
    setProperty (Property_Page, pageName);
    setProperty (Property_Name, settingName);
    setProperty (Property_Label, label.isEmpty() ? settingName : label);
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

    mProxies [setting->page() + '/' + setting->name()] = list;
}

void CSMSettings::Setting::addProxy (const Setting *setting,
                                     const QList <QStringList> &list)
{
    if (serializable())
        setProperty (Property_Serializable, false);

    mProxies [setting->page() + '/' + setting->name()] = list;
}

void CSMSettings::Setting::setColumnSpan (int value)
{
    setProperty (Property_ColumnSpan, value);
}

int CSMSettings::Setting::columnSpan() const
{
    return property (Property_ColumnSpan).at(0).toInt();
}

void CSMSettings::Setting::setDeclaredValues (QStringList list)
{
    setProperty (Property_DeclaredValues, list);
}

QStringList CSMSettings::Setting::declaredValues() const
{
    return property (Property_DeclaredValues);
}

QStringList CSMSettings::Setting::property (SettingProperty prop) const
{
    if (prop >= mProperties.size())
        return QStringList();

    return mProperties.at(prop);
}

void CSMSettings::Setting::setDefaultValue (int value)
{
    setDefaultValues (QStringList() << QVariant (value).toString());
}

void CSMSettings::Setting::setDefaultValue (double value)
{
    setDefaultValues (QStringList() << QVariant (value).toString());
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

void CSMSettings::Setting::setSpecialValueText(const QString &text)
{
    setProperty (Property_SpecialValueText, text);
}

QString CSMSettings::Setting::specialValueText() const
{
    return property (Property_SpecialValueText).at(0);
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

void CSMSettings::Setting::setStyleSheet (const QString &value)
{
    setProperty (Property_StyleSheet, value);
}

QString CSMSettings::Setting::styleSheet() const
{
    return property (Property_StyleSheet).at(0);
}

void CSMSettings::Setting::setPrefix (const QString &value)
{
    setProperty (Property_Prefix, value);
}

QString CSMSettings::Setting::prefix() const
{
    return property (Property_Prefix).at(0);
}

void CSMSettings::Setting::setRowSpan (const int value)
{
    setProperty (Property_RowSpan, value);
}

int CSMSettings::Setting::rowSpan () const
{
    return property (Property_RowSpan).at(0).toInt();
}

void CSMSettings::Setting::setSingleStep (int value)
{
    setProperty (Property_SingleStep, value);
}

void CSMSettings::Setting::setSingleStep (double value)
{
    setProperty (Property_SingleStep, value);
}

QString CSMSettings::Setting::singleStep() const
{
    return property (Property_SingleStep).at(0);
}

void CSMSettings::Setting::setSuffix (const QString &value)
{
    setProperty (Property_Suffix, value);
}

QString CSMSettings::Setting::suffix() const
{
    return property (Property_Suffix).at(0);
}

void CSMSettings::Setting::setTickInterval (int value)
{
    setProperty (Property_TickInterval, value);
}

int CSMSettings::Setting::tickInterval () const
{
    return property (Property_TickInterval).at(0).toInt();
}

void CSMSettings::Setting::setTicksAbove (bool state)
{
    setProperty (Property_TicksAbove, state);
}

bool CSMSettings::Setting::ticksAbove() const
{
    return (property (Property_TicksAbove).at(0) == "true");
}

void CSMSettings::Setting::setTicksBelow (bool state)
{
    setProperty (Property_TicksBelow, state);
}

bool CSMSettings::Setting::ticksBelow() const
{
    return (property (Property_TicksBelow).at(0) == "true");
}

void CSMSettings::Setting::setType (int settingType)
{
    setProperty (Property_SettingType, settingType);
}

CSMSettings::SettingType CSMSettings::Setting::type() const
{
    return static_cast <CSMSettings::SettingType> ( property (
                                        Property_SettingType).at(0).toInt());
}

void CSMSettings::Setting::setRange (int min, int max)
{
    setProperty (Property_Minimum, min);
    setProperty (Property_Maximum, max);
}

void CSMSettings::Setting::setRange (double min, double max)
{
    setProperty (Property_Minimum, min);
    setProperty (Property_Maximum, max);
}

QString CSMSettings::Setting::maximum() const
{
    return property (Property_Maximum).at(0);
}

QString CSMSettings::Setting::minimum() const
{
    return property (Property_Minimum).at(0);
}

CSVSettings::ViewType CSMSettings::Setting::viewType() const
{
    return static_cast <CSVSettings::ViewType> ( property (
                                    Property_SettingType).at(0).toInt() / 10);
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

void CSMSettings::Setting::setWrapping (bool state)
{
    setProperty (Property_Wrapping, state);
}

bool CSMSettings::Setting::wrapping() const
{
    return (property (Property_Wrapping).at(0) == "true");
}

void CSMSettings::Setting::setLabel (const QString& label)
{
    setProperty (Property_Label, label);
}

QString CSMSettings::Setting::getLabel() const
{
    return property (Property_Label).at (0);
}

void CSMSettings::Setting::setToolTip (const QString& toolTip)
{
    setProperty (Property_ToolTip, toolTip);
}

QString CSMSettings::Setting::getToolTip() const
{
    return property (Property_ToolTip).at (0);
}

void CSMSettings::Setting::setProperty (SettingProperty prop, bool value)
{
    setProperty (prop, QStringList() << QVariant (value).toString());
}

void CSMSettings::Setting::setProperty (SettingProperty prop, int value)
{
    setProperty (prop, QStringList() << QVariant (value).toString());
}

void CSMSettings::Setting::setProperty (SettingProperty prop, double value)
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

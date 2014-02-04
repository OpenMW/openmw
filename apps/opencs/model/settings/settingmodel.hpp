#ifndef CSMSETTINGS_SETTINGMODEL_HPP
#define CSMSETTINGS_SETTINGMODEL_HPP

#include <QStandardItemModel>

#include "settingitem.hpp"
#include "setting.hpp"
#include "../../view/settings/support.hpp"

namespace CSMSettings
{
    class SettingModel : public QStandardItemModel
    {
        Q_OBJECT

    public:
        explicit SettingModel(QObject *parent = 0);

        ///returns a list of SettingItems for an existing model row
        Setting *setting (int row);

        ///boilerplate conveneince functions for creating model rows
        ///for specific setting properties
        Setting *declareSingleText (const QString &name,
                                            const QString &page,
                                            const QString &defaultValue)
        { return declareSetting (Type_SingleText, name, page, defaultValue); }

        Setting *declareMultiText  (const QString &name,
                                            const QString &page,
                                            const QString &defaultValue)
        { return declareSetting (Type_MultiText, name, page, defaultValue); }

        Setting *declareSingleBool (const QString &name,
                                            const QString &page,
                                            const QString &defaultValue)
        { return declareSetting (Type_SingleBool, name, page, defaultValue); }

        Setting *declareMultiBool  (const QString &name,
                                            const QString &page,
                                            const QString &defaultValue)
        { return declareSetting (Type_MultiBool, name, page, defaultValue); }

        Setting *declareSingleRange (const QString &name,
                                             const QString &page,
                                             const QString &defaultValue)
        { return declareSetting (Type_SingleRange, name, page, defaultValue); }

        Setting *declareMultiRange (const QString &name,
                                            const QString &page,
                                            const QString &defaultValue)
        { return declareSetting (Type_SingleRange, name, page, defaultValue); }

        Setting *declareSingleList (const QString &name,
                                            const QString &page,
                                            const QString &defaultValue)
        { return declareSetting (Type_SingleList, name, page, defaultValue); }

        Setting *declareMultiList (const QString &name,
                                           const QString &page,
                                           const QString &defaultValue)
        { return declareSetting (Type_SingleList, name, page, defaultValue); }

        ///create a new setting (append a row and return a list of SettingItems)
        Setting *declareSetting (SettingType typ,
                                             const QString &name,
                                             const QString &page,
                                             const QString &defaultValue);

    signals:

    public slots:

    };
}
#endif // CSMSETTINGS_SETTINGMODEL_HPP

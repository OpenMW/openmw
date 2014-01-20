#ifndef CSMSETTINGS_DECLARATIONMODEL_HPP
#define CSMSETTINGS_DECLARATIONMODEL_HPP

#include <QAbstractItemModel>

#include "../../view/settings/support.hpp"

namespace CSMSettings
{
    class Setting;

    class DeclarationModel : public QAbstractItemModel
    {
        Q_OBJECT

        DeclarationList mDeclarations;

    public:
        explicit DeclarationModel(QObject *parent = 0);

        int rowCount (const QModelIndex &parent = QModelIndex()) const;
        int columnCount (const QModelIndex &parent = QModelIndex()) const;

        QVariant data (const QModelIndex &index, int role) const;

        Qt::ItemFlags flags (const QModelIndex &index) const;

        QModelIndex index(int row, int column,
                          const QModelIndex &parent = QModelIndex()) const;

        QModelIndex parent(const QModelIndex &child) const
        { return QModelIndex(); }

        Setting *getSetting (const QString &pageName,
                             const QString &settingName) const;

        Setting *getSetting (int row) const;

        Setting *multiText(
                const QString &pageName, const QString &settingName,
                const QString &defaultValue = "", const QString &inputMask = "",
                bool isHorizontal = true);

        Setting *singleText(
                const QString &pageName, const QString &settingName,
                const QString &defaultValue = "", const QString &inputMask = "",
                bool isHorizontal = true);
/*
        Setting *range(
                const QString &pageName, const QString &settingName,
                const QString &defaultValue, bool isHorizontal = true);
*/
        Setting *multiList (
                const QString &pageName, const QString &settingName,
                const QStringList &valueList, const QString &defaultValue = "",
                bool isHorizontal = true);

        Setting *singleList (
                const QString &pageName, const QString &settingName,
                const QStringList &valueList, const QString &defaultValue = "",
                bool isHorizontal = true);

        Setting *singleBool (
                const QString &pageName, const QString &settingName,
                const QStringList &valueList, const QString &defaultValue = "",
                bool isHorizontal = true);

        Setting *multiBool (
                const QString &pageName, const QString &settingName,
                const QStringList &valueList, const QString &defaultValue = "",
                bool isHorizontal = true);

        Setting *declareSetting (
                CSVSettings::ViewType viewType, bool isMultiValue,
                const QString &pageName, const QString &settingName,
                const QStringList &valueList, const QString &defaultValue,
                const QString &inputMask, bool isHorizontal);
    };
}
#endif // CSMSETTINGS_DECLARATIONMODEL_HPP

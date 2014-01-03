#ifndef SETTINGMODEL_HPP
#define SETTINGMODEL_HPP

#include <QAbstractItemModel>
#include <QSortFilterProxyModel>
#include <QList>

#include "setting.hpp"

namespace CSMSettings
{
    class SettingData : public QObject
    {
        QString mSection;
        QString mValue;

        Q_OBJECT

    public:
        SettingData (const QString &section, const QString &name,
                     QObject *parent = 0)
            : mSection (section), QObject(parent)
        { setObjectName(name); }

        void setValue (const QString &value)    { mValue = value; }

        QString value () const                  { return mValue;  }

        QString section() const                 { return mSection; }
        QString name() const                    { return objectName(); }
    };

    //pair of setting name and setting value
    typedef QPair <QString, QString>        SettingValuePair;

    //pair of section name and setting name / value pair
    typedef QPair <QString, SettingValuePair>   SectionSettingPair;

    typedef QList <SectionSettingPair>          SettingDefinitionList;
    typedef QMap <QString, Setting *>           SettingDeclarationList;

    class SettingModel : public QAbstractItemModel
    {

        Q_OBJECT

        SettingDeclarationList mSettings;
        SettingDefinitionList mSettingValues;

    public:

        explicit SettingModel(QObject *parent);

        int rowCount (const QModelIndex &parent = QModelIndex()) const;
        int columnCount (const QModelIndex &parent = QModelIndex()) const;
        QVariant data (const QModelIndex &index, int role) const;

        bool setData (const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);

        Qt::ItemFlags flags (const QModelIndex &index) const;

        QModelIndex index(int row, int column, const QModelIndex &parent) const;

        QModelIndex parent(const QModelIndex &child) const
        { return QModelIndex(); }

        bool insertRows(int row, int count, const QModelIndex &parent);
        bool removeRows(int row, int count, const QModelIndex &parent);

        Setting *declareSetting (const QString &settingName,
                                 const QString &sectionName,
                                 const QString &value);

        QModelIndex defineSetting ( const QString &settingName,
                                    const QString &sectionName,
                                    const QString &value);

        QModelIndex createSetting (const QString &name, const QString &section,
                                   const QString &value, const Setting *settingS);

        bool hasDeclaredSetting (const QString &key) const;
        QString getSettingValue (int row) const;

        QString getSettingValue (const QString &sectionName, const QString &settingName) const;

        QStringList getSettingValueList
                (const QString &sectionName, const QString &settingName) const;

        SectionSettingPair getSectionSettingPair(int index) const;

        void validate();

    private:


    signals:

    public slots:

    };
}
#endif // SETTINGMODEL_HPP

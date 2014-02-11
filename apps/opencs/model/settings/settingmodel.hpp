#ifndef CSMSETTINGS_SETTINGMODEL_HPP
#define CSMSETTINGS_SETTINGMODEL_HPP

#include <QStandardItemModel>
#include <QSortFilterProxyModel>

#include "settingitem.hpp"
#include "setting.hpp"
#include "../../view/settings/support.hpp"

class QTextStream;

namespace CSMSettings
{
    typedef QMap <QString, QStringList *> SettingMap;
    typedef QMap <QString, SettingMap *> PageMap;

    class WriterSortModel : public QSortFilterProxyModel
    {
        bool lessThan(const QModelIndex &rLeft, const QModelIndex &rRight) const
        {
            int const leftRow  = rLeft.row();
            int const rightRow = rRight.row();

            for (int column = Property_Page; column >= Property_Name; --column)
            {
                QModelIndex const leftIdx = sourceModel()->index
                                    (leftRow, column, QModelIndex());

                QModelIndex const rightIdx = sourceModel()->index
                                    (rightRow, column, QModelIndex());

                QString const leftData = sourceModel()->
                                                    data(leftIdx).toString();

                QString const rightData = sourceModel()->
                                                    data(rightIdx).toString();

                int const compare = QString::localeAwareCompare
                                                        (leftData, rightData);
                if (compare != 0)
                    return (compare < 0);
            }
            return false;
        }
    };

    class SettingModel : public QStandardItemModel
    {
        Q_OBJECT

        QString mReadOnlyMessage;
        QString mReadWriteMessage;

    public:
        explicit SettingModel(QObject *parent = 0);

        ///returns a list of SettingItems for an existing model row
        Setting *setting (int row);

        Setting *setting (const QString &page, const QString &name);

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

        bool defineSetting (const QString &page, const QString &name,
                                                        const QString &value);

    protected:

        void validate(PageMap &pageMap);

        PageMap readFilestream(QTextStream *stream);

        bool writeFilestream (QTextStream *stream);

        void mergeSettings (PageMap &destMap, PageMap &srcMap,
                            MergeMethod mergeMethod = Merge_Accept);

        QTextStream *openFilestream (const QString &filePath,
                                     bool isReadOnly) const;

        void destroyStream(QTextStream *stream) const;

        void displayFileErrorMessage(const QString &message,
                                     bool isReadOnly) const;

        void buildModel (PageMap &pageMap);

    private:

        void removeUndeclaredDefinitions (PageMap &pageMap);
        void validateDefinitions (PageMap &pageMap);

    signals:

    public slots:

    };
}
#endif // CSMSETTINGS_SETTINGMODEL_HPP

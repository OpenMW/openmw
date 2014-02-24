#ifndef CSMSETTNIGS_SETTINGMANAGER_HPP
#define CSMSETTINGS_SETTINGMANAGER_HPP

#include <QObject>
#include <QStandardItemModel>
#include <QMap>
#include <QStringList>
#include <QTextStream>
#include <QSortFilterProxyModel>

#include "../../view/settings/support.hpp"
#include "setting.hpp"

namespace CSMSettings
{

    typedef QMap <QString, QStringList *> SettingMap;
    typedef QMap <QString, SettingMap *> PageMap;
    typedef QList <RowItemList *> RowList;

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

    class SettingManager : public QObject
    {
        Q_OBJECT

        QStandardItemModel mSettingModel;
        QString mReadOnlyMessage;
        QString mReadWriteMessage;

    public:
        explicit SettingManager(QObject *parent = 0);

        QStandardItemModel &model()     { return mSettingModel; }

        Setting getSetting
                        (const QString &pageName, const QString &settingName);

        QList <Setting> getSettings (const QString &pageName);

    protected:

        void addDeclaration (Setting *setting);
        void addDefinition (const QString &settingName, const QString &pageName,
                            const QString &value);

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

        ///Returns a list of model rows as QStandardItems which match the text
        ///in the indicated column exactly from the provided source list.
        RowList *findSettings
            (RowList *source, const QString &text, int column);

        /// Returns an entire row of QStandardItems based on the matched
        /// values of text and column.  Duplicated rows are returned only once.
        /// Searches the entire model for matches.
        RowList *findSettings
            (const QString &text, int column);

        RowItemList* findSetting
                        (const QString &settingName, const QString &pageName);

        RowItemList *findSetting (int row) const;

        QList <QStandardItem *> buildItemList (const QStringList &list) const;


    signals:

    public slots:

    };
}
#endif // CSMSETTINGS_SETTINGMANAGER_HPP

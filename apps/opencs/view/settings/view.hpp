#ifndef CSVSETTINGS_VIEW_HPP
#define CSVSETTINGS_VIEW_HPP

#include <QWidget>
#include <QMap>
#include <QItemSelectionModel>
#include <QAbstractItemModel>
#include <QList>

#include "support.hpp"

class QGroupBox;
class QStringList;
class QStandardItem;

namespace CSMSettings
{
    class Setting;
    class Selector;
}

namespace CSVSettings
{
    class SettingBox;
    class Page;

    class View : public QWidget
    {
        Q_OBJECT

        Page *mParentPage;
        SettingBox *mViewFrame;

        CSMSettings::Setting *mSetting;
        QItemSelectionModel *mSelectionModel;
        QAbstractItemModel *mDataModel;

        bool mHasDeclaredValues;

    public:

        explicit View (CSMSettings::Setting *setting,
                       Page *parent);

        CSMSettings::Setting *setting() const              { return mSetting; }
        QStringList selectedValues() const;
        void setSelectedValues (const QStringList &values,
                                bool doViewUpdate = true);

        SettingBox *viewFrame() const                    { return mViewFrame; }

    protected:

        QAbstractItemModel *dataModel()             { return mDataModel; }
        QItemSelectionModel *selectionModel()       { return mSelectionModel;}

        void showEvent ( QShowEvent * event );
        virtual void updateView() const;

    private:
        void buildView();
        void buildModel();

        void refresh() const;

        void select (const QItemSelection &selection) const;

        bool stringListsMatch (const QStringList &list1,
                               const QStringList &list2) const;

        QList <QStandardItem *> toStandardItemList(const QStringList &) const;

        QString value (int row) const;

    signals:
        void viewUpdated() const;

    };

    class IViewFactory
    {
    public:
        virtual View *createView (CSMSettings::Setting *setting,
                                  Page *parent) = 0;
    };
}
#endif // CSVSETTINGS_VIEW_HPP

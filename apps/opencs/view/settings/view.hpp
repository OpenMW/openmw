#ifndef CSVSETTINGS_VIEW_HPP
#define CSVSETTINGS_VIEW_HPP

#include <QWidget>
#include <QList>

#include "frame.hpp"
#include "../../model/settings/support.hpp"

class QGroupBox;
class QStringList;
class QStandardItem;
class QItemSelection;
class QAbstractItemModel;
class QItemSelectionModel;

namespace CSMSettings { class Setting; }

namespace CSVSettings
{
    class Page;

    class View : public Frame
    {
        Q_OBJECT

        ///Pointer to the owning Page instance
        Page *mParentPage;

        ///Pointer to the selection model for the view
        QItemSelectionModel *mSelectionModel;

        ///Pointer to the data model for the view's selection model
        QAbstractItemModel *mDataModel;

        ///State indicating whether or not the setting has a pre-defined list
        ///of values, limiting possible definitions
        bool mHasFixedValues;

        ///State indicating whether the view will allow multiple values
        bool mIsMultiValue;

        ///'pagename.settingname' form of the view's id
        QString mViewKey;

        ///indicates whether or not the setting is written to file
        bool mSerializable;

    public:

        explicit View (CSMSettings::Setting *setting, Page *parent);

        ///Returns the index / row of the passed value, -1 if not found.
        int currentIndex () const;

        ///Returns the number of rows in the view's data model
        int rowCount() const;

        ///Returns bool indicating the data in this view should / should not
        ///be serialized to a config file
        bool serializable() const                     { return mSerializable; }

        ///Returns a pointer to the view's owning parent page
        const Page *parentPage() const                  { return mParentPage; }

        ///Returns the selected items in the selection model as a QStringList
        QStringList selectedValues() const;

        ///Sets the selected items in the selection model based on passed list.
        ///Bools allow opt-out of updating the view
        ///or signaling the view was updatedto avoid viscious cylcing.
        void setSelectedValues (const QStringList &values,
                                bool updateView = true,
                                bool signalUpdate = true) const;

        void setSelectedValue (const QString &value,
                               bool updateView = true,
                               bool signalUpdate = true);


        ///Returns the value of the data model at the specified row
        QString value (int row) const;

        QString viewKey() const                     { return mViewKey; }

    protected:

        /// Returns the model which provides data for the selection model
        QAbstractItemModel *dataModel()             { return mDataModel; }

        ///Accessor function for subclasses
        bool isMultiValue()                         { return mIsMultiValue; }

        ///Returns the view selection model
        QItemSelectionModel *selectionModel()       { return mSelectionModel;}

        ///Global callback for basic view initialization
        void showEvent ( QShowEvent * event );

        ///Virtual for updating a specific View subclass
        ///bool indicates whether viewUpdated() signal is emitted
        virtual void updateView (bool signalUpdate = true) const;

        ///Returns the pixel width corresponding to the specified number of
        ///characters.
        int widgetWidth(int characterCount) const;

    private:

        ///Constructs the view layout
        void buildView();

        ///Constructs the data and selection models
        void buildModel (const CSMSettings::Setting *setting);

        ///In cases where the view has a pre-defined list of possible values,
        ///a QStringListModel is created using those values.
        ///View changes operate on the selection model only.
        void buildFixedValueModel (const QStringList &definitions);

        ///In cases where the view does not have a pre-defined list of possible
        ///values, a QStandardItemModel is created, containing the actual
        ///setting definitions.  View changes first update the data in the
        ///model to match the data in the view.  The selection model always
        ///selects all values.
        void buildUpdatableValueModel (const QStringList &definitions);

        ///Refreshes the view
        void refresh() const;

        ///Convenince function for selection model's select() method.  Also
        ///clears out the model beforehand to ensure complete selection.
        void select (const QItemSelection &selection) const;

        ///Compares two string lists "loosely", ensuring that all values in
        ///one list are contained entirely in the other, and that neither list
        ///has more values than the other. List order is not considered.
        bool stringListsMatch (const QStringList &list1,
                               const QStringList &list2) const;

        ///Converts a string list to a list of QStandardItem pointers.
        QList <QStandardItem *> toStandardItemList(const QStringList &) const;

    signals:

        ///Signals that the view has been changed.
        void viewUpdated(const QString &, const QStringList &) const;

    };

    class IViewFactory
    {
    public:

        ///Creation interface for view factories
        virtual View *createView (CSMSettings::Setting *setting,
                                  Page *parent) = 0;
    };
}
#endif // CSVSETTINGS_VIEW_HPP

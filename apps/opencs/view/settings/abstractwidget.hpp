#ifndef ABSTRACTWIDGET_HPP
#define ABSTRACTWIDGET_HPP

#include <QDebug>
#include "support.hpp"

class QLayout;
class QAbstractTableModel;

namespace CSMSettings { class Setting; }

class QSortFilterProxyModel;
class QDataWidgetMapper;
class QWidget;

namespace CSVSettings
{
    /// Abstract base class for widgets which are used in user preferences dialog
    class AbstractWidget : public QObject
    {
        Q_OBJECT

        QLayout *mLayout;
        QDataWidgetMapper *mDataAdapter;
        QSortFilterProxyModel *mFilterProxy;
        QWidget *mWidget;

    public:

        /// Passed layout is assigned the constructed widget.
        explicit AbstractWidget (const QString &name, QSortFilterProxyModel *model, QLayout *layout, QWidget* parent);

        /// retrieve layout for insertion into itemblock
        QLayout *layout() { return mLayout; }

        /// create the derived widget instance
        void build (Orientation orient, Alignment align, const QString &caption = "");

        /// Virtual function to determine wheter or not a setting widget
        /// can / should handle multiple selections
        virtual bool isMultiSelect() { return false; }

        /// reference to the derived widget instance
        virtual QWidget *widget() { return mWidget; }

    protected:

        /// Converts user-defined enum to Qt equivalents
        QFlags<Qt::AlignmentFlag> getAlignment (Alignment flag);

        /// Assigns the widget created in the template class and finishes
        /// connecting the data adapter
        void setWidget (QWidget *widget, int column = 0);

    private:

        /// Creates layout and assigns label and widget as appropriate
        void createLayout (Orientation direction);

        /// Reconstructs the setting object for a model item
        CSMSettings::Setting *buildSetting (QSortFilterProxyModel *settingModel);
    };
}
#endif // ABSTRACTWIDGET_HPP

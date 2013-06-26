#ifndef ABSTRACTWIDGET_HPP
#define ABSTRACTWIDGET_HPP

#include <QWidget>
#include "support.hpp"

class QLayout;

namespace CSVSettings
{
    /// Abstract base class for widgets which are used in user preferences dialog
    class AbstractWidget : public QObject
    {
        Q_OBJECT

        QLayout *mLayout;

    public:

        /// Passed layout is assigned the constructed widget.
        /// if no layout is passed, one is created.
        explicit AbstractWidget (QLayout *layout = 0, QWidget* parent = 0)
            : QObject (parent), mLayout (layout)
        {}

        /// retrieve layout for insertion into itemblock
        QLayout *getLayout();

        /// create the derived widget instance
        void build (QWidget* widget, WidgetDef &def, bool noLabel = false);

        /// reference to the derived widget instance
        virtual QWidget *widget() = 0;

    protected:

        /// Callback called by receiving slot for widget udpates
        virtual void updateWidget (const QString &value) = 0;

        /// Converts user-defined enum to Qt equivalents
        QFlags<Qt::AlignmentFlag> getAlignment (Alignment flag);

    private:

        /// Creates layout and assigns label and widget as appropriate
        void createLayout (Orientation direction, bool isZeroMargin);

        /// Creates label and widget according to passed definition
        void buildLabelAndWidget (QWidget *widget, WidgetDef &def, bool noLabel);


    signals:

        /// outbound update signal
        void signalUpdateItem (const QString &value);

    public slots:

        /// receives inbound updates
        void slotUpdateWidget (const QString &value);

        /// Overloads for outbound updates from derived widget signal
        void slotUpdateItem  (const QString &value);
        void slotUpdateItem  (bool value);
        void slotUpdateItem  (int value);
        void slotUpdateItem (QListWidgetItem* current, QListWidgetItem* previous);
    };
}
#endif // ABSTRACTWIDGET_HPP

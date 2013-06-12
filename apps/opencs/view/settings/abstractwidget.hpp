#ifndef ABSTRACTWIDGET_HPP
#define ABSTRACTWIDGET_HPP

#include <QWidget>
#include "support.hpp"

class QLayout;

namespace CSVSettings
{
    class AbstractWidget : public QObject
    {
        Q_OBJECT

        QLayout *mLayout;

    public:

        explicit AbstractWidget (QLayout *layout = 0, QWidget* parent = 0)
            : QObject (parent), mLayout (layout)
        {}

        //retrieve layout for insertion into itemblock
        QLayout *getLayout();

        //create the derived widget instance
        void build (QWidget* widget, WidgetDef &def, bool noLabel = false);

        //reference to the derived widget instance
        virtual QWidget *widget() = 0;

    protected:

        //called by inbound signal for type-specific widget udpates
        virtual void updateWidget (const QString &value) = 0;

        //converts user-defined enum to Qt equivalents
        QFlags<Qt::AlignmentFlag> getAlignment (Alignment flag);

    private:

        //widget initialization utilities
        void createLayout (Orientation direction, bool isZeroMargin);
        void buildLabelAndWidget (QWidget *widget, WidgetDef &def, bool noLabel);


    signals:

        //outbound update
        void signalUpdateItem (const QString &value);

    public slots:

        //inbound updates
        void slotUpdateWidget (const QString &value);

        //Outbound updates from derived widget signal
        void slotUpdateItem  (const QString &value);
        void slotUpdateItem  (bool value);
        void slotUpdateItem  (int value);
        void slotUpdateItem (QListWidgetItem* current, QListWidgetItem* previous);
    };
}
#endif // ABSTRACTWIDGET_HPP

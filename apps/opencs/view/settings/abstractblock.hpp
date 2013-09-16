#ifndef ABSTRACTBLOCK_HPP
#define ABSTRACTBLOCK_HPP

#include <QObject>
#include <QList>

#include "settingwidget.hpp"

namespace CSVSettings
{

    class GroupBox;
    /// Abstract base class for all blocks
    class AbstractBlock : public QObject
    {
        Q_OBJECT

    protected:

     //   GroupBox *mBox;
        QWidget *mWidgetParent;

    public:

        explicit AbstractBlock (QWidget *parent = 0);
        explicit AbstractBlock (bool isVisible, QWidget *parent = 0);

     //   QGroupBox *getGroupBox();
        void setVisible (bool isVisible);
        bool isVisible() const;

    protected:

        /// Creates the layout for the block's QGroupBox
    //    QLayout *createLayout (Orientation direction, QWidget* parent = 0);

        /// Creates widgets that exist as direct children of the block
        //AbstractWidget *buildWidget (const QString &widgetName, WidgetDef &wDef,
       //                              QLayout *layout = 0, bool isConnected = true) const;

        QWidget *getParent() const;

    public slots:

        /// enables / disables block-level widgets based on signals from other widgets
        /// used in ToggleBlock
     //   void slotSetEnabled (bool value);
    };
}
#endif // ABSTRACTBLOCK_HPP

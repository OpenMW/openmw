#ifndef CSVSETTINGS_FRAME_HPP
#define CSVSETTINGS_FRAME_HPP

#include <QSizePolicy>
#include <QGroupBox>
#include <QGridLayout>
#include "../../model/settings/support.hpp"

namespace CSVSettings
{
    class SettingLayout : public QGridLayout
    {
    public:
        explicit SettingLayout (QWidget *parent = 0)
            : QGridLayout (parent)
        {
          setContentsMargins(0,0,0,0);
          setAlignment(Qt::AlignLeft | Qt::AlignTop);
        }
    };

    /// Custom implementation of QGroupBox to act as a base for view classes
    class Frame : public QGroupBox
    {
        static const QString sInvisibleBoxStyle;

        QString mVisibleBoxStyle;

        bool mIsHorizontal;

        SettingLayout *mLayout;

    public:
        explicit Frame (bool isVisible, const QString &title = "",
                                                        QWidget *parent = 0);

        ///Adds a widget to the grid layout, setting the position
        ///relative to the last added widgets, or absolutely for positive
        ///row / column values
        void addWidget (QWidget *widget, int row = -1, int column = -1,
                        int rowSpan = 1, int columnSpan = 1);

        ///Force the grid to lay out in horizontal or vertical alignments
        void setHLayout()           { mIsHorizontal = true; }
        void setVLayout()           { mIsHorizontal = false; }

        ///show / hide widgets (when stacked widget page changes)
        void showWidgets();
        void hideWidgets();

    private:

        ///functions which return the index for the next layout row / column
        int getNextColumn() const;
        int getNextRow() const;

    };
}

#endif // CSVSETTINGS_FRAME_HPP

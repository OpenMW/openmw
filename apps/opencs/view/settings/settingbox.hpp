#ifndef SETTINGBOX_HPP
#define SETTINGBOX_HPP

#include <QGroupBox>
#include <QGridLayout>
#include "support.hpp"

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

    /// Custom implementation of QGroupBox to be used with block classes
    class SettingBox : public QGroupBox
    {
        static const QString INVISIBLE_BOX_STYLE;
        QString mVisibleBoxStyle;
        SettingLayout *mLayout;
        bool mIsHorizontal;

    public:
        explicit SettingBox (bool isVisible, const QString &title = "",
                                                        QWidget *parent = 0);

        void addWidget (QWidget *widget, int row, int column);
        void addWidget (QWidget *widget);

        void setHLayout()           { mIsHorizontal = true; }
        void setVLayout()           { mIsHorizontal = false; }

    private:
        void setMinimumWidth();
    };
}

#endif // SETTINGBOX_HPP

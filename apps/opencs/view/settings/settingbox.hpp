#ifndef SETTINGBOX_HPP
#define SETTINGBOX_HPP

#include <QGroupBox>
#include "support.hpp"

namespace CSVSettings
{
    /// Custom implementation of QGroupBox to be used with block classes
    class SettingBox : public QGroupBox
    {
        static const QString INVISIBLE_BOX_STYLE;
        QString mVisibleBoxStyle;

    public:
        explicit SettingBox (bool isHorizontal, bool isVisible, QWidget *parent = 0);

        void setTitle (const QString &title);
        void setBorderVisibility (bool value);
        bool borderVisibile() const;

    private:
        void setMinimumWidth();
    };
}

#endif // SETTINGBOX_HPP

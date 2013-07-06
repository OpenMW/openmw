#ifndef GROUPBOX_HPP
#define GROUPBOX_HPP

#include <QGroupBox>

namespace CSVSettings
{
    /// Custom implementation of QGroupBox to be used with block classes
    class GroupBox : public QGroupBox
    {
        static const QString INVISIBLE_BOX_STYLE;
        QString VISIBLE_BOX_STYLE;                  //not a const...

    public:
        explicit GroupBox (QWidget *parent = 0);
        explicit GroupBox (bool isVisible, QWidget *parent = 0);

        void setTitle (const QString &title);
        void setBorderVisibility (bool value);
        bool borderVisibile() const;

    private:
        void setMinimumWidth();
        void initBox(bool isVisible = true);
    };
}

#endif // GROUPBOX_HPP

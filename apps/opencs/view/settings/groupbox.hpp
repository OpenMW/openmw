#ifndef GROUPBOX_HPP
#define GROUPBOX_HPP

#include <QGroupBox>

namespace CsSettings
{
    class GroupBox : public QGroupBox
    {
        Q_OBJECT

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

#ifndef TEXTINPUTDIALOG_HPP
#define TEXTINPUTDIALOG_HPP

#include <QDialog>
//#include "lineedit.hpp"

class QDialogButtonBox;
<<<<<<< Updated upstream
class LineEdit;

class TextInputDialog : public QDialog
{
    Q_OBJECT
public:
    explicit TextInputDialog(const QString& title, const QString &text, QWidget *parent = 0);
    inline LineEdit *lineEdit() { return mLineEdit; }
    void setOkButtonEnabled(bool enabled);

    LineEdit *mLineEdit;
=======

namespace Launcher
{
    class TextInputDialog : public QDialog
    {
        Q_OBJECT

        class DialogLineEdit : public LineEdit
        {
        public:
            explicit DialogLineEdit (QWidget *parent = 0);
        };

        DialogLineEdit *mLineEdit;
        QDialogButtonBox *mButtonBox;

    public:

        explicit TextInputDialog(const QString& title, const QString &text, QWidget *parent = 0);
        ~TextInputDialog () {}

        QString getText() const;
>>>>>>> Stashed changes

        int exec();

<<<<<<< Updated upstream
private:
    QDialogButtonBox *mButtonBox;

    
};
=======
    private slots:
        void slotUpdateOkButton(QString text);

    };
}
>>>>>>> Stashed changes

#endif // TEXTINPUTDIALOG_HPP

#ifndef TEXTINPUTDIALOG_HPP
#define TEXTINPUTDIALOG_HPP

#include <QDialog>
//#include "lineedit.hpp"

class QDialogButtonBox;
class LineEdit;

class TextInputDialog : public QDialog
{
    Q_OBJECT
public:
    explicit TextInputDialog(const QString& title, const QString &text, QWidget *parent = 0);
    inline LineEdit *lineEdit() { return mLineEdit; }
    void setOkButtonEnabled(bool enabled);

    LineEdit *mLineEdit;

    int exec();

private:
    QDialogButtonBox *mButtonBox;

    
};

#endif // TEXTINPUTDIALOG_HPP

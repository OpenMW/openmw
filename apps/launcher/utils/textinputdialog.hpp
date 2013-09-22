#ifndef TEXTINPUTDIALOG_HPP
#define TEXTINPUTDIALOG_HPP

#include <QDialog>

class QDialogButtonBox;

namespace ContentSelectorView {
    class LineEdit;
}


class TextInputDialog : public QDialog
{
    Q_OBJECT
public:
    explicit TextInputDialog(const QString& title, const QString &text, QWidget *parent = 0);
    inline ContentSelectorView::LineEdit *lineEdit() { return mLineEdit; }
    void setOkButtonEnabled(bool enabled);

    ContentSelectorView::LineEdit *mLineEdit;

    int exec();

private:
    QDialogButtonBox *mButtonBox;

    
};

#endif // TEXTINPUTDIALOG_HPP

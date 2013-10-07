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

    ContentSelectorView::LineEdit *mLineEdit;
    QDialogButtonBox *mButtonBox;

public:

    explicit TextInputDialog(const QString& title, const QString &text, QWidget *parent = 0);
    QString getText() const;

    int exec();

private slots:
    void slotUpdateOkButton(QString text);
    
};

#endif // TEXTINPUTDIALOG_HPP

#ifndef TEXTINPUTDIALOG_HPP
#define TEXTINPUTDIALOG_HPP

#include <QDialog>

#include "lineedit.hpp"

class QDialogButtonBox;

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

        int exec();

    private slots:
        void slotUpdateOkButton(QString text);

    };
}

#endif // TEXTINPUTDIALOG_HPP

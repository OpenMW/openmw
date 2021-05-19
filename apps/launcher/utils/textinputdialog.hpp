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

    public:

        explicit TextInputDialog(const QString& title, const QString &text, QWidget *parent = nullptr);
        ~TextInputDialog () override;

        inline LineEdit *lineEdit() { return mLineEdit; }
        void setOkButtonEnabled(bool enabled);

        int exec() override;

    private:

        QDialogButtonBox *mButtonBox;
        LineEdit *mLineEdit;

    };
}

#endif // TEXTINPUTDIALOG_HPP

/****************************************************************************
**
** Copyright (c) 2007 Trolltech ASA <info@trolltech.com>
**
** Use, modification and distribution is allowed without limitation,
** warranty, liability or support of any kind.
**
****************************************************************************/

#ifndef LINEEDIT_H
#define LINEEDIT_H

#include <QLineEdit>

class QToolButton;

class ComboBoxLineEdit : public QLineEdit
{
    Q_OBJECT

public:
    ComboBoxLineEdit(QWidget *parent = 0);

protected:
    void resizeEvent(QResizeEvent *);

private slots:
    void updateClearButton(const QString &text);

private:
    QToolButton *mClearButton;
};

#endif // LIENEDIT_H


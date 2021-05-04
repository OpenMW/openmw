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
#include <QStylePainter>
#include <QToolButton>

class QToolButton;

class LineEdit : public QLineEdit
{
    Q_OBJECT

    QString mPlaceholderText;

public:
    LineEdit(QWidget *parent = nullptr);

protected:
    void resizeEvent(QResizeEvent *) override;

private slots:
    void updateClearButton(const QString &text);

protected:
    QToolButton *mClearButton;

    void setupClearButton();
};

#endif // LIENEDIT_H


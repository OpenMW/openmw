/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef CHECKABLEMESSAGEBOX_HPP
#define CHECKABLEMESSAGEBOX_HPP

#include <QDialogButtonBox>
#include <QMessageBox>
#include <QDialog>

class CheckableMessageBoxPrivate;

class CheckableMessageBox : public QDialog
{
    Q_OBJECT
    Q_PROPERTY(QString text READ text WRITE setText)
    Q_PROPERTY(QPixmap iconPixmap READ iconPixmap WRITE setIconPixmap)
    Q_PROPERTY(bool isChecked READ isChecked WRITE setChecked)
    Q_PROPERTY(QString checkBoxText READ checkBoxText WRITE setCheckBoxText)
    Q_PROPERTY(QDialogButtonBox::StandardButtons buttons READ standardButtons WRITE setStandardButtons)
    Q_PROPERTY(QDialogButtonBox::StandardButton defaultButton READ defaultButton WRITE setDefaultButton)

public:
    explicit CheckableMessageBox(QWidget *parent);
    virtual ~CheckableMessageBox();

    static QDialogButtonBox::StandardButton
        question(QWidget *parent,
                 const QString &title,
                 const QString &question,
                 const QString &checkBoxText,
                 bool *checkBoxSetting,
                 QDialogButtonBox::StandardButtons buttons = QDialogButtonBox::Yes|QDialogButtonBox::No,
                 QDialogButtonBox::StandardButton defaultButton = QDialogButtonBox::No);

    QString text() const;
    void setText(const QString &);

    bool isChecked() const;
    void setChecked(bool s);

    QString checkBoxText() const;
    void setCheckBoxText(const QString &);

    bool isCheckBoxVisible() const;
    void setCheckBoxVisible(bool);

    QDialogButtonBox::StandardButtons standardButtons() const;
    void setStandardButtons(QDialogButtonBox::StandardButtons s);
    QPushButton *button(QDialogButtonBox::StandardButton b) const;
    QPushButton *addButton(const QString &text, QDialogButtonBox::ButtonRole role);

    QDialogButtonBox::StandardButton defaultButton() const;
    void setDefaultButton(QDialogButtonBox::StandardButton s);

    // See static QMessageBox::standardPixmap()
    QPixmap iconPixmap() const;
    void setIconPixmap (const QPixmap &p);

    // Query the result
    QAbstractButton *clickedButton() const;
    QDialogButtonBox::StandardButton clickedStandardButton() const;

    // Conversion convenience
    static QMessageBox::StandardButton dialogButtonBoxToMessageBoxButton(QDialogButtonBox::StandardButton);

private slots:
    void slotClicked(QAbstractButton *b);

private:
    CheckableMessageBoxPrivate *d;
};

#endif // CHECKABLEMESSAGEBOX_HPP

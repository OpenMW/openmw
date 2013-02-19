#include <QDebug>
#include <QVariant>

#include <QPushButton>
#include <QAction>
#include <QApplication>
#include <QButtonGroup>
#include <QCheckBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QSpacerItem>
#include <QVBoxLayout>

#include "checkablemessagebox.hpp"


/*
    class CheckableMessageBox
    Modified from the one used in Qt Creator

    A messagebox suitable for questions with a
    "Do not ask me again" checkbox.

    Emulates the QMessageBox API with
    static conveniences. The message label can open external URLs.
*/


class CheckableMessageBoxPrivate
{
public:
    CheckableMessageBoxPrivate(QDialog *q)
        : clickedButton(0)
    {
        QSizePolicy sizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);

        pixmapLabel = new QLabel(q);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(pixmapLabel->sizePolicy().hasHeightForWidth());
        pixmapLabel->setSizePolicy(sizePolicy);
        pixmapLabel->setVisible(false);

        QSpacerItem *pixmapSpacer =
                new QSpacerItem(0, 5, QSizePolicy::Minimum, QSizePolicy::MinimumExpanding);

        messageLabel = new QLabel(q);
        messageLabel->setMinimumSize(QSize(300, 0));
        messageLabel->setWordWrap(true);
        messageLabel->setOpenExternalLinks(true);
        messageLabel->setTextInteractionFlags(Qt::LinksAccessibleByKeyboard|Qt::LinksAccessibleByMouse);

        QSpacerItem *checkBoxRightSpacer =
                new QSpacerItem(1, 1, QSizePolicy::Expanding, QSizePolicy::Minimum);
        QSpacerItem *buttonSpacer =
                new QSpacerItem(0, 1, QSizePolicy::Minimum, QSizePolicy::Minimum);

        checkBox = new QCheckBox(q);
        checkBox->setText(CheckableMessageBox::tr("Do not ask again"));

        buttonBox = new QDialogButtonBox(q);
        buttonBox->setOrientation(Qt::Horizontal);
        buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);

        QVBoxLayout *verticalLayout = new QVBoxLayout();
        verticalLayout->addWidget(pixmapLabel);
        verticalLayout->addItem(pixmapSpacer);

        QHBoxLayout *horizontalLayout_2 = new QHBoxLayout();
        horizontalLayout_2->addLayout(verticalLayout);
        horizontalLayout_2->addWidget(messageLabel);

        QHBoxLayout *horizontalLayout = new QHBoxLayout();
        horizontalLayout->addWidget(checkBox);
        horizontalLayout->addItem(checkBoxRightSpacer);

        QVBoxLayout *verticalLayout_2 = new QVBoxLayout(q);
        verticalLayout_2->addLayout(horizontalLayout_2);
        verticalLayout_2->addLayout(horizontalLayout);
        verticalLayout_2->addItem(buttonSpacer);
        verticalLayout_2->addWidget(buttonBox);
    }

    QLabel *pixmapLabel;
    QLabel *messageLabel;
    QCheckBox *checkBox;
    QDialogButtonBox *buttonBox;
    QAbstractButton *clickedButton;
};

CheckableMessageBox::CheckableMessageBox(QWidget *parent) :
    QDialog(parent),
    d(new CheckableMessageBoxPrivate(this))
{
    setModal(true);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    connect(d->buttonBox, SIGNAL(accepted()), SLOT(accept()));
    connect(d->buttonBox, SIGNAL(rejected()), SLOT(reject()));
    connect(d->buttonBox, SIGNAL(clicked(QAbstractButton*)),
            SLOT(slotClicked(QAbstractButton*)));
}

CheckableMessageBox::~CheckableMessageBox()
{
    delete d;
}

void CheckableMessageBox::slotClicked(QAbstractButton *b)
{
    d->clickedButton = b;
}

QAbstractButton *CheckableMessageBox::clickedButton() const
{
    return d->clickedButton;
}

QDialogButtonBox::StandardButton CheckableMessageBox::clickedStandardButton() const
{
    if (d->clickedButton)
        return d->buttonBox->standardButton(d->clickedButton);
    return QDialogButtonBox::NoButton;
}

QString CheckableMessageBox::text() const
{
    return d->messageLabel->text();
}

void CheckableMessageBox::setText(const QString &t)
{
    d->messageLabel->setText(t);
}

QPixmap CheckableMessageBox::iconPixmap() const
{
    if (const QPixmap *p = d->pixmapLabel->pixmap())
        return QPixmap(*p);
    return QPixmap();
}

void CheckableMessageBox::setIconPixmap(const QPixmap &p)
{
    d->pixmapLabel->setPixmap(p);
    d->pixmapLabel->setVisible(!p.isNull());
}

bool CheckableMessageBox::isChecked() const
{
    return d->checkBox->isChecked();
}

void CheckableMessageBox::setChecked(bool s)
{
    d->checkBox->setChecked(s);
}

QString CheckableMessageBox::checkBoxText() const
{
    return d->checkBox->text();
}

void CheckableMessageBox::setCheckBoxText(const QString &t)
{
    d->checkBox->setText(t);
}

bool CheckableMessageBox::isCheckBoxVisible() const
{
    return d->checkBox->isVisible();
}

void CheckableMessageBox::setCheckBoxVisible(bool v)
{
    d->checkBox->setVisible(v);
}

QDialogButtonBox::StandardButtons CheckableMessageBox::standardButtons() const
{
    return d->buttonBox->standardButtons();
}

void CheckableMessageBox::setStandardButtons(QDialogButtonBox::StandardButtons s)
{
    d->buttonBox->setStandardButtons(s);
}

QPushButton *CheckableMessageBox::button(QDialogButtonBox::StandardButton b) const
{
    return d->buttonBox->button(b);
}

QPushButton *CheckableMessageBox::addButton(const QString &text, QDialogButtonBox::ButtonRole role)
{
    return d->buttonBox->addButton(text, role);
}

QDialogButtonBox::StandardButton CheckableMessageBox::defaultButton() const
{
    foreach (QAbstractButton *b, d->buttonBox->buttons())
        if (QPushButton *pb = qobject_cast<QPushButton *>(b))
            if (pb->isDefault())
                return d->buttonBox->standardButton(pb);
    return QDialogButtonBox::NoButton;
}

void CheckableMessageBox::setDefaultButton(QDialogButtonBox::StandardButton s)
{
    if (QPushButton *b = d->buttonBox->button(s)) {
        b->setDefault(true);
        b->setFocus();
    }
}

QDialogButtonBox::StandardButton
CheckableMessageBox::question(QWidget *parent,
                              const QString &title,
                              const QString &question,
                              const QString &checkBoxText,
                              bool *checkBoxSetting,
                              QDialogButtonBox::StandardButtons buttons,
                              QDialogButtonBox::StandardButton defaultButton)
{
    CheckableMessageBox mb(parent);
    mb.setWindowTitle(title);
    mb.setIconPixmap(QMessageBox::standardIcon(QMessageBox::Question));
    mb.setText(question);
    mb.setCheckBoxText(checkBoxText);
    mb.setChecked(*checkBoxSetting);
    mb.setStandardButtons(buttons);
    mb.setDefaultButton(defaultButton);
    mb.exec();
    *checkBoxSetting = mb.isChecked();
    return mb.clickedStandardButton();
}

QMessageBox::StandardButton CheckableMessageBox::dialogButtonBoxToMessageBoxButton(QDialogButtonBox::StandardButton db)
{
    return static_cast<QMessageBox::StandardButton>(int(db));
}

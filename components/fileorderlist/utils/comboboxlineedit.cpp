#include <QToolButton>
#include <QStyle>

#include "comboboxlineedit.hpp"

ComboBoxLineEdit::ComboBoxLineEdit(QWidget *parent)
    : QLineEdit(parent)
{
    mClearButton = new QToolButton(this);
    QPixmap pixmap(":images/clear.png");
    mClearButton->setIcon(QIcon(pixmap));
    mClearButton->setIconSize(pixmap.size());
    mClearButton->setCursor(Qt::ArrowCursor);
    mClearButton->setStyleSheet("QToolButton { border: none; padding: 0px; }");
    mClearButton->hide();
    connect(mClearButton, SIGNAL(clicked()), this, SLOT(clear()));
    connect(this, SIGNAL(textChanged(const QString&)), this, SLOT(updateClearButton(const QString&)));
    int frameWidth = style()->pixelMetric(QStyle::PM_DefaultFrameWidth);

    setObjectName(QString("ComboBoxLineEdit"));
    setStyleSheet(QString("ComboBoxLineEdit { background-color: transparent; padding-right: %1px; } ").arg(mClearButton->sizeHint().width() + frameWidth + 1));
}

void ComboBoxLineEdit::resizeEvent(QResizeEvent *)
{
    QSize sz = mClearButton->sizeHint();
    int frameWidth = style()->pixelMetric(QStyle::PM_DefaultFrameWidth);
    mClearButton->move(rect().right() - frameWidth - sz.width(),
                      (rect().bottom() + 1 - sz.height())/2);
}

void ComboBoxLineEdit::updateClearButton(const QString& text)
{
    mClearButton->setVisible(!text.isEmpty());
}

#include <QToolButton>
#include <QStyle>

#include "lineedit.hpp"

LineEdit::LineEdit(QWidget *parent)
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

    setObjectName(QString("LineEdit"));
    setStyleSheet(QString("LineEdit { padding-right: %1px; } ").arg(mClearButton->sizeHint().width() + frameWidth + 1));
    QSize msz = minimumSizeHint();
    setMinimumSize(qMax(msz.width(), mClearButton->sizeHint().height() + frameWidth * 2 + 2),
                   qMax(msz.height(), mClearButton->sizeHint().height() + frameWidth * 2 + 2));
}

void LineEdit::resizeEvent(QResizeEvent *)
{
    QSize sz = mClearButton->sizeHint();
    int frameWidth = style()->pixelMetric(QStyle::PM_DefaultFrameWidth);
    mClearButton->move(rect().right() - frameWidth - sz.width(),
                      (rect().bottom() + 1 - sz.height())/2);
}

void LineEdit::updateClearButton(const QString& text)
{
    mClearButton->setVisible(!text.isEmpty());
}

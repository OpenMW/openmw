
#include "editwidget.hpp"

CSVFilter::EditWidget::EditWidget (QWidget *parent)
: QLineEdit (parent)
{
    mPalette = palette();
    connect (this, SIGNAL (textChanged (const QString&)), this, SLOT (textChanged (const QString&)));
}

void CSVFilter::EditWidget::textChanged (const QString& text)
{
    if (mParser.parse (text.toUtf8().constData()))
    {
        setPalette (mPalette);
        emit filterChanged (mParser.getFilter(), "");
    }
    else
    {
        QPalette palette (mPalette);
        palette.setColor (QPalette::Text, Qt::red);
        setPalette (palette);

        /// \todo improve error reporting; mark only the faulty part
    }
}
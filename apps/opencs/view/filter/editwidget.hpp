#ifndef CSV_FILTER_EDITWIDGET_H
#define CSV_FILTER_EDITWIDGET_H

#include <QLineEdit>

#include "../../model/filter/parser.hpp"

namespace CSVFilter
{
    class EditWidget : public QLineEdit
    {
            Q_OBJECT

            CSMFilter::Parser mParser;

        public:

            EditWidget (QWidget *parent = 0);

        signals:

            void filterChanged();

        private slots:

            void textChanged (const QString& text);
    };
}

#endif

#ifndef CSV_FILTER_EDITWIDGET_H
#define CSV_FILTER_EDITWIDGET_H

#include <boost/shared_ptr.hpp>

#include <QLineEdit>
#include <QPalette>

#include "../../model/filter/parser.hpp"
#include "../../model/filter/node.hpp"

namespace CSMWorld
{
    class Data;
}

namespace CSVFilter
{
    class EditWidget : public QLineEdit
    {
            Q_OBJECT

            CSMFilter::Parser mParser;
            QPalette mPalette;

        public:

            EditWidget (const CSMWorld::Data& data, QWidget *parent = 0);

        signals:

            void filterChanged (boost::shared_ptr<CSMFilter::Node> filter);

        private slots:

            void textChanged (const QString& text);
    };
}

#endif

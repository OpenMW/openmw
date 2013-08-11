#ifndef CSV_FILTER_RECORDFILTERBOX_H
#define CSV_FILTER_RECORDFILTERBOX_H

#include <QWidget>

#include <QHBoxLayout>

namespace CSVFilter
{
    class RecordFilterBox : public QWidget
    {
            Q_OBJECT

        public:

            RecordFilterBox (QWidget *parent = 0);
    };

}

#endif
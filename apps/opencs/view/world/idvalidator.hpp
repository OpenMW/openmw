#ifndef CSV_WORLD_IDVALIDATOR_H
#define CSV_WORLD_IDVALIDATOR_H

#include <QValidator>

namespace CSVWorld
{
    class IdValidator : public QValidator
    {
        private:

            bool isValid (const QChar& c, bool first) const;

        public:

            IdValidator (QObject *parent = 0);

            virtual State validate (QString& input, int& pos) const;

    };
}

#endif

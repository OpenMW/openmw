#ifndef CSV_WORLD_IDVALIDATOR_H
#define CSV_WORLD_IDVALIDATOR_H

#include <string>

#include <QValidator>

namespace CSVWorld
{
    class IdValidator : public QValidator
    {
            bool mRelaxed;
            std::string mNamespace;

        private:

            bool isValid (const QChar& c, bool first) const;

        public:

            IdValidator (bool relaxed = false, QObject *parent = 0);
            ///< \param relaxed Relaxed rules for IDs that also functino as user visible text

            virtual State validate (QString& input, int& pos) const;

            void setNamespace (const std::string& namespace_);

    };
}

#endif

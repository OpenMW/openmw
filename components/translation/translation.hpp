#ifndef COMPONENTS_TRANSLATION_DATA_H
#define COMPONENTS_TRANSLATION_DATA_H

#include <components/files/collections.hpp>
#include <components/toutf8/toutf8.hpp>

namespace Translation
{
    class Storage
    {
    public:
        Storage();

        void loadTranslationData(const Files::Collections& dataFileCollections, std::string_view esmFileName);

        std::string_view translateCellName(std::string_view cellName) const;

        // Standard form usually means nominative case
        std::string_view topicStandardForm(std::string_view phrase) const;

        // The phrase that will act as the hyperlink for the given topic ID
        std::string_view topicKeyword(std::string_view phrase) const;

        // Manual population for testing
        void addPhraseForm(std::string_view phrase, std::string_view topicId);

        void setEncoder(ToUTF8::Utf8Encoder* encoder);

    private:
        typedef std::map<std::string, std::string, std::less<>> ContainerType;

        void loadData(ContainerType& container, std::string_view fileNameNoExtension, std::string_view extension,
            const Files::Collections& dataFileCollections);

        void loadDataFromStream(ContainerType& container, std::istream& stream);

        ToUTF8::Utf8Encoder* mEncoder;
        ContainerType mCellNamesTranslations, mKeywords, mPhraseForms;
    };
}

#endif

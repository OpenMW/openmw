#ifndef COMPONENT_ESM_READER_H
#define COMPONENT_ESM_READER_H

#include <vector>

#include <components/to_utf8/to_utf8.hpp>

#include "common.hpp" // MasterData

namespace ToUTF8
{
    class Utf8Encoder;
}

namespace ESM
{
    class Reader
    {
        std::vector<Reader*>* mGlobalReaderList;

    public:
        virtual ~Reader() {}

        static Reader* getReader(const std::string& filename);

        void setGlobalReaderList(std::vector<Reader*> *list) {mGlobalReaderList = list;}
        std::vector<Reader*> *getGlobalReaderList() {return mGlobalReaderList;}

        virtual inline bool isEsm4() const = 0;

        virtual inline bool hasMoreRecs() const = 0;

        virtual inline void setEncoder(const ToUTF8::StatelessUtf8Encoder* encoder) = 0;

        // used to check for dependencies e.g. CS::Editor::run()
        virtual inline const std::vector<ESM::MasterData>& getGameFiles() const = 0;

        // used by ContentSelector::ContentModel::addFiles()
        virtual inline const std::string getAuthor() const = 0;
        virtual inline const std::string getDesc() const = 0;
        virtual inline int getFormat() const = 0;

        virtual inline std::string getFileName() const = 0;

        // used by CSMWorld::Data::startLoading() and getTotalRecords() for loading progress bar
        virtual inline int getRecordCount() const = 0;

        virtual void setModIndex(std::uint32_t index) = 0;

        // used by CSMWorld::Data::getTotalRecords()
        virtual void close() = 0;

    protected:
        bool getStringImpl(std::string& str, std::size_t size,
                std::istream& stream, const ToUTF8::StatelessUtf8Encoder* encoder, bool hasNull = false);
    };
}

#endif // COMPONENT_ESM_READER_H

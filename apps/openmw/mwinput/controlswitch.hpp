#ifndef MWINPUT_CONTROLSWITCH_H
#define MWINPUT_CONTROLSWITCH_H

#include <cstdint>
#include <map>
#include <string>
#include <string_view>

namespace ESM
{
    struct ControlsState;
    class ESMReader;
    class ESMWriter;
}

namespace Loading
{
    class Listener;
}

namespace MWInput
{
    class ControlSwitch
    {
    public:
        ControlSwitch();

        bool get(std::string_view key);
        void set(std::string_view key, bool value);
        void clear();

        void write(ESM::ESMWriter& writer, Loading::Listener& progress);
        void readRecord(ESM::ESMReader& reader, uint32_t type);
        size_t countSavedGameRecords() const;

    private:
        std::map<std::string, bool, std::less<>> mSwitches;
    };
}
#endif

#ifndef MWINPUT_CONTROLSWITCH_H
#define MWINPUT_CONTROLSWITCH_H

#include <map>

namespace MWInput
{
    class ControlSwitch
    {
    public:
        ControlSwitch();

        bool get(const std::string& key);
        void set(const std::string& key, bool value);
        void clear();

    private:
        std::map<std::string, bool> mSwitches;
    };
}
#endif

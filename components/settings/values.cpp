#include "values.hpp"

namespace Settings
{
    Values* Values::sValues = nullptr;

    void Values::init()
    {
        static Values values;
        Values::sValues = &values;
    }
}

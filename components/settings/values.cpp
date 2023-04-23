#include "values.hpp"

#include <stdexcept>

namespace Settings
{
    Values* Values::sValues = nullptr;

    void Values::initDefaults()
    {
        if (Values::sValues != nullptr)
            throw std::logic_error("Default settings already initialized");
        static Values values;
        Values::sValues = &values;
    }

    void Values::init()
    {
        if (Values::sValues == nullptr)
            throw std::logic_error("Default settings are not initialized");
        static Values values(std::move(*Values::sValues));
        Values::sValues = &values;
    }
}

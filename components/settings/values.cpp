#include "values.hpp"

#include <stdexcept>

namespace Settings
{
    Index* StaticValues::sIndex = nullptr;
    Values* StaticValues::sValues = nullptr;

    void StaticValues::initDefaults()
    {
        if (sValues != nullptr)
            throw std::logic_error("Default settings already initialized");
        static Index index;
        static Values values(index);
        sIndex = &index;
        sValues = &values;
    }

    void StaticValues::init()
    {
        if (sValues == nullptr)
            throw std::logic_error("Default settings are not initialized");
        static Values values(std::move(*sValues));
        sValues = &values;
    }
}

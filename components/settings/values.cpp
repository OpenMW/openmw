#include "values.hpp"

#include <stdexcept>

namespace Settings
{
    std::unique_ptr<Index> StaticValues::sIndex;
    std::unique_ptr<Values> StaticValues::sDefaultValues;
    std::unique_ptr<Values> StaticValues::sValues;

    void StaticValues::initDefaults()
    {
        if (sDefaultValues != nullptr)
            throw std::logic_error("Default settings are already initialized");
        sIndex = std::make_unique<Index>();
        sDefaultValues = std::make_unique<Values>(*sIndex);
    }

    void StaticValues::init()
    {
        if (sDefaultValues == nullptr)
            throw std::logic_error("Default settings are not initialized");
        sValues = std::make_unique<Values>(std::move(*sDefaultValues));
    }

    void StaticValues::clear()
    {
        sValues = nullptr;
        sDefaultValues = nullptr;
        sIndex = nullptr;
    }
}

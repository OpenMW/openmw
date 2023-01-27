#ifndef COMPONENTS_ESM_ESMBRIDGE
#define COMPONENTS_ESM_ESMBRIDGE
#include <string>
#include <string_view>
#include <variant>

#include <components/misc/notnullptr.hpp>

namespace ESM4
{
    struct Cell;
}

namespace ESM
{
    struct Cell;
    struct CellId;
    struct RefId;

    struct CellVariant
    {
        std::variant<const ESM4::Cell*, const ESM::Cell*, const void*> mVariant;

        CellVariant()
            : mVariant((void*)(nullptr))
        {
        }

        explicit CellVariant(const ESM4::Cell* cell)
            : mVariant(cell)
        {
        }

        explicit CellVariant(const ESM::Cell* cell)
            : mVariant(cell)
        {
        }

        bool isValid() const
        {
            return std::holds_alternative<const ESM4::Cell*>(mVariant)
                || std::holds_alternative<const ESM::Cell*>(mVariant);
        }

        bool isEsm4() const { return std::holds_alternative<const ESM4::Cell*>(mVariant); }

        const ESM4::Cell& getEsm4() const;

        const ESM::Cell& getEsm3() const;
    };
}

#endif

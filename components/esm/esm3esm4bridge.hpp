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
    // Common interface for esm3 and esm4 cells
    struct CellCommon
    {
        virtual int getGridX() const = 0;
        virtual int getGridY() const = 0;
        virtual bool isExterior() const = 0;
        virtual bool isQuasiExterior() const = 0;
        virtual bool hasWater() const = 0;
        virtual bool noSleep() const { return false; }
        virtual const ESM::CellId& getCellId() const = 0;
        virtual const ESM::RefId& getRegion() const = 0;
        virtual std::string_view getEditorName() const = 0;
        virtual std::string getDescription() const = 0;
    };

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

        const ESM::CellCommon* getCommon() const;
    };
}

#endif

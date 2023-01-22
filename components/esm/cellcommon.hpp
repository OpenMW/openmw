#ifndef COMPONENTS_ESM_CELLCOMMON
#define COMPONENTS_ESM_CELLCOMMON
#include <string>
#include <string_view>
#include <variant>

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
        std::variant<const ESM4::Cell*, const ESM::Cell*> mVariant;

        explicit CellVariant(const ESM4::Cell* cell)
            : mVariant(cell)
        {
        }

        explicit CellVariant(const ESM::Cell* cell)
            : mVariant(cell)
        {
        }

        bool isEsm4() const { return getEsm4(); }

        const ESM4::Cell* getEsm4() const
        {
            auto cell4 = std::get_if<const ESM4::Cell*>(&mVariant);
            if (cell4)
                return *cell4;
            return nullptr;
        }

        const ESM::Cell* getEsm3() const
        {
            auto cell3 = std::get_if<const ESM::Cell*>(&mVariant);
            if (cell3)
                return *cell3;
            return nullptr;
        }

        const ESM::CellCommon* getCommon() const;
    };
}

#endif

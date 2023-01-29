#ifndef COMPONENTS_ESM_ESMBRIDGE
#define COMPONENTS_ESM_ESMBRIDGE
#include <string>
#include <string_view>
#include <variant>

#include <components/esm3/cellref.hpp>
#include <components/esm4/loadrefr.hpp>

namespace ESM4
{
    struct Cell;
}

namespace ESM
{
    struct Cell;
    struct CellId;
    struct RefId;

    class CellVariant
    {
    protected:
        std::variant<const ESM4::Cell*, const ESM::Cell*> mVariant;

    public:
        explicit CellVariant(const ESM4::Cell& cell)
            : mVariant(&cell)
        {
        }

        explicit CellVariant(const ESM::Cell& cell)
            : mVariant(&cell)
        {
        }

        bool isEsm4() const { return std::holds_alternative<const ESM4::Cell*>(mVariant); }
        const ESM4::Cell& getEsm4() const;
        const ESM::Cell& getEsm3() const;

        template <class F>
        auto visit(F&& f) const
        {
            return std::visit(f, mVariant);
        }
        template <class F>
        auto visit(F&& f)
        {
            return std::visit(f, mVariant);
        }
        template <class F>
        auto visit(F&& f, const CellVariant& v2) const
        {
            return std::visit(f, mVariant, v2.mVariant);
        }
    };

    struct ReferenceVariant
    {
        std::variant<ESM::CellRef, ESM4::Reference> mVariant;

        explicit ReferenceVariant(const ESM4::Reference& ref)
            : mVariant(ref)
        {
        }

        explicit ReferenceVariant(const ESM::CellRef& ref)
            : mVariant(ref)
        {
        }

        bool isESM4() const { return std::holds_alternative<ESM4::Reference>(mVariant); }

        const ESM::CellRef& getEsm3() const { return std::get<ESM::CellRef>(mVariant); }
        const ESM4::Reference& getEsm4() const { return std::get<ESM4::Reference>(mVariant); }

        ESM::CellRef& getEsm3() { return std::get<ESM::CellRef>(mVariant); }
        ESM4::Reference& getEsm4() { return std::get<ESM4::Reference>(mVariant); }
    };
}

namespace std
{
    template <class F>
    auto visit(F&& f, const ESM::CellVariant& v)
    {
        return v.visit(f);
    }
    template <class F>
    auto visit(F&& f, ESM::CellVariant& v)
    {
        return v.visit(f);
    }
    template <class F>
    auto visit(F&& f, const ESM::CellVariant& v1, const ESM::CellVariant& v2)
    {
        return v1.visit(f, v2);
    }
}
#endif

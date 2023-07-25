#ifndef COMPONENTS_ESM_ESMBRIDGE
#define COMPONENTS_ESM_ESMBRIDGE

#include <string>
#include <string_view>
#include <variant>

#include <components/esm3/cellref.hpp>
#include <components/esm4/loadachr.hpp>
#include <components/esm4/loadrefr.hpp>

namespace ESM4
{
    struct Cell;
}

namespace ESM
{
    struct Cell;
    class RefId;

    class CellVariant;

    template <class, class T>
    using Substitute = T;

    template <class F, class... T>
    using VisitReturnType = std::enable_if_t<(std::is_base_of_v<CellVariant, std::decay_t<T>> && ...),
        typename std::invoke_result<F, Substitute<T, const Cell&>...>::type>;

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

        template <class F, class... T>
        friend VisitReturnType<F, T...> visit(F&& f, T&&... v);
    };

    struct ReferenceVariant
    {
        std::variant<ESM::CellRef, ESM4::Reference, ESM4::ActorCharacter> mVariant;

        explicit ReferenceVariant(const ESM4::Reference& ref)
            : mVariant(ref)
        {
        }

        explicit ReferenceVariant(const ESM4::ActorCharacter& ref)
            : mVariant(ref)
        {
        }

        explicit ReferenceVariant(const ESM::CellRef& ref)
            : mVariant(ref)
        {
        }
    };

    template <class F, class... T>
    VisitReturnType<F, T...> visit(F&& f, T&&... v)
    {
        return std::visit([&](auto*... ptr) { return std::forward<F>(f)(*ptr...); }, std::forward<T>(v).mVariant...);
    }

    template <class... Ts>
    struct VisitOverload : Ts...
    {
        using Ts::operator()...;
    };

    template <class... Ts>
    VisitOverload(Ts...) -> VisitOverload<Ts...>;
}
#endif
